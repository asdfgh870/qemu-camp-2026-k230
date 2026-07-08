## 后续开发指南

### 1. 编译代码

```bash
# 首次配置（只需运行一次）
make -f Makefile.camp configure

# 编译（开发过程中常用）
make -f Makefile.camp build

# 清理后重新编译
make -f Makefile.camp rebuild
```

### 2. 修改代码后快速重建

修改 [k230.c](file:///home/pc/qemu-camp-2026-k230/hw/riscv/k230.c) 或 [k230_wdt.c](file:///home/pc/qemu-camp-2026-k230/hw/watchdog/k230_wdt.c) 后，只需运行：

```bash
make -f Makefile.camp build
```

### 3. 运行测试

```bash
# 运行所有 K230 测试
make -f Makefile.camp test

# 只运行 WDT 测试
make -f Makefile.camp test-k230-wdt
```

### 4. 启动 K230 虚拟机

```bash
# 快速测试
make -f Makefile.camp run

# 完整启动（需要固件和内核）
./build/qemu-system-riscv64 -machine k230 -nographic \
    -bios path/to/opensbi.elf \
    -kernel path/to/kernel.bin \
    -dtb path/to/k230.dtb
```

### 5. 清理构建

```bash
# 清理构建产物（保留配置）
make -f Makefile.camp clean

# 删除整个构建目录（需要重新配置）
make -f Makefile.camp distclean
```

### 6. 查看帮助

```bash
make -f Makefile.camp help
```

## 关键配置说明

- **目标架构**: `riscv64-softmmu`（仅编译 RISC-V 系统模拟器）
- **编译选项**: `-O0 -g3`（调试模式，保留完整调试信息）
- **文档禁用**: `--disable-docs`（避免文档依赖问题）
- **并行编译**: 默认使用所有 CPU 核心（可通过 `JOBS=N` 指定）

## 测试覆盖

当前测试文件 [k230-wdt-test.c](file:///home/pc/qemu-camp-2026-k230/tests/qtest/k230-wdt-test.c) 包含以下测试用例：
- 寄存器读写测试
- 计数器重启测试
- 中断模式测试
- 复位模式测试
- 超时计算测试
- WDT1 寄存器测试
- 使能/禁用测试

所有测试均已通过，您可以在此基础上继续开发新的外设（如 GPIO、UART、I2C 等）并添加相应的测试用例。