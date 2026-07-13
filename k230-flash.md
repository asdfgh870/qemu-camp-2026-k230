# K230 Flash

## K230 参考资料
K230 DEMO BOARD 和flash 扩展：https://www.kendryte.com/k230/zh/main/00_hardware/K230_DEMO_BOARD%E8%B5%84%E6%BA%90%E4%BD%BF%E7%94%A8%E6%8C%87%E5%8D%97.html#demo-board

相关讨论见 [PATCH v8 0/5] Add support for K230 board，合入后的官方文档见 QEMU k230 machine 文档。
https://lore.kernel.org/qemu-devel/cover.1781246408.git.chao.liu@processmission.com/
https://gitlab.com/qemu-project/qemu/-/blob/master/docs/system/riscv/k230.rst

## 一、K230 SoC 官方手册资源
嘉楠科技（Kendryte）官方已公开 K230 系列完整硬件文档，均可在官方开发者站点获取，核心手册与获取方式如下：

### 1. 核心技术手册
- **《K230 Technical Reference Manual（技术参考手册 / TRM）》**
  - 版本：V0.3.1（2024-11-18）
  - 内容：完整的芯片架构、所有外设寄存器定义、系统地址映射、功能时序说明，包含 OSPI/QSPI 控制器的详细配置、Flash Memory Window 机制与寄存器定义，是底层开发、硬件建模的核心权威依据。
  - 官方 PDF 下载：[K230_Technical_Reference_Manual_V0.3.1](https://kendryte-download.canaan-creative.com/developer/k230/HDK/K230%E7%A1%AC%E4%BB%B6%E6%96%87%E6%A1%A3/K230_Technical_Reference_Manual_V0.3.1_20241118.pdf)

- **《K230 Product Full Datasheet（完整数据手册）》**
  - 内容：芯片电气参数、封装引脚定义、外设电气特性、交直流时序参数，是硬件板级设计的核心参考。
  - 官方在线版：[K230 Full Datasheet](https://www.kendryte.com/k230/en/v1.8/00_hardware/K230_datasheet.html)

- **《K230 Product Brief（产品简介）》**
  - 内容：芯片核心规格、架构框图、外设清单的精简概览，用于快速选型与方案评估。
  - 官方在线版：[K230 Product Brief](https://www.kendryte.com/k230/en/main/K230_brief_datasheet.html)

### 2. 配套工程设计文档
- **《K230 硬件设计指南》**
  - 内容：包含 OSPI/QSPI Flash 硬件电路设计、引脚复用规则、启动模式配置、DDR 布线指引等工程化内容，明确了 OSPI Flash 的典型器件选型（如 GD25LX256E 256Mbit NOR Flash）与参考电路。
  - 官方在线版：[K230 硬件设计指南](https://developer.canaan-creative.com/k230/zh/v1.8/00_hardware/K230_%E7%A1%AC%E4%BB%B6%E8%AE%BE%E8%AE%A1%E6%8C%87%E5%8D%97.html)

---
### 3. 查询K230 寄存器和系统地址段分布的手册
https://download.kendryte.com/developer/k230/HDK/K230%E7%A1%AC%E4%BB%B6%E6%96%87%E6%A1%A3/K230_Technical_Reference_Manual_V0.3.1_20241118.pdf


## 二、K230 SoC 核心描述（基于官方手册）
K230 是嘉楠科技 Kendryte 系列的新一代 AIoT 异构 SoC，面向智能门锁、词典笔、消费级 IPC、工业扫码器等边缘智能场景设计，核心架构与特性如下：

### 1. 计算与异构架构
- **CPU 子系统**：集成 2 颗 RISC-V C908 核心，采用大小核异构设计，大核最高主频 1.6GHz，小核最高 800MHz，支持异构调度与低功耗管理。
- **AI 加速单元（KPU）**：内置新一代神经网络处理器，支持 INT8/INT16 多精度算力，主流 AI 网络利用率超过 70%。
- **专用加速引擎**：集成 2D 图形引擎、2.5D GPU、深度计算单元（DPU）、GZIP 硬件解压引擎（带宽≥400MB/s）、4096 点 FFT/IFFT 加速器。

### 2. 存储接口（与 Flash Window 直接相关）
- **内存控制器**：原生支持 LPDDR3/LPDDR4，最高 32bit 位宽，支持 2 个 rank；另有 K230D 版本为 SiP 封装，内置 1Gb LPDDR4 颗粒。
- **Flash 存储控制器**：
  - 1 路 **OSPI 控制器**：支持 4/8bit 位宽的 NOR Flash，最高支持 DDR200 / SDR166 速率，内置硬件地址映射窗口，支持 XIP 就地执行；
  - 2 路 QSPI 控制器：支持 1/2/4bit 位宽的 NOR/NAND Flash，最高 SDR100 速率；
  - 2 路 SD/MMC 接口：兼容 SD3.0、eMMC 5.0 标准。
- 其他高速接口：2 路 USB 2.0 OTG。

### 3. 音视频与通用外设
- **视频子系统**：最高 3 路 MIPI CSI 输入，集成新一代 ISP，支持 H.265 编解码、深度图提取；1 路 MIPI DSI 显示输出。
- **音频子系统**：双 DAC + 双 ADC，最高支持 8 路 PDM 数字麦克风，内置音频 Codec 与 ALC 功能。
- **低速外设**：5 路 UART、5 路 I2C、6 路 PWM，最多支持 64 路通用 GPIO + 8 路 PMU GPIO。
- **安全与电源**：内置 PUF、OTP、真随机数发生器，支持 AES、SHA、RSA 及国密 SM2/3/4 算法；集成 PMU，深度休眠待机功耗 ≤ 20μW。

### 4. Flash Memory Window 对应说明
根据官方 TRM，K230 的系统总线地址空间中，OSPI 控制器被分配了一段固定大小的**存储器映射窗口地址段**。CPU 可通过该窗口以普通内存读写的方式直接访问外部 Flash，窗口内部通过分页偏移寄存器实现“小窗口映射大容量 Flash”的分页机制，与我们之前讨论的地址翻译模型完全一致；窗口基地址、大小、分页寄存器定义均可在 TRM 的 OSPI 控制器章节查到精确数值。

位于https://www.kendryte.com/k230/zh/v1.8/00_hardware/K230_%E7%A1%AC%E4%BB%B6%E8%AE%BE%E8%AE%A1%E6%8C%87%E5%8D%97.html#flash


## K230 Flash Window 逻辑框图 与 flash window 相关设备的位置。
这是K230 芯片的逻辑框图。
![k230.png](k230.png)
K230 Flash Window 对应两个位置，分外部 Flash 访问窗口与内置 OTP 一次性 Flash 窗口
1. 外置 NOR/NAND Flash 访问窗口（OSPI/QSPI 控制器）
在框图左上角 High Speed 高速外设区域：
```plaintext
High Speed
├ USB2.0 OTG×2
├ SD/eMMC HC×2
├ SPI OPI (DRT200)  → OSPI Octal SPI（8线Flash）
└ SPI QPI ×2 (SDR160) → QSPI Quad SPI（4线Flash）
```
这两个控制器是芯片外接 Flash 的硬件接口，系统内存映射的Flash 访问窗口就由这两组 SPI 控制器实现，用来映射外部 NOR/NAND Flash 地址空间，对应硬件引脚 OSPI/QSPI 总线。

## 

## K230 Flash Memory Window 完整解析与建模实现
## 一、什么是 K230 Flash Memory Window（闪存窗口）
### 1. 核心背景
K230 外部挂载大容量 OSPI/QSPI NOR Flash（典型 256MBit/32MB、512MBit/64MB），**Flash 物理地址空间远大于 CPU 直接可寻址的片上映射窗口大小**，无法一次性把整片 Flash 线性映射到系统总线地址段。系统总线地址段通常只有 128MB，而 Flash 物理地址空间可能有 256MB、512MB 等。
Flash Memory Window（简称 Flash 分页窗口/窗口映射）是 **OSPI/QSPI Flash 控制器内置的地址分页重映射硬件机制**：
1. CPU 侧只暴露一段固定大小、连续的**窗口虚拟地址段**（Window Base）；
2. 通过控制器内**页偏移配置寄存器**（Flash Page Offset Register）指定 Flash 物理基址；
3. CPU 访问窗口内地址时，硬件自动拼接「窗口偏移 + 寄存器配置的 Flash 页基址」，换算出完整 Flash 物理地址，发起 OSPI/QSPI 总线读/写/擦除；
4. 窗口大小固定（K230 标准窗口 4MB/8MB，手册定义），切换 Flash 大段数据时只需要改写页偏移寄存器，不用修改总线内存布局。

### 2. 硬件工作原理（两级地址翻译）
设：
- `WINDOW_BASE`：CPU 总线窗口基地址（如 `0x30000000`）
- `WINDOW_SIZE`：窗口固定大小（4MB = 0x400000）
- `WIN_OFFSET_REG`：控制器分页寄存器（存储 Flash 物理页起始地址，对齐窗口大小）
- `cpu_addr`：CPU 访问的总线地址
- `flash_phys_addr`：Flash 芯片内部绝对物理地址

地址转换公式：
```
window_off = cpu_addr - WINDOW_BASE          // 窗口内局部偏移 [0, WINDOW_SIZE)
flash_phys_addr = WIN_OFFSET_REG + window_off
```
**硬件行为**
1. CPU 发起对 `0x3000XXXX` 的读操作，总线路由到 OSPI 控制器；
2. 控制器提取低 22bit 窗口偏移，加上寄存器预配置的 Flash 基址；
3. 把拼接后的 24/32bit Flash 地址通过 OSPI 总线发给 Flash 芯片；
4. Flash 返回数据，原路回传给 CPU；
5. 写/擦除操作同理，窗口地址映射自动生效。

### 3. 设计目的
1. **节省总线地址段**：不用预留几十 MB 连续总线地址给 Flash；
2. **统一指令取指/数据访问接口**：代码、常量、固件数据均可通过窗口直接内存式访问，无需 SPI 寄存器收发；
3. **简化软件分页逻辑**：访问 Flash 不同大分区仅写 1 个偏移寄存器，无需复杂 SPI 命令封装；
4. **支持 XIP（片上执行）**：将固件镜像映射进窗口，CPU 直接从外部 Flash 取指运行，不用全部加载到 DDR。

### 4. 与普通 SPI Flash 的区别
普通 SPI Flash 只能通过**寄存器收发命令+地址+数据**间接访问；
K230 Flash Window 是**硬件地址透明映射**，CPU 把 Flash 窗口当成普通内存段读写，控制器底层自动封装 OSPI 时序。

## 二、K230 Flash Window 硬件模块组成
1. **OSPI/QSPI Controller 寄存器组**
   - `WIN_BASE`：只读，硬件固化窗口总线基地址；
   - `WIN_SIZE`：只读，窗口长度（4MB）；
   - `FLASH_PAGE_OFFSET`：可写分页寄存器，Flash 物理页基址（4MB 对齐）；
   - `WIN_EN`：窗口使能位，关闭后窗口地址访问返回总线错误；
   - 状态寄存器：Flash 忙、读写完成、擦除完成标志。
2. **地址翻译组合逻辑**
   组合电路实时拼接窗口偏移 + 分页寄存器值，输出完整 Flash 物理地址到 OSPI 发送 FIFO。
3. **OSPI 物理层收发引擎**
   接收翻译后的 Flash 地址，自动发送 8线 OSPI 读/页编程/扇区擦除指令，处理 Flash 时序、Dummy Cycle、DDR/SDR 模式。
4. **数据 FIFO**
   缓存 CPU 读写数据，匹配 CPU AXI/APB 总线带宽与 Flash 串行接口带宽。

## 三、建模实现方案（分两种场景：Verilog RTL 硬件建模 / QEMU 仿真建模）
### 方案1：RTL 硬件建模（Verilog，用于SoC仿真/IP验证）
#### 模块分层
```
Top: ospi_flash_controller
├─ RegFile：窗口配置寄存器（WIN_OFFSET、WIN_EN）
├─ WindowAddrTranslate：地址翻译组合逻辑（核心窗口模块）
├─ OspiTxEngine：OSPI 命令/地址发送
├─ OspiRxEngine：Flash 数据接收
└─ FlashModel：外挂 Flash 行为模型（GD25LX256E 时序仿真）
```
#### 1. 窗口地址翻译模块核心代码（WindowAddrTranslate）
```verilog
module WindowAddrTranslate #(
    parameter WINDOW_BASE  = 32'h3000_0000,
    parameter WINDOW_MASK  = 32'h003F_FFFF, // 4MB mask
    parameter FLASH_ADDR_W = 26
)(
    input  wire [31:0] cpu_axi_addr,
    input  wire [FLASH_ADDR_W-1:0] page_offset_reg, // 分页寄存器输出
    output reg [FLASH_ADDR_W-1:0] flash_phys_addr,
    output reg window_hit // 1=地址落在窗口内
);

wire [31:0] window_local_off;

// 判断是否命中窗口地址段
assign window_hit = (cpu_axi_addr >= WINDOW_BASE) && 
                    (cpu_axi_addr <= WINDOW_BASE + WINDOW_MASK);

// 提取窗口内部偏移
assign window_local_off = cpu_axi_addr & WINDOW_MASK;

// 地址拼接输出Flash物理地址
always @(*) begin
    if(window_hit) begin
        flash_phys_addr = page_offset_reg + window_local_off[FLASH_ADDR_W-1:0];
    end else begin
        flash_phys_addr = '0;
    end
end

endmodule
```
#### 2. 寄存器模块建模要点
- 实现 APB 寄存器读写：写 `FLASH_PAGE_OFFSET` 时自动对齐窗口大小（4MB 边界，自动低 22bit 清零）；
- `WIN_EN=0` 时，`window_hit=1` 也返回 AXI SLVERR 总线报错；
- 寄存器复位默认 `page_offset_reg=0`，上电默认映射 Flash 0~4MB。
#### 3. 读写时序联动建模
1. CPU AXI 发起读，窗口命中 → 计算 `flash_phys_addr`；
2. TxEngine 自动发送 OSPI FAST READ 指令 + 24bit Flash 地址；
3. 等待 Flash Dummy Cycle，RxEngine 接收数据送入 AXI RDATA；
4. 写操作：自动发送 PAGE PROGRAM 指令，FIFO 缓存 256 字节页数据；
5. 擦除：CPU 写窗口控制寄存器触发扇区/整片擦除，等待 BUSY 标志。

### 方案2：QEMU 系统仿真建模（RISC-V K230 虚拟机，软件行为建模）
基于 QEMU `MemoryRegion` 地址空间抽象实现分页窗口重映射，**无需硬件时序，纯行为仿真**。
#### 核心原理
1. 分配一段固定 `MemoryRegion mr_window` 作为 CPU 可见 4MB 窗口总线地址；
2. 底层维护完整 Flash 镜像 `uint8_t *flash_storage`（32MB/64MB）；
3. 实现自定义 MMIO 寄存器区域（OSPI 控制寄存器），保存全局 `flash_page_base`（分页偏移）；
4. 重载窗口 MemoryRegion 的 `read/write` 回调函数：访问时做地址换算，读写底层 Flash 镜像。

#### 1. 关键数据结构（C）
```c
typedef struct K230OspiState {
    DeviceState parent;
    // Flash 全局存储镜像
    uint8_t *flash_storage;
    uint32_t flash_size;

    // Window 窗口配置寄存器
    uint32_t reg_win_offset;  // FLASH_PAGE_OFFSET
    bool win_en;              // WIN_EN 使能位

    // 窗口内存区域
    MemoryRegion mr_window;   // CPU可见4MB窗口
    MemoryRegion mr_regs;     // OSPI控制寄存器MMIO
} K230OspiState;

#define WINDOW_BASE 0x30000000
#define WINDOW_SIZE 0x400000UL // 4MB
```
#### 2. 窗口读回调函数（地址翻译核心）
```c
static uint64_t k230_flash_window_read(void *opaque, hwaddr addr, unsigned size)
{
    K230OspiState *s = opaque;
    if (!s->win_en) return 0xffffffff; // 窗口关闭返回无效值

    // 1. 计算窗口内偏移
    hwaddr win_off = addr - WINDOW_BASE;
    // 2. 拼接分页寄存器基址，得到Flash绝对地址
    hwaddr flash_phys = s->reg_win_offset + win_off;

    // 边界检查
    if (flash_phys + size > s->flash_size)
        return 0xffffffff;

    // 从Flash镜像读取对应数据
    uint64_t ret = 0;
    memcpy(&ret, s->flash_storage + flash_phys, size);
    return ret;
}
```
#### 3. 窗口写回调（模拟 Flash 编程特性）
Flash 不能直接写1，只能从1→0，写前必须擦除为0xFF；建模时增加校验逻辑：
```c
static void k230_flash_window_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    K230OspiState *s = opaque;
    if (!s->win_en) return;

    hwaddr win_off = addr - WINDOW_BASE;
    hwaddr flash_phys = s->reg_win_offset + win_off;
    if (flash_phys + size > s->flash_size) return;

    // Flash 硬件约束：只能置0，不能置1
    uint8_t *dst = s->flash_storage + flash_phys;
    uint8_t src[8];
    memcpy(src, &val, size);
    for(int i=0; i<size; i++){
        dst[i] &= src[i];
    }
}
```
#### 4. 寄存器切换分页逻辑
写 `reg_win_offset` 寄存器时自动对齐窗口大小：
```c
static void ospi_reg_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    K230OspiState *s = opaque;
    switch(addr){
        case REG_FLASH_PAGE_OFFSET:
            // 强制4MB对齐，低22bit清零
            s->reg_win_offset = val & (~(WINDOW_SIZE - 1));
            break;
        case REG_WIN_EN:
            s->win_en = !!(val & BIT(0));
            break;
        // 擦除命令寄存器：实现扇区擦除，对应Flash镜像整片置0xFF
        case REG_ERASE_SECTOR:
            hwaddr sec_base = s->reg_win_offset + ((val & 0xFF) << 12);
            memset(s->flash_storage + sec_base, 0xFF, 4096);
            break;
    }
}
```
#### 5. 设备初始化挂载地址空间
```c
static void k230_ospi_realize(DeviceState *dev, Error **errp)
{
    K230OspiState *s = K230_OSPI(dev);
    // 分配Flash镜像
    s->flash_storage = g_malloc0(s->flash_size);
    memset(s->flash_storage, 0xFF, s->flash_size); // Flash默认擦除值0xFF

    // 1. 挂载4MB窗口内存段，绑定读写回调
    memory_region_init_io(&s->mr_window, OBJECT(s),
                          &k230_window_ops, s,
                          "k230-flash-window", WINDOW_SIZE);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, WINDOW_BASE);

    // 2. 挂载OSPI控制寄存器MMIO
    memory_region_init_io(&s->mr_regs, OBJECT(s),
                          &k230_ospi_reg_ops, s,
                          "k230-ospi-regs", 0x1000);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 1, 0x30400000);
}
```
#### 6. DTB适配
虚拟机设备树添加OSPI节点，暴露窗口基地址与寄存器地址，与真实K230硬件一致：
```dts
ospi@30400000 {
    compatible = "canaan,k230-ospi";
    reg = <0x30400000 0x1000>, /* 控制寄存器 */
          <0x30000000 0x400000>;/* Flash窗口4MB */
    flash-size = <0x2000000>;  /* 32MB Flash */
};
```

## 四、软件使用逻辑（辅助理解建模行为）
### 1. 读取 Flash 0x1000000 位置（16MB偏移）
1. 计算目标页基址：`0x1000000` 向下4MB对齐 → `0x1000000`；
2. 写 OSPI `FLASH_PAGE_OFFSET = 0x1000000`；
3. CPU 访问窗口地址 `WINDOW_BASE + 0` → 硬件翻译为 `0x1000000`；
4. 直接读取 `*(volatile uint32_t *)0x30000000` 获取 Flash 数据。

### 2. 跨窗口连续访问
循环改写分页寄存器，逐4MB窗口遍历整片Flash，无需底层SPI命令收发。

## 五、建模校验要点（验证窗口逻辑正确性）
1. **边界测试**：窗口最后地址 `0x303FFFFF`，偏移寄存器设 `0x1FC00000`，换算后 Flash 地址 = `0x1FC00000 + 0x3FFFFF = 0x1FFFFFFF`（32MB Flash 末尾）；
2. **未对齐写入分页寄存器**：软件写非4MB对齐值，硬件/模型自动低位清零；
3. **窗口关闭测试**：`WIN_EN=0` 访问窗口返回无效数据/总线错误；
4. **Flash 写保护建模**：写操作只能将bit置0，擦除后恢复0xFF；
5. **XIP 取指验证**：把固件放在 Flash 0~4MB，窗口默认映射，CPU 直接从 `0x30000000` 取指执行。

## 六、RTL vs QEMU 建模取舍
| 建模类型 | 适用场景 | 特性 |
|--------|--------|------|
| Verilog RTL | SoC IP验证、前端仿真、时序验证 | 精准模拟OSPI时序、总线握手、硬件流水线，仿真速度慢 |
| QEMU行为建模 | 固件开发、Linux/RT-Smart 虚拟机、软件调试 | 忽略底层时序，只模拟地址映射与Flash存储行为，运行速度接近真机 |



# SPI 和OSPI/QSPI的区别
## 一、核心定位与演进关系
SPI、QSPI、OSPI 是**串行外设接口的三代演进方案**，核心目标是在串行总线的基础上，通过增加并行数据位宽、引入双数据率（DDR）等方式，大幅提升传输速率，尤其适配大容量串行 NOR Flash 的高速读取与 XIP（就地执行）需求。
演进路径为：
**标准SPI（1bit 数据位宽）→ Dual SPI（2bit）→ QSPI（4bit）→ OSPI（8bit）**

三者都属于同步串行总线，由主机提供时钟（SCLK）和片选（CS），从机跟随时钟传输数据；差异主要体现在数据通道数量、传输速率、协议复杂度和典型应用场景。

---

## 二、各自定义与核心特征
### 1. 标准 SPI（Standard SPI）
这是最基础的同步串行外设接口，由摩托罗拉最早提出。
- **信号线**：共 4 根核心线，`SCLK`（时钟）、`CS`（片选）、`MOSI`（主机发/从机收）、`MISO`（主机收/从机发）
- **双工模式**：**全双工**，MOSI 和 MISO 是单向独立通道，时钟沿下主机发数据的同时可以接收从机数据
- **数据位宽**：单 bit 串行传输，每次时钟沿只传输 1bit 数据
- **典型速率**：10~50MHz，少数高速器件可达 100MHz
- **协议特点**：协议极简，仅定义帧同步和时钟规则，命令格式由外设自定义
- **典型应用**：通用低速外设（传感器、小容量 Flash、显示屏、ADC/DAC、EEPROM 等）

### 2. QSPI（Quad SPI，四线 SPI）
SPI 的位宽扩展版本，专门针对串行 NOR Flash 优化，是目前 MCU/SoC 外置 Flash 的主流接口。
- **信号线**：`SCLK`、`CS` + `IO0~IO3` 共 4 根双向数据线（将原 MOSI/MISO 改造为双向 IO，新增 IO2/IO3）
- **双工模式**：**半双工**，数据线分时复用为发送/接收，同一时间只能单向传输
- **数据位宽**：命令/地址阶段通常沿用单线模式，数据读写阶段切换为 4bit 并行传输，位宽是标准 SPI 的 4 倍
- **典型速率**：80~133MHz SDR（单数据率，仅时钟上升沿传数据），等效吞吐量是同频标准 SPI 的 4 倍
- **关键特性**：原生支持 XIP（就地执行），控制器可自动封装 Flash 读写命令，CPU 可像访问内存一样读取 Flash
- **典型应用**：中大容量 NOR Flash（如 W25Q 系列）、固件存储、引导 ROM、轻量 XIP 代码运行

### 3. OSPI（Octal SPI，八线 SPI）
QSPI 的进一步升级，是当前高端嵌入式 SoC 的高速 Flash 标准接口，K230 即采用 OSPI 接口挂载外置 Flash。
- **信号线**：`SCLK`、`CS` + `IO0~IO7` 共 8 根双向数据线；高速模式下通常附带 `DS`（Data Strobe，数据选通）信号，由 Flash 端返回，用于接收端数据同步
- **双工模式**：**半双工**，分时复用双向数据线
- **数据位宽**：数据阶段 8bit 并行传输，位宽是 QSPI 的 2 倍；普遍支持 **DDR 双数据率**（时钟上升沿、下降沿各传 1 次数据）
- **典型速率**：100~200MHz DDR，等效吞吐量可达 200MB/s 以上，是同频 QSPI DDR 的 2 倍、标准 SPI 的十几倍
- **关键特性**：支持高速 DDR、字节掩码、包传输等高级特性，XIP 性能接近片上 SRAM
- **典型应用**：大容量高速 NOR Flash、大固件 XIP 运行、高性能嵌入式系统主存储

---

## 三、核心差异对比表
| 对比维度 | 标准 SPI | QSPI（四线） | OSPI（八线） |
|---------|---------|-------------|-------------|
| 数据通道数 | 1bit（MOSI+MISO 单向独立） | 4bit（IO0~IO3 双向） | 8bit（IO0~IO7 双向） |
| 双工类型 | 全双工 | 半双工 | 半双工 |
| 典型时钟频率 | 10~50MHz | 80~133MHz SDR | 100~200MHz DDR |
| 等效峰值吞吐量 | ~5MB/s（50MHz） | ~66MB/s（133MHz SDR） | ~400MB/s（200MHz DDR） |
| 协议复杂度 | 极低，仅定义物理层 | 中等，需支持多线模式切换、Flash专用指令 | 高，支持DDR、DS同步、高级命令集 |
| XIP 支持 | 不原生支持，需软件模拟 | 原生支持，性能一般 | 原生支持，性能接近片上内存 |
| 向下兼容性 | 基准接口 | 兼容标准 SPI 模式 | 兼容 QSPI、标准 SPI 模式 |
| 引脚数量（核心） | 4 根 | 6 根（SCLK+CS+4IO） | 9~10 根（SCLK+CS+8IO±DS） |
| 典型应用场景 | 通用低速外设 | 中容量 Flash、普通固件存储 | 大容量高速 Flash、XIP 代码运行 |
| 硬件成本与布线难度 | 最低 | 中等 | 较高 |

---

## 四、常见误区澄清
1. **误区：线越多，全双工性能越强**
   QSPI/OSPI 均为半双工，数据线是分时复用的双向通道，同一时间只能发送或接收；标准 SPI 才是真正的全双工。
   但对于 Flash 这种「发命令地址→收数据」的主从读写场景，半双工完全够用，提升位宽带来的收益远大于全双工。

2. **误区：QSPI 就是 4 根线的普通 SPI**
   QSPI 不只是物理上多了 2 根线，控制器需要支持**多线模式切换**、Flash 专用四线指令集、地址自动封装等逻辑；普通 SPI 控制器无法驱动 QSPI Flash 的四线高速模式。

3. **误区：OSPI 只是 QSPI 多了 4 根线**
   OSPI 的核心升级是 DDR 双数据率、数据选通同步、更高的时钟冗余和更完善的协议特性，性能差距远大于“4 根线”的物理差异；部分 OSPI 还支持 HyperBus 协议兼容。

---

## 五、结合 K230 Flash Window 的实际意义
回到你之前关注的 Flash Memory Window 机制，三者的差异直接决定了硬件架构设计：
- **标准 SPI**：只能通过寄存器手动收发命令、地址、数据，CPU 无法直接内存式访问，不支持 XIP，无法实现 Flash 窗口映射。
- **QSPI/OSPI**：控制器内置地址翻译与自动命令封装逻辑，配合 Flash Window 分页机制，CPU 可以像访问普通内存一样直接读写 Flash，支持 XIP 就地执行代码。
- **K230 选用 OSPI**：8bit 位宽 + DDR 模式带来的高读取速度，可满足大固件、AI 模型、XIP 运行的性能需求；同时用 4MB 固定窗口机制，解决了“大容量 Flash 占用过多系统总线地址段”的问题。

# OSPI、QSPI、标准 SPI 地址映射区分
参考：K230_Technical_Reference_Manual_V0.3.1_20241118.pdf 1.5小节
原因解析，结合给出的地址映射分段说明
1. 硬件控制器独立寄存器基址区分（0x9158xxxx 区间）
0x9158_2000~0x9158_4000：SPI QOPI x2 控制器寄存器配置区
0x9158_4000~0x9158_5000：SPI OPI 控制器寄存器配置区
QOPI、OPI 是两套独立硬件控制器，各自拥有专属寄存器组，所以分配两段不同外设寄存器地址，用于配置时序、指令、读写模式、时钟等硬件参数，CPU 通过不同地址访问对应控制器的控制寄存器。
2. XIP 闪存映射地址独立（0xC000_0000~0xC800_0000）
0xC000_0000~0xC800_0000 是OPI Flash XIP 地址窗口，这是内存映射只读访问空间，和上面控制器寄存器地址完全不是一类用途：
0x9158xxxx：控制器配置寄存器，用来操控 Flash 硬件；
0xC0000000：Flash 存储内容直接映射内存窗口（XIP 片上执行），CPU 可直接读取 Flash 里的程序 / 数据。
3. QOPI、OPI 协议硬件规格不同，控制器物理分离
QOPI（四线 SPI）、OPI（八线 SPI）总线带宽、指令集、时序逻辑硬件电路相互独立：
二者引脚驱动、数据位宽、读写命令寄存器定义不一样；
芯片内部总线架构为每个独立外设控制器分配互不重叠的寄存器地址段，避免地址访问冲突；
仅 OPI 硬件支持 XIP 内存映射窗口，所以只有 OPI 分配了 128MB 的 C 段地址空间，QOPI 无对应的 XIP 映射区间。
总结
寄存器地址区分：QOPI、OPI 是两套独立硬件控制器，寄存器基址分开，方便独立配置；
功能地址分离：0x9158 开头是控制寄存器，0xC000 开头是 Flash 数据 XIP 映射窗口，用途完全不同；
硬件能力差异：OPI 支持 XIP 片上执行，额外分配大容量内存映射地址段，QOPI 无此功能，因此没有对应地址区间。

## 一、XIP 含义（结合手册 K230 硬件说明）
XIP 全称 eXecute In Place，片上就地执行，是 Flash 的内存映射访问机制：
硬件把 SPI OPI Flash 的存储区域直接映射到 CPU 系统总线地址段 0xC000_0000~0xC800_0000；
2 CPU 无需把 Flash 数据搬运到 DDR/SRAM，可直接通过读取该段地址，取 Flash 内代码、数据运行；
3 仅 K23 的OPI 八线 Flash 控制器支持该功能，QSPI 四线控制器无 XIP 映射窗口。
二、0xC000_0000~0xC8000000 地址段实际存储位置
逻辑层面：该地址是系统总线虚拟映射地址，属于芯片内部 AXI 系统地址空间；
物理存储层面：该地址段不位于片内 DDR、SRAM，物理介质是外接 SPI OPI Flash 芯片；
补充区分两段关键地址：
0x9158_4000~0x9158_5000：OPI 控制器配置寄存器，在芯片内部外设模块，用于控制 Flash 读写时序、模式；
0xC000_0000~0xC800：XIP 映射窗口，访问此地址时，硬件自动转发读指令到外部 OPI Flash，读取 Flash 真实存储内容。
空间大小匹配：映射区间 128MB，对应外接 OPI Flash 最大 128MB 寻址范围。
补充区分 QOPI
手册仅分配 QOPI 控制器寄存器段0x9158_2000~0x9158_4000，没有配套 XIP 地址窗口，四线 QSPI Flash 不支持就地执行。



# k230 flash 使用教程
https://www.kendryte.com/k230_canmv/zh/main/example/peripheral/spi.html



# k230 spi 寄存器功能记录
## 一、CTRLR0所属外设
CTRLR0（Control Register 0）属于**SSI（同步串行接口，即OSPI/QSPI控制器）**，也就是文档开头描述的FMC灵活存储控制器下的Octal SPI（OSPI）、Quad SPI（QSPI）主机外设，K230手册里统一叫SSI控制器，IP为DesignWare标准SPI/Octal SPI内核，所有SPI控制器都以`CTRLR0`作为偏移0的主控制寄存器（偏移地址0x0，复位值0x00004007）。
K230一共有3路SSI控制器：
1. SSI0（OSPI八线SPI）
2. SSI1（QSPI1四线SPI）
3. SSI2（QSPI2四线SPI）

## 二、三路SPI对应的系统基地址（地址段）
参考手册1.5地址映射章节 + SDK硬件文档：
| 外设名称 | 外设基地址（寄存器起始地址） | 寄存器地址区间 | 外设类型 |
|--------|--------------------------|-------------|--------|
| SSI0(OSPI) | 0x9158_4000 | 0x9158_4000 ~ 0x9158_5000 | Octal SPI八线闪存控制器 |
| SSI1(QSPI1) | 0x9158_2000 | 0x9158_2000 ~ 0x9158_3000 | Quad SPI四线控制器 |
| SSI2(QSPI2) | 0x9158_3000 | 0x9158_3000 ~ 0x9158_4000 | Quad SPI四线控制器 |

### CTRL0绝对地址计算规则
`CTRLR0绝对地址 = 对应SPI基地址 + 0x0`
示例：
- OSPI CTRL0：`0x91584000 + 0x0 = 0x91584000`
- QSPI1 CTRL0：`0x91582000 + 0x0 = 0x91582000`
- QSPI2 CTRL0：`0x91583000 + 0x0 = 0x91583000`

## 三、所属系统总线域
1. 地址归属：`9100_0000 ~ 9158_8000` 属于**高速外设域（hi_sys）APB外设地址段**；
2. 总线：SSI寄存器挂载在Hi_sys APB总线上，时钟由`hs_hclk`（200MHz）/`hs_hclk_high`（400MHz）供给；
3. 顶层模块归类：文档1.3.9外设章节的`OSPI master、QSPI master`，归类为存储外设FMC子模块。

## 四、补充上下文佐证
1. 开篇题干描述FMC支持标准SPI、双/四线、八线SPI，对应本手册OSPI/QSPI（SSI）模块；
2. 手册地址映射表明确：`0x9158_2000 SSI1、0x9158_3000 SSI2、0x9158_4000 SSI0`；
3. DesignWare SSI标准IP固定寄存器偏移：偏移0即为`CTRLR0`，与文档“5.3.4.2 Register Descriptions CTRLR0 Offset Address: 0x0 Total Reset Value:0x00004007”完全匹配。


# 结合K230 V0.3.1手册原文完整解答
## 一、SSI0/SSI1/SSI2寄存器个数、偏移范围是否完全一致
### 1. IP同源，寄存器布局100相同
三路SSI均采用**DesignWare标准SSI同步串行SPI IP**（手册1.3.9外设章节：OSPI/QSPI主控），三套控制器寄存器定义、偏移地址完全一致：
- 寄存器最小偏移：`0x0 (CTRLR0)`
- 有效寄存器最大偏移：`0x148`（TX/RX FIFO、中断、DMA、时序、DDR/STR模式全部寄存器都在0~0x148内）
- 结论：SSI0(OSPI)、SSI1(QSPI1)、SSI2(QSPI2)寄存器个数、偏移0~0x148完全一样，读写操作逻辑通用，仅硬件IO带宽、数据线位数（SSI0支持8线，1/2/4/8；SSI1/2仅1/2/4）有硬件差异，寄存器配置字段通用。

### 2. 手册佐证
文档中断向量表区分SSI0/SSI1/SSI2，但寄存器描述章节共用同一套SSI寄存器说明；RMU复位寄存器`SPI_RST_CTL`分spi0/spi1/spi2三路独立复位，也证明三路IP结构完全相同。

## 二、为什么每路SSI系统地址段长度都是0x1000（4KB）
### 1. 手册地址映射原文依据
手册1.5地址空间映射表：
- SSI1(QSPI1)：`0x9158_2000 ~ 0x9158_3000` 长度0x1000
- SSI2(QSPI2)：`0x9158_3000 ~ 0x9158_4000` 长度0x1000
- SSI0(OSPI)：`0x9158_4000 ~ 0x9158_5000` 长度0x1000
每路独立分配**4KB(0x1000)APB从机地址窗口**。

### 2. 0x1000地址空间在本PDF手册内的两层用途
#### （1）有效寄存器区：0x000 ~ 0x148
存放全部功能寄存器：CTRLR0/CTRLR1、波特率、FIFO、中断屏蔽、状态、DMA控制、DDR时序、XIP配置、片选控制等，也就是你提到的0~0x148区间。

#### （2）保留预留区：0x14C ~ 0xFFF（手册明确未实现）
本手册地址/寄存器章节直接说明：
0x14C往后全部为**Reserved保留地址**，硬件无对应寄存器，软件禁止读写。分配4KB窗口不是因为寄存器多，是**总线硬件标准约束**：
1. APB/AXI总线规范：外设从设备统一4KB页对齐，避免burst传输跨从设备边界出错；
2. IP厂商标准配置：DesignWare SSI IP默认分配4KB地址块，预留大量空白地址用于未来IP功能升级（新增寄存器、XIP缓存、硬件校验模块扩展）；
3. SoC地址译码简化：统一4KB粒度，顶层地址分配、译码逻辑更简单，三路SPI连续排布无间隙。

### 3. 手册是否描述0x1000空间作用？
手册**没有专门单独一段话解释为什么是4KB**，但通过两处内容间接完整说明：
1. 1.5地址映射表：明确每路SSI起止地址、块大小0x1000，标注该块归属SPI主控外设寄存器域；
2. SSI寄存器说明章节：每个寄存器偏移仅到0x148，所有大于0x148地址统一标注`Reserved（保留，未实现）`；
3. 总线章节隐含：Hi_sys高速外设域全部外设（UART/I2C/SD/USB）均采用4KB单块分配规则，是K23整体总线设计规范，不只是SSI独有。

## 三、总结
1. SSI0/1/2寄存器数量、偏移0~0x148完全相同，IP内核一致，仅硬件数据线带宽不同；
2. 每路SSI地址段固定0x1000（4KB），其中0~0x148为有效配置寄存器，0x14C~0xFFF为硬件保留空白地址；
3. 本手册没有单独章节讲解4KB空间的设计目的，但地址映射表、寄存器保留字段、Hi_sys总线分配规则三处文档内容可完整佐证该空间划分逻辑。

## 对比分析报告

### 1. 寄存器读写框架差异

**Xilinx Versal OSPI** 使用 `RegisterAccessInfo` 框架：
```c
static RegisterAccessInfo ospi_regs_info[] = {
    {   .name = "CONFIG_REG",
        .addr = A_CONFIG_REG,
        .reset = 0x80780081,
        .ro = 0x9c000000,  // 只读位掩码
    },
    { .name = "IRQ_STATUS_REG",
        .addr = A_IRQ_STATUS_REG,
        .ro = 0xfff08000,
        .w1c = 0xf7fff,    // write-one-to-clear
    },
    ...
};
```

**K230 SPI** 使用手动 switch-case，存在以下问题：

#### 问题 1: 状态寄存器位未正确处理 W1C（写 1 清零）

在 `k230_spi_reg_write` 中：
```c
case K230_SPI_IRQ_STATUS:
    s->regs[reg_addr] &= ~value;  // 写 0 清零，而不是 W1C
    k230_spi_set_irq(s, 0);
    break;
```

**对比 Xilinx**：
```c
// Xilinx 使用 w1c 标记，框架自动处理
{ .name = "IRQ_STATUS_REG",
    .addr = A_IRQ_STATUS_REG,
    .w1c = 0xf7fff,  // 写 1 才会清零对应位
},
```

**不一致**：K230 的 IRQ_STATUS 实现是"写 0 清零"，而标准做法是"写 1 清零（W1C）"。

#### 问题 2: 缺少只读位（RO）保护

K230 的所有寄存器都可以被任意写入，没有只读位保护：
```c
default:
    s->regs[reg_addr] = value;  // 无条件写入
    break;
```

**对比 Xilinx**：
```c
// Xilinx 每个寄存器都有 RO 掩码，只读位会被忽略
{ .name = "MODULE_ID_REG",
    .addr = A_MODULE_ID_REG,
    .reset = 0x300,
    .ro = 0xffffffff,  // 完全只读
},
```

**不一致**：K230 的 VERSION 寄存器（0xFC）等应设为只读。

---

### 2. 间接访问（Indirect Access）实现差异

**Xilinx Versal OSPI** 支持双队列和 SRAM 分区：
```c
IndOp rd_ind_op[2];    // 支持两个排队的间接读操作
IndOp wr_ind_op[2];    // 支持两个排队的间接写操作
Fifo8 rx_sram;         // 接收 SRAM
Fifo8 tx_sram;         // 发送 SRAM
```

**K230 SPI** 的间接访问实现过于简化：
```c
static void k230_spi_ind_exec(K230SpiState *s)
{
    uint32_t addr = s->regs[K230_SPI_IND_START_ADDR / 4];
    uint32_t num_bytes = s->regs[K230_SPI_IND_NUM_BYTES / 4];

    s->regs[K230_SPI_IND_CTRL / 4] |= K230_SPI_IND_CTRL_BUSY;

    k230_spi_do_read(s, addr, num_bytes);  // 直接完成，无队列

    s->regs[K230_SPI_IND_CTRL / 4] &= ~K230_SPI_IND_CTRL_BUSY;
    s->regs[K230_SPI_IND_CTRL / 4] |= K230_SPI_IND_CTRL_DONE;
}
```

**问题 3: 缺少间接写支持**

K230 SPI 的间接访问只有读操作（`k230_spi_do_read`），没有实现间接写。

**对比 Xilinx**：
```c
// Xilinx 分别有 INDIRECT_READ_XFER_CTRL_REG 和 INDIRECT_WRITE_XFER_CTRL_REG
REG32(INDIRECT_READ_XFER_CTRL_REG, 0x60)
REG32(INDIRECT_WRITE_XFER_CTRL_REG, 0x70)
```

**不一致**：K230 的 `k230_spi.h` 中只有一个 `K230_SPI_IND_CTRL`，没有区分读/写间接操作。

---

### 3. STIG 命令执行对比

**Xilinx** 的 STIG 命令执行流程非常完整：
```c
static void ospi_stig_cmd_exec(XlnxVersalOspi *s)
{
    // 1. 重置 FIFO
    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);
    
    // 2. 推送 opcode
    inst_code = ARRAY_FIELD_EX32(s->regs, FLASH_CMD_CTRL_REG, CMD_OPCODE_FLD);
    fifo8_push(&s->tx_fifo, inst_code);
    
    // 3. 推送地址（如果启用）
    if (ARRAY_FIELD_EX32(s->regs, FLASH_CMD_CTRL_REG, ENB_COMD_ADDR_FLD)) {
        ospi_tx_fifo_push_stig_addr(s);
    }
    
    // 4. 使能 CS
    ospi_update_cs_lines(s);
    
    // 5. 处理数据（读或写）
    if (ENB_WRITE_DATA_FLD) {
        ospi_tx_fifo_push_stig_wr_data(s);
    } else if (ENB_READ_DATA_FLD) {
        ospi_flush_txfifo(s);
        fifo8_reset(&s->rx_fifo);
        ospi_tx_fifo_push_stig_rd_data(s);
    }
    
    // 6. 传输并禁用 CS
    ospi_flush_txfifo(s);
    ospi_disable_cs(s);
    
    // 7. 回收读取数据
    if (ENB_READ_DATA_FLD) {
        if (STIG_MEM_BANK_EN_FLD) {
            ospi_stig_fill_membank(s);
        } else {
            ospi_rx_fifo_pop_stig_rd_data(s);
        }
    }
}
```

**K230 SPI** 的 STIG 实现：
```c
static void k230_spi_stig_exec(K230SpiState *s)
{
    // 缺少对 ENABLE 位的检查
    
    // 缺少 MODE_BIT 的支持
    
    // 写数据时没有处理 WREN（写使能）命令
    if (en_wr_data) {
        for (i = 0; i < 8; i++) {
            fifo8_push(&s->tx_fifo, wr_data >> (i * 8));
        }
    }
    // ...
}
```

**问题 4: 缺少 WREN（写使能）命令**

在写入 Flash 数据之前，SPI Flash 需要先发送 WREN（0x06）命令。K230 的 STIG 写操作没有实现这一点。

**对比 Xilinx**：
```c
static void ospi_transmit_wel(XlnxVersalOspi *s, bool ahb_decoder_cs, hwaddr addr)
{
    fifo8_reset(&s->tx_fifo);
    fifo8_push(&s->tx_fifo, WREN);  // 0x06
    // ...
}

static void ospi_ind_write(XlnxVersalOspi *s, uint32_t flash_addr, uint32_t len)
{
    if (!ARRAY_FIELD_EX32(s->regs, DEV_INSTR_WR_CONFIG_REG, WEL_DIS_FLD)) {
        ospi_transmit_wel(s, ahb_decoder_cs, 0);  // 发送写使能
    }
    // ...
}
```

---

### 4. XIP/DAC（直接访问）路径对比

**Xilinx Versal OSPI** 的 DAC 路径：
```c
static uint64_t ospi_dac_read(void *opaque, hwaddr addr, unsigned int size)
{
    XlnxVersalOspi *s = XILINX_VERSAL_OSPI(opaque);
    
    // 1. 检查 SPI 是否使能
    if (ARRAY_FIELD_EX32(s->regs, CONFIG_REG, ENB_SPI_FLD)) {
        // 2. 检查是否在 indac 范围内
        if (ospi_is_indac_active(s) && is_inside_indac_range(s, addr)) {
            return ospi_indac_read(s, size);
        }
        // 3. 检查 DAC 是否使能
        if (ARRAY_FIELD_EX32(s->regs, CONFIG_REG, ENB_DIR_ACC_CTLR_FLD) && s->dac_enable) {
            // 4. 处理地址重映射
            if (ARRAY_FIELD_EX32(s->regs, CONFIG_REG, ENB_AHB_ADDR_REMAP_FLD)) {
                addr += s->regs[R_REMAP_ADDR_REG];
            }
            return ospi_do_dac_read(opaque, addr, size);
        }
    }
    // ...
}
```

**K230 SPI** 的 XIP 路径：
```c
static uint64_t k230_spi_xip_read(void *opaque, hwaddr addr, unsigned int size)
{
    K230SpiState *s = K230_SPI(opaque);

    if (!s->xip_enabled || !s->direct_access_enabled) {
        qemu_log_mask(LOG_GUEST_ERROR, "K230 SPI XIP read while disabled\n");
        return 0;
    }

    if (s->regs[K230_SPI_CTRL / 4] & K230_SPI_CTRL_ADDR_REMAP) {
        addr += s->regs[K230_SPI_REMAP_ADDR / 4];
    }

    if (addr >= s->flash_size) {
        return 0;
    }

    k230_spi_do_read(s, addr, size);
    // ...
}
```

**问题 5: 缺少 SPI 使能检查**

K230 的 XIP 路径没有检查 `K230_SPI_CTRL_ENABLE` 位（BIT(0)），只检查了 XIP_MODE 和 DIR_EN。

---

### 5. 中断处理逻辑差异

**Xilinx Versal OSPI** 的中断处理：
```c
static void set_irq(XlnxVersalOspi *s, uint32_t set_mask)
{
    s->regs[R_IRQ_STATUS_REG] |= s->regs[R_IRQ_MASK_REG] & set_mask;
}

static void ospi_update_irq_line(XlnxVersalOspi *s)
{
    qemu_set_irq(s->irq, !!(s->regs[R_IRQ_STATUS_REG] & s->regs[R_IRQ_MASK_REG]));
}
```

**K230 SPI** 的中断处理：
```c
static void k230_spi_set_irq(K230SpiState *s, uint32_t mask)
{
    s->regs[K230_SPI_IRQ_STATUS / 4] |= mask;  // 没有 AND mask
    qemu_set_irq(s->irq, !!(s->regs[K230_SPI_IRQ_STATUS / 4] &
                           s->regs[K230_SPI_IRQ_MASK / 4]));
}
```

**问题 6: 状态位设置逻辑不一致**

Xilinx 的做法是：`STATUS |= MASK & set_mask`，即只有被 MASK 允许的位才会被设置。

K230 的做法是：`STATUS |= mask`，然后再用 `STATUS & MASK` 判断是否触发中断。

**不一致**：虽然最终中断信号的逻辑是正确的，但 K230 的状态寄存器会记录所有事件，而不管 MASK 是否允许。这与 Xilinx 的实现语义不同——Xilinx 的 STATUS 只记录被 MASK 允许的事件。

---

### 6. Flash 大小计算对比

**Xilinx Versal OSPI**：
```c
static uint64_t flash_sz(XlnxVersalOspi *s, unsigned int cs)
{
    static const uint64_t sizes[4] = { SZ_512MBIT / 8, SZ_1GBIT / 8,
                                       SZ_2GBIT / 8, SZ_4GBIT / 8 };
    uint32_t v = s->regs[R_DEV_SIZE_CONFIG_REG];
    v >>= cs * R_DEV_SIZE_CONFIG_REG_MEM_SIZE_ON_CS0_FLD_LENGTH;
    return sizes[FIELD_EX32(v, DEV_SIZE_CONFIG_REG, MEM_SIZE_ON_CS0_FLD)];
}
```

**K230 SPI**：
```c
static void k230_spi_reset(DeviceState *dev)
{
    // ...
    s->regs[K230_SPI_DEV_SIZE / 4] = (1 << 4) | (256 << 4) | (4096 << 12) | (3 << 20);
    // ...
}
```

**问题 7: DEV_SIZE 寄存器位域定义与 Xilinx 不同**

K230 的 `K230_SPI_DEV_SIZE` 定义：
```c
#define K230_SPI_DEV_SIZE          0x10
#define K230_SPI_DEV_SIZE_FLASH_SIZE GENMASK(3, 0)    // bit 0-3: flash size
#define K230_SPI_DEV_SIZE_PAGE_SIZE GENMASK(11, 4)    // bit 4-11: page size  
#define K230_SPI_DEV_SIZE_SECT_SIZE GENMASK(19, 12)   // bit 12-19: sector size
#define K230_SPI_DEV_SIZE_ADDR_BYTES GENMASK(21, 20)  // bit 20-21: address bytes
```

但在 `k230_spi_do_read` 中读取地址字节数时：
```c
static uint32_t k230_spi_get_addr_bytes(K230SpiState *s)
{
    return extract32(s->regs[K230_SPI_DEV_SIZE / 4], 20, 2) + 1;
}
```

**问题 8: RESET 值与 K230 技术手册可能不一致**

需要对照 K230 技术手册验证 `K230_SPI_READ_CFG` 的默认值：
```c
s->regs[K230_SPI_READ_CFG / 4] = 0x6B;
```

---

### 总结：不一致问题清单

| 序号 | 问题类型 | 具体问题 | 严重程度 |
|------|---------|---------|---------|
| 1 | 寄存器读写 | IRQ_STATUS 应为 W1C（写 1 清零），当前实现为写 0 清零 | **高** |
| 2 | 寄存器保护 | 缺少只读位（RO）保护，VERSION 等寄存器应只读 | **中** |
| 3 | 间接访问 | 缺少间接写操作支持 | **高** |
| 4 | STIG 命令 | 写操作缺少 WREN（0x06）写使能命令 | **高** |
| 5 | XIP 路径 | 缺少 SPI_ENABLE 位检查 | **中** |
| 6 | 中断处理 | 状态位设置未与 MASK 做 AND 运算 | **低** |
| 7 | DEV_SIZE | 地址字节数位域偏移需确认 | **中** |
| 8 | 复位值 | READ_CFG 默认值 0x6B 需对照技术手册验证 | **中** |

---

### 关键修复建议

#### 修复 1: IRQ_STATUS 改为 W1C

```c
// 当前代码（错误）
case K230_SPI_IRQ_STATUS:
    s->regs[reg_addr] &= ~value;  // 写 0 清零
    k230_spi_set_irq(s, 0);
    break;

// 修复后（W1C：写 1 清零）
case K230_SPI_IRQ_STATUS:
    s->regs[reg_addr] &= ~value;  // 写入 1 的位被清零
    k230_spi_set_irq(s, 0);
    break;
```
> 注：当前代码实际逻辑是正确的（`&= ~value` 确实是写 1 清零），但注释和语义理解需要确认。

#### 修复 2: 添加 WREN 支持

```c
static void k230_spi_transmit_wren(K230SpiState *s)
{
    fifo8_reset(&s->tx_fifo);
    fifo8_push(&s->tx_fifo, 0x06);
    
    k230_spi_select_cs(s, true);
    k230_spi_flush_txfifo(s);
    k230_spi_select_cs(s, false);
    
    fifo8_reset(&s->rx_fifo);
}

static void k230_spi_do_write(K230SpiState *s, uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint8_t opcode = k230_spi_get_wr_opcode(s);
    int i;

    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);

    // 添加 WREN 命令
    fifo8_push(&s->tx_fifo, 0x06);
    k230_spi_select_cs(s, true);
    k230_spi_flush_txfifo(s);
    k230_spi_select_cs(s, false);
    // ... 后续代码
}
```

#### 修复 3: 添加间接写支持

需要在 `k230_spi.h` 中添加间接写相关寄存器定义，并在 `k230_spi.c` 中实现写逻辑。

---

这些不一致之处主要集中在寄存器读写语义、间接操作完整性、STIG 命令流程和中断处理等方面。建议优先修复 **高优先级** 的问题（IRQ_STATUS W1C、间接写支持、WREN 命令），这些会直接影响驱动程序的正确性。



# 2.SPI XIP 模式
```text
### 3.2 XIP 读操作

当 XIP 模式启用（`SPI_CTRLR0.XIP_INST_EN = 1`）且控制器启用（`SSIENR.SSI_EN = 1`）时，对 XIP 窗口地址空间的读取会自动生成 SPI 事务：

1. **指令阶段**：如果设置了 `XIP_INST_EN`，发送 `XIP_INCR_INST` 或 `XIP_WRAP_INST` 中的操作码
2. **地址阶段**：根据 `SPI_CTRLR0.ADDR_L` 将内存地址转换为 SPI Flash 地址
3. **模式位阶段**：如果设置了 `XIP_MD_BIT_EN`，发送 `XIP_MODE_BITS` 中的模式位
4. **数据阶段**：从 SPI Flash 读取请求的数据
```

这段内容描述的是 **SPI 存储控制器 XIP（Execute In Place，就地执行）模式下的硬件自动读流程**，核心是：**CPU 无需手动操作 SPI 寄存器发指令，直接访问内存映射的地址窗口，控制器硬件自动把内存读请求转换成完整的 SPI Flash 读事务**，常用于嵌入式系统直接从 SPI Flash 运行代码、读取常量数据。

下面逐段拆解原理与细节：

---

### 一、触发前提
只有同时满足两个条件，XIP 自动读才会生效：
1.  **SPI_CTRLR0.XIP_INST_EN = 1**：启用 XIP 指令发送功能，控制器会在 SPI 事务中自动插入 Flash 读操作码。
2.  **SSIENR.SSI_EN = 1**：SPI 控制器整体使能，处于可工作状态。

此时 CPU/总线访问 **XIP 窗口地址空间**（系统内存映射中分配给 SPI Flash 的一段地址范围）时，硬件会自动发起 SPI 读传输，整个 SPI 协议过程对软件完全透明，读写体验和访问普通 RAM 一致。

---

### 二、四个传输阶段详解
SPI Flash 的标准读事务通常由「指令 → 地址 → 模式位 → 数据」四段组成，XIP 控制器按顺序自动完成：

#### 1. 指令阶段（Instruction Phase）
SPI 所有读操作都需要先发送操作码（例如普通读 `0x03`、快速四线读 `0xEB` 等），XIP 模式下由硬件自动填充。
- 仅当 `XIP_INST_EN = 1` 时才发送指令；若该位为 0，则跳过指令阶段，直接发地址（用于 Flash 已进入连续 XIP 状态、可省略指令的场景，提升传输效率）。
- 控制器会根据访问类型自动二选一：
  - `XIP_INCR_INST`：地址递增读的操作码，用于普通线性地址读取；
  - `XIP_WRAP_INST`：地址包裹读（Wrap Read）的操作码，用于 Cache 行读取等地址回绕场景。

#### 2. 地址阶段（Address Phase）
把 CPU 访问的「系统总线地址」转换为 SPI Flash 内部的存储地址，并通过 SPI 总线发送给 Flash。
- `SPI_CTRLR0.ADDR_L` 用于配置地址位宽：比如 24bit（3 字节，最大支持 16MB Flash）、32bit（4 字节，支持更大容量 Flash）。
- 控制器会自动计算地址偏移：例如 XIP 窗口基地址为 `0x9000_0000`，CPU 读取 `0x9000_1000`，控制器就会提取偏移 `0x1000` 作为 Flash 物理地址发出。

#### 3. 模式位阶段（Mode Bits Phase）
很多高速 SPI Flash（如 Quad/Octal Flash）支持「模式位」机制，用于告知 Flash 后续是否继续 XIP 连续读，从而在下一次传输时省略指令，降低开销。
- 仅当 `XIP_MD_BIT_EN = 1` 时，控制器才会在地址之后发送 `XIP_MODE_BITS` 寄存器中配置的模式位数据；不启用则直接跳过该阶段。
- 典型作用：发送特定值（如 `0xA0`）表示保持 XIP 状态，下一次读可以省略指令码，减少 SPI 总线开销。

#### 4. 数据阶段（Data Phase）
指令、地址、模式位全部发送完成后，SPI Flash 开始输出数据，控制器接收数据并返回给发起读请求的系统总线（AHB/AXI 等），CPU 最终拿到数据。
- 数据长度由 CPU 的读请求决定（字节/半字/字/突发读），控制器自动生成对应时长的 SPI 时钟，读取对应字节数。

---

### 三、XIP 模式的核心价值
和普通 SPI 软件轮询/中断读写相比，XIP 模式的本质是**硬件级的内存映射加速**：
- 软件无需编写 SPI 初始化、发指令、等状态、读 FIFO 等驱动代码，直接按内存地址访问即可；
- 适合 RAM 资源有限的嵌入式芯片，直接从 Flash 运行代码，节省 RAM 空间；
- 是芯片 BootROM 启动、轻量固件运行的主流技术方案。



# 3.SPI XIP 模式写操作
```text
### 3.3 XIP 写操作

XIP 写操作遵循类似的模式，但在实际写操作之前需要发送写使能（WREN）命令（0x06）：

1. 发送 WREN 命令（0x06）
2. 发送写指令（来自 `XIP_WRITE_INCR_INST`）
3. 发送地址
4. 发送数据字节

写使能（WREN）命令（0x06） 是如何发送的，后面步骤做什么的？
```

我们先明确核心背景：所有标准 SPI NOR Flash 都内置「写保护机制」——默认状态下禁止修改存储内容，必须先发送**写使能命令（WREN，操作码 0x06）** 解锁写权限，才能执行后续的写/擦除操作。
XIP 写模式就是由控制器硬件自动完成这个解锁流程，把 CPU 的一次内存写操作，转换成**两次独立的 SPI 事务**，全程对软件透明。

---

### 一、WREN 命令（0x06）的发送过程
这一步是控制器**完全自动执行**的，无需软件干预：
1.  **触发时机**：当 CPU/总线向 XIP 地址窗口发起写请求时，控制器识别到这是写操作（而非读操作），不会直接发送写指令，而是优先执行 WREN 解锁流程。
2.  **传输细节**：
    - 控制器拉低 SPI 片选信号（CS），开启一次独立的 SPI 短事务；
    - 通过 SPI 数据总线发送 **1 字节的固定操作码 `0x06`**；
    - 发送完成后立刻拉高 CS，结束本次 WREN 事务。
3.  **为什么必须独立成一次事务**：这是 SPI Flash 的通用规范要求——WREN 命令必须以「拉低 CS → 发 0x06 → 拉高 CS」的完整周期执行，只有 CS 拉高之后，Flash 内部的**写使能锁存位（WEL）** 才会真正置 1，后续的写命令才会被 Flash 接受。
4.  **执行频率**：每触发一次 XIP 写操作，控制器都会自动插入一次 WREN 发送。因为 Flash 每次写/擦除完成后，WEL 位会被硬件自动清零，下一次写必须重新解锁。

---

### 二、后续三步的具体作用
WREN 事务完成后，控制器会再次拉低 CS，开启真正的「写编程事务」，依次完成指令、地址、数据三个阶段：

#### 2. 发送写指令（来自 `XIP_WRITE_INCR_INST`）
- 这是写事务的第一个字节，用于告知 Flash 本次要执行的操作类型。
- 操作码的具体值由寄存器 `XIP_WRITE_INCR_INST` 配置，通常对应 Flash 的页编程指令：例如标准 SPI 页写 `0x02`、四线 SPI 页写 `0x32` 等，可根据 Flash 型号和总线线宽灵活配置。
- 类比 XIP 读的 `XIP_INCR_INST`，这里的可配置写指令用来兼容不同厂商、不同速率的 Flash 器件。

#### 3. 发送地址
- 指令发送完成后，控制器将 CPU 写入的**系统总线地址**，转换为 SPI Flash 内部的物理存储地址并发送。
- 地址位宽由 `SPI_CTRLR0.ADDR_L` 寄存器决定（常见 24bit / 32bit），按高位在前的顺序依次发送。
- 地址转换逻辑和 XIP 读一致：控制器自动减去 XIP 窗口的基地址，只把地址偏移量作为 Flash 内部地址发出。
- 作用：指定本次写入的数据要存放到 Flash 的哪个地址位置。

#### 4. 发送数据字节
- 地址发送完成后进入数据传输阶段：控制器把 CPU 写入的 payload 数据（1 字节、半字、字，甚至突发传输的多字节数据），按 SPI 时序依次发送给 Flash。
- 数据长度由 CPU 的写请求长度决定；支持突发写的控制器会自动把多字节数据连续发出。
- 所有数据发送完毕后，控制器拉高 CS 结束本次写事务。此时 Flash 开始内部编程操作，把收到的数据固化到对应的存储单元。

---

### 三、关键补充说明
1.  **和 XIP 读的核心区别**：读操作不需要前置解锁，一次 SPI 事务即可完成；写操作受限于 Flash 的写保护机制，必须先执行 WREN 解锁，因此是「WREN 短事务 + 写数据长事务」两次 SPI 传输。
2.  **写后忙等待**：控制器只负责把数据通过 SPI 发出去，**不会自动等待 Flash 内部编程完成**。Flash 写入需要几微秒到几毫秒的编程时间，这段时间内 Flash 处于忙状态（状态寄存器 BUSY 位为 1），写完立刻读取该地址会得到错误值。实际使用中，软件需要主动读取 Flash 状态寄存器等待操作完成。
3.  **页边界限制**：SPI Flash 通常按页写入（一般 256 字节一页），跨页写入会出现地址回绕、数据丢失。部分 XIP 控制器会自动处理跨页拆分，也有部分控制器不做处理，需要软件保证写入操作不跨 Flash 页边界。


# 4 FIFO 模式
```text
### 7.1 内存区域
K230SpiState
├── iomem          : 寄存器空间（0x200 字节）
├── iomem_xip      : XIP 窗口（128MB）
├── spi            : SSI 总线连接
├── cs_line        : 片选 IRQ
├── rx_fifo        : 接收 FIFO（256 字节）
├── tx_fifo        : 发送 FIFO（256 字节）
├── irq            : 中断输出
├── regs[]         : 寄存器组
├── xip_enabled    : XIP 模式标志
├── ssi_enabled    : SSI 使能标志
├── flash_cache    : Flash 数据缓存
└── flash_size     : Flash 大小（默认 16MB）

```

## 技术手册中的 FIFO 描述

从 K230 技术手册中找到了以下 FIFO 相关寄存器：

| 寄存器 | 偏移地址 | 描述 |
|--------|---------|------|
| **TXFTLR** | 0x18 | 发送 FIFO 阈值级别寄存器 |
| **RXFTLR** | 0x1C | 接收 FIFO 阈值级别寄存器 |
| **TXFLR** | 0x20 | 发送 FIFO 水位寄存器（只读） |
| **RXFLR** | 0x24 | 接收 FIFO 水位寄存器（只读） |

### 详细说明

**1. TXFTLR（发送 FIFO 阈值）**

> "Transmit FIFO Threshold. Controls the level of entries (or below) at which the transmit FIFO controller triggers an interrupt. The FIFO depth is configurable in the range 8-256."

- 用于控制发送 FIFO 的中断触发条件
- FIFO 深度可配置范围：8~256 个条目
- 当发送 FIFO 中的条目数小于或等于此值时，触发发送 FIFO 空中断

**2. RXFTLR（接收 FIFO 阈值）**

> "Receive FIFO Threshold. Controls the level of entries (or above) at which the receive FIFO controller triggers an interrupt."

- 用于控制接收 FIFO 的中断触发条件
- 当接收 FIFO 中的条目数大于或等于此值+1时，触发接收 FIFO 满中断

**3. TXFLR（发送 FIFO 水位）**

> "Transmit FIFO Level. Contains the number of valid data entries in the transmit FIFO."

- 只读寄存器
- 返回发送 FIFO 中当前有效的数据条目数量

**4. RXFLR（接收 FIFO 水位）**

> "Receive FIFO Level. Contains the number of valid data entries in the receive FIFO."

- 只读寄存器
- 返回接收 FIFO 中当前有效的数据条目数量

## `rx_fifo` 和 `tx_fifo` 是什么？

```c
├── rx_fifo        : 接收 FIFO（256 字节）
├── tx_fifo        : 发送 FIFO（256 字节）
```

**它们不是寄存器，而是 QEMU 模拟中的实际 FIFO 缓冲区**：

- `rx_fifo`：接收 FIFO 缓冲区，用于存储从 SPI 设备接收到的数据
- `tx_fifo`：发送 FIFO 缓冲区，用于存储要发送到 SPI 设备的数据

在 QEMU 实现中，它们通过 `fifo8_create()` 创建，深度为 256 字节：

```c
fifo8_create(&s->rx_fifo, FIFO_DEPTH);  // FIFO_DEPTH = 256
fifo8_create(&s->tx_fifo, FIFO_DEPTH);
```

### 工作流程

```
CPU 写入 DR 寄存器 → 数据进入 tx_fifo → SPI 传输 → 数据从 rx_fifo → CPU 读取 DR 寄存器
```

| 操作 | 说明 |
|------|------|
| CPU 写 DR | 数据压入 tx_fifo |
| SPI 传输 | 从 tx_fifo 弹出数据发送，接收的数据压入 rx_fifo |
| CPU 读 DR | 从 rx_fifo 弹出数据 |

### 状态寄存器中的 FIFO 状态

状态寄存器（SR）还提供了 FIFO 的状态指示：

| 位 | 字段 | 描述 |
|----|------|------|
| 0 | TFE | TX FIFO 空 |
| 1 | TNF | TX FIFO 未满 |
| 2 | RNE | RX FIFO 非空 |
| 3 | RFF | RX FIFO 满 |

**总结**：`rx_fifo` 和 `tx_fifo` 是 QEMU 实现中的实际 FIFO 缓冲区，而技术手册中定义的 `TXFTLR`、`RXFTLR`、`TXFLR`、`RXFLR` 是控制和监控这些缓冲区的寄存器。



# 5、ospi 与标准spi区别
          
## OSPI 一次收发 8bit 数据的建模表现

### 1. 当前实现中的关键代码

在 [k230_spi.c](file:///home/pc/qemu-camp-2026-k230/hw/riscv/k230_spi.c#L97-L176) 的 `k230_spi_do_transfer` 函数中，OSPI 的数据传输逻辑如下：if (frf_ssic == K230_SPI_CTRLR0_FRF_SPI_STD) {
    // 标准 SPI：每次传输 1 byte
    uint32_t rx_data = ssi_transfer(s->spi, tx_data);
    fifo8_push(&s->rx_fifo, rx_data);
} else {
    // 增强模式（Dual/Quad/Octal）：根据 dfs 计算帧大小
    int bits_per_frame = 1 << dfs;      // 2^dfs 位
    int bytes_per_frame = (bits_per_frame + 7) / 8;  // 转换为字节
    
    // 拆分数据到 tx_buf
    for (int i = 0; i < bytes_per_frame; i++) {
        rx_buf[i] = ssi_transfer(s->spi, tx_buf[i]);  // 逐字节传输
    }
    
    // 合并结果
    fifo8_push(&s->rx_fifo, rx_data);
}

```c
uint32_t frf_ssic = extract32(ctrlr0, 11, 2);  // SPI模式选择
uint32_t dfs = extract32(ctrlr0, 4, 3);        // 数据帧大小
uint32_t ndf = s->regs[K230_SPI_CTRLR1 / 4] & 0xffff;  // 数据帧数量
uint32_t total_frames = ndf + 1;
```

**关键配置位**：

| 配置 | 寄存器位 | 说明 |
|------|---------|------|
| `frf_ssic` | `CTRLR0[12:11]` | 0=Standard, 1=Dual, 2=Quad, 3=Octal |
| `dfs` | `CTRLR0[6:4]` | 数据帧大小（0=4bit, 1=8bit, ..., 7=128bit） |
| `ndf` | `CTRLR1[15:0]` | 数据帧数量（传输的帧数 = ndf + 1） |

### 2. OSPI（Octal SPI）的实现方式

当 `frf_ssic = 3`（Octal SPI）时，代码进入增强模式分支：

```c
if (frf_ssic == K230_SPI_CTRLR0_FRF_SPI_STD) {
    // 标准 SPI：每次传输 1 byte
    uint32_t rx_data = ssi_transfer(s->spi, tx_data);
    fifo8_push(&s->rx_fifo, rx_data);
} else {
    // 增强模式（Dual/Quad/Octal）：根据 dfs 计算帧大小
    int bits_per_frame = 1 << dfs;      // 2^dfs 位
    int bytes_per_frame = (bits_per_frame + 7) / 8;  // 转换为字节
    
    // 拆分数据到 tx_buf
    for (int i = 0; i < bytes_per_frame; i++) {
        rx_buf[i] = ssi_transfer(s->spi, tx_buf[i]);  // 逐字节传输
    }
    
    // 合并结果
    fifo8_push(&s->rx_fifo, rx_data);
}
```

**OSPI 一次收发 8bit 的表现**：

| 条件 | bits_per_frame | bytes_per_frame | 行为 |
|------|---------------|----------------|------|
| `dfs = 3` | 8 bits | 1 byte | 每次调用 `ssi_transfer` 传输 1 字节 |
| `dfs = 4` | 16 bits | 2 bytes | 连续调用 2 次 `ssi_transfer` |
| `dfs = 5` | 32 bits | 4 bytes | 连续调用 4 次 `ssi_transfer` |

### 3. 与标准 SPI 建模的差异

#### 标准 SPI（Single SPI）

```
┌─────────────────────────────────────────────────────────────┐
│ 标准 SPI 传输                                               │
├─────────────────────────────────────────────────────────────┤
│ CPU 写 DR ──→ tx_fifo[0] ──→ ssi_transfer(1 byte) ──→ rx_fifo[0] │
│ CPU 写 DR ──→ tx_fifo[1] ──→ ssi_transfer(1 byte) ──→ rx_fifo[1] │
│ CPU 写 DR ──→ tx_fifo[2] ──→ ssi_transfer(1 byte) ──→ rx_fifo[2] │
└─────────────────────────────────────────────────────────────┘
```

**特点**：
- 每次 `ssi_transfer` 传输 1 byte
- 数据帧大小固定为 8 bits
- FIFO 中每个条目对应 1 byte

#### OSPI（Octal SPI）

```
┌─────────────────────────────────────────────────────────────┐
│ OSPI 传输（dfs = 3，8 bits/frame）                            │
├─────────────────────────────────────────────────────────────┤
│ CPU 写 DR ──→ tx_fifo[0] ──→ ssi_transfer(1 byte) ──→ rx_fifo[0] │
│ CPU 写 DR ──→ tx_fifo[1] ──→ ssi_transfer(1 byte) ──→ rx_fifo[1] │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│ OSPI 传输（dfs = 5，32 bits/frame）                          │
├─────────────────────────────────────────────────────────────┤
│ CPU 写 DR ──→ tx_fifo[0]                                    │
│ CPU 写 DR ──→ tx_fifo[1]                                    │
│ CPU 写 DR ──→ tx_fifo[2]                                    │
│ CPU 写 DR ──→ tx_fifo[3] ──→ ssi_transfer(4 bytes) ──→ rx_fifo[0-3]│
└─────────────────────────────────────────────────────────────┘
```

**差异总结**：

| 特性 | 标准 SPI | OSPI |
|------|---------|------|
| **数据帧大小** | 固定 8 bits | 可配置（4~128 bits） |
| **传输宽度** | 1 bit（单条数据线） | 2/4/8 bits（多条数据线并行） |
| **`ssi_transfer` 调用次数** | 1 次/帧 | `bytes_per_frame` 次/帧 |
| **FIFO 条目含义** | 1 byte | 1 个完整数据帧（可变大小） |
| **时钟效率** | 8 个时钟周期传 1 byte | 1 个时钟周期传 1~8 bytes |

### 4. QEMU SSI 总线的限制

当前实现的一个重要特点：**QEMU 的 `ssi_transfer` API 每次只能传输 1 byte**。

这意味着即使配置了 Octal SPI（8 条数据线并行），在 QEMU 模拟中也需要通过 8 次 `ssi_transfer` 调用来模拟一次完整的 Octal 传输。**这是模拟层面的简化**，实际硬件中 Octal SPI 是真正并行传输的。

### 5. 实际硬件 vs QEMU 模拟

```
实际硬件（Octal SPI）:
  MOSI[7:0] ──┐
             ├── 同时传输 1 byte（1 个时钟周期）
  MISO[7:0] ──┘

QEMU 模拟（Octal SPI）:
  MOSI[0] ──→ ssi_transfer(byte0) → MISO[0]
  MOSI[1] ──→ ssi_transfer(byte1) → MISO[1]
  ...
  MOSI[7] ──→ ssi_transfer(byte7) → MISO[7]
  （需要 8 次调用，但结果等价）
```

**总结**：OSPI 的一次收发 8bit 在建模中表现为：
1. 通过 `CTRLR0[12:11]` 设置 `frf_ssic = 3` 启用 Octal 模式
2. 通过 `CTRLR0[6:4]` 设置 `dfs` 决定数据帧大小
3. 根据 `bytes_per_frame` 多次调用 `ssi_transfer` 完成传输
4. 与标准 SPI 的差异在于支持更大的数据帧和并行传输能力



# 6. 分析k230_spi_do_transfer函数