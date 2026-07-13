# K230 Flash 建模验证方案。


## 一、当前 K230 SPI 配置分析

**问题发现**：当前 K230 的 SPI 控制器虽然创建了 SSI 总线，但**没有连接任何 Flash 设备**！

对比 SiFive U 平台的实现：

| 平台 | Flash 设备 | 连接方式 |
|------|-----------|---------|
| SiFive U | `is25wp256` | `qdev_realize_and_unref(flash_dev, BUS(s->soc.spi0.spi), ...)` |
| **K230** | **无** | **未连接** |

这意味着当前的 K230 SPI 建模缺少关键的 Flash 外设支持，需要先添加。

---

## 二、验证方案设计

### 阶段 1：添加 Flash 外设支持

#### 1.1 修改 k230.c 添加 Flash 设备

```c
// 在 k230_soc_realize 函数中，sysbus_connect_irq(&s->spi, ...) 之后添加：

DeviceState *flash_dev = qdev_new("is25wp256");
qdev_realize_and_unref(flash_dev, BUS(&s->spi.spi), &error_fatal);

qemu_irq flash_cs = qdev_get_gpio_in_named(flash_dev, SSI_GPIO_CS, 0);
sysbus_connect_irq(SYS_BUS_DEVICE(&s->spi), 0, flash_cs);
```

#### 1.2 支持通过 QEMU 命令行挂载 Flash 镜像

添加对 `-drive` 参数的支持，允许用户指定 Flash 镜像文件：

```c
DriveInfo *dinfo = drive_get(IF_MTD, 0, 0);
if (dinfo) {
    qdev_prop_set_drive_err(flash_dev, "drive",
                            blk_by_legacy_dinfo(dinfo),
                            &error_fatal);
}
```

---

### 阶段 2：驱动层面验证 OSPI

#### 2.1 验证方案

通过 Linux 驱动访问 SPI Flash，验证以下功能：

| 验证项 | 方法 | 预期结果 |
|--------|------|---------|
| Flash 识别 | 读取 JEDEC ID | 返回正确的 Flash ID（如 W25Q128 的 0xEF4018） |
| 标准 SPI 读 | 读取 Flash 内容 | 正确返回数据 |
| OSPI 模式切换 | 配置 Dual/Quad/Octal | 模式切换成功 |
| OSPI 快速读 | 使用 Quad SPI 读命令 | 数据正确，速度提升 |

#### 2.2 测试命令（Linux shell 中）

```bash
# 1. 检查 SPI 设备
ls /dev/spidev*

# 2. 使用 flashrom 或 mtd-utils 读取 Flash ID
flashrom -p linux_spi:dev=/dev/spidev0.0

# 3. 读取 Flash 内容
dd if=/dev/mtdblock0 of=/tmp/flash_dump.bin bs=4k count=1

# 4. 验证 XIP 窗口（如果 XIP 模式已启用）
hexdump -C /sys/class/misc/xip/device/mem | head
```

---

### 阶段 3：验证 XIP 模式

#### 3.1 XIP 验证原理

XIP（eXecute In Place）模式允许 CPU 直接从 Flash 内存窗口执行代码，无需先加载到 RAM。

**验证流程**：

```
1. 将代码写入 Flash
2. 启用 XIP 模式（配置 SPI_CTRLR0.XIP_INST_EN = 1）
3. CPU 从 XIP 地址空间（0xC0000000）读取/执行代码
4. 验证读取的数据与写入的数据一致
```

#### 3.2 验证步骤

**步骤 1：创建包含测试代码的 Flash 镜像**

```bash
# 创建 16MB Flash 镜像
dd if=/dev/zero of=flash.img bs=1M count=16

# 将测试代码写入镜像（偏移 0）
dd if=test_code.bin of=flash.img bs=1 conv=notrunc
```

**步骤 2：启动 QEMU 挂载 Flash 镜像**

```bash
qemu-system-riscv64 -machine k230 \
    -drive file=flash.img,if=mtd,format=raw \
    -kernel Image \
    -dtb k230-canmv.dtb \
    -initrd rootfs.cpio.gz \
    -append "console=ttyS0,115200 earlycon=sbi"
```

**步骤 3：在 Linux 中验证 XIP**

```bash
# 1. 启用 XIP 模式
# 通过 SPI 控制器寄存器配置

# 2. 读取 XIP 窗口内容
hexdump -C /proc/iomem | grep flash
# 应该看到类似：c0000000-c7ffffff : flash

# 3. 直接从 XIP 地址读取
devmem 0xC0000000 32
# 应该返回写入的测试代码内容
```

---

### 阶段 4：从 Flash 启动 Linux

#### 4.1 启动流程原理

```
Flash (0xC0000000)
    │
    ├── U-Boot (偏移 0)
    │       │
    │       ├── 初始化硬件
    │       ├── 读取 Linux Kernel
    │       └── 启动 Linux
    │
    └── Linux Kernel (偏移 0x100000)
            │
            └── 挂载 rootfs
```

#### 4.2 实现要点

**关键条件**：

| 条件 | 说明 |
|------|------|
| **XIP 模式支持** | CPU 必须能直接从 Flash 地址空间执行代码 |
| **Flash 内容正确** | U-Boot 必须位于 Flash 起始位置 |
| **设备树正确** | 必须包含 SPI Flash 和 XIP 窗口的正确描述 |
| **启动地址正确** | CPU 复位向量必须指向 Flash 地址 |

#### 4.3 验证步骤

**步骤 1：构建包含 U-Boot 的 Flash 镜像**

```bash
# 创建 16MB Flash 镜像
dd if=/dev/zero of=flash.img bs=1M count=16

# 将 U-Boot 写入偏移 0
dd if=u-boot-spl.bin of=flash.img bs=1 conv=notrunc

# 将完整 U-Boot 写入偏移 0x10000
dd if=u-boot.bin of=flash.img bs=1 seek=$((0x10000)) conv=notrunc

# 将 Linux Kernel 写入偏移 0x100000
dd if=Image of=flash.img bs=1 seek=$((0x100000)) conv=notrunc

# 将 DTB 写入偏移 0x200000
dd if=k230-canmv.dtb of=flash.img bs=1 seek=$((0x200000)) conv=notrunc
```

**步骤 2：修改 QEMU 启动地址**

需要修改 `k230_machine_init`，让 CPU 从 Flash 地址启动：

```c
// 当前：从 DDR 启动
start_addr = memmap[K230_DEV_DDRC].base;

// 修改为：从 Flash XIP 窗口启动
start_addr = memmap[K230_DEV_FLASH].base;
```

**步骤 3：启动 QEMU**

```bash
qemu-system-riscv64 -machine k230 \
    -drive file=flash.img,if=mtd,format=raw \
    -no-reboot \
    -serial stdio
```

**步骤 4：验证启动流程**

```
U-Boot 启动信息...
K230#  (U-Boot 提示符)
U-Boot> bootm 0xC100000 - 0xC200000
Starting kernel ...
Linux 启动信息...
~ #  (Linux shell)
```

---

## 三、关键技术要点

### 1. Flash 设备连接

QEMU 中 SPI Flash 的连接需要：
- 创建 Flash 设备（如 `is25wp256`）
- 将其连接到 SPI 总线
- 连接片选信号（CS）

### 2. XIP 窗口映射

K230 的 XIP 窗口位于 `0xC0000000`，大小 128MB，需要：
- 在 `k230.c` 中正确映射 `K230_DEV_FLASH`
- 在设备树中正确描述

### 3. 启动地址配置

CPU 复位后的启动地址决定了从哪里开始执行：
- `0x80000000`：从 DDR 启动（当前方式）
- `0xC0000000`：从 Flash XIP 窗口启动

### 4. Linux 驱动支持

需要 Linux 内核支持：
- SPI Flash 驱动（`spi-nor`）
- MTD 子系统
- XIP 支持

---

## 四、实现优先级

| 优先级 | 任务 | 描述 |
|--------|------|------|
| **P0** | 添加 Flash 设备 | 在 k230.c 中连接 SPI Flash |
| **P0** | 支持 Flash 镜像 | 支持 `-drive` 参数挂载 Flash |
| **P1** | 驱动验证 | 在 Linux 中验证 Flash 读写 |
| **P1** | XIP 验证 | 验证 XIP 窗口访问 |
| **P2** | Flash 启动 | 实现从 Flash 启动 Linux |

---

## 五、总结

当前 K230 QEMU 实现的主要缺失是**未连接 Flash 外设**。要验证 OSPI/XIP 建模，需要：

1. **添加 Flash 设备**到 SPI 总线
2. **支持通过 QEMU 命令行**挂载 Flash 镜像
3. **通过 Linux 驱动**验证 Flash 读写功能
4. **配置 XIP 模式**并验证内存窗口访问
5. **修改启动地址**实现从 Flash 启动

这个方案从底层硬件连接开始，逐步验证到完整的 Linux 启动，覆盖了 OSPI 和 XIP 的所有关键功能。