# K230 SPI 控制器（Cadence SSIC）实现文档

## 1. 概述

本文档描述了 K230 SPI 控制器的 QEMU 实现，该控制器基于 Cadence SSIC（SPI 串行接口控制器）IP 核。实现遵循 K230 技术参考手册 V0.3.1（2024-11-18）。

### 主要特性

- 标准 SPI、Dual SPI、Quad SPI 和 Octal SPI 模式
- XIP（eXecute In Place）模式，支持直接访问 Flash 内存
- 可配置阈值的 TX/RX FIFO
- 使用专用清除寄存器的中断处理机制
- Flash 内存窗口映射

## 2. 寄存器映射

K230 SPI 控制器实现了技术手册中定义的以下寄存器布局：

### 2.1 核心控制寄存器（0x00 - 0x5C）

| 偏移地址 | 寄存器名 | 描述 | 复位值 |
|----------|----------|------|--------|
| 0x00 | CTRLR0 | 控制寄存器 0 | 0x00004007 |
| 0x04 | CTRLR1 | 控制寄存器 1 | 0x00000000 |
| 0x08 | SSIENR | SSI 使能寄存器 | 0x00000000 |
| 0x0C | MWCR | Microwire 控制寄存器 | 0x00000000 |
| 0x10 | SER | 从设备使能寄存器 | 0x00000000 |
| 0x14 | BAUDR | 波特率选择寄存器 | 0x00000000 |
| 0x18 | TXFTLR | TX FIFO 阈值寄存器 | 0x00000000 |
| 0x1C | RXFTLR | RX FIFO 阈值寄存器 | 0x00000000 |
| 0x20 | TXFLR | TX FIFO 水位寄存器 | 0x00000000 |
| 0x24 | RXFLR | RX FIFO 水位寄存器 | 0x00000000 |
| 0x28 | SR | 状态寄存器 | 0x00000006 |
| 0x2C | IMR | 中断掩码寄存器 | 0x0000003F |
| 0x30 | ISR | 中断状态寄存器 | 0x00000000 |
| 0x34 | RISR | 原始中断状态寄存器 | 0x00000000 |
| 0x38 | TXEICR | TX 错误中断清除寄存器 | 0x00000000 |
| 0x3C | RXOICR | RX 溢出中断清除寄存器 | 0x00000000 |
| 0x40 | RXUICR | RX 下溢中断清除寄存器 | 0x00000000 |
| 0x44 | MSTICR | 多主机竞争中断清除寄存器 | 0x00000000 |
| 0x48 | ICR | 中断清除寄存器 | 0x00000000 |
| 0x4C | DMACR | DMA 控制寄存器 | 0x00000000 |
| 0x58 | IDR | 标识寄存器 | 0xA1B2C3D5 |
| 0x5C | SSIC_VERSION_ID | 版本寄存器 | 0x3130332A |

### 2.2 数据寄存器（0x60）

| 偏移地址 | 寄存器名 | 描述 |
|----------|----------|------|
| 0x60 | DR | 数据寄存器（8 个条目） |

### 2.3 扩展控制寄存器（0x68 - 0x148）

| 偏移地址 | 寄存器名 | 描述 | 复位值 |
|----------|----------|------|--------|
| 0x68 | SSI_CTRL | 控制寄存器 | 0x00000000 |
| 0xF0 | RX_SAMPLE_DELAY | RX 采样延迟寄存器 | 0x00000000 |
| 0xF4 | SPI_CTRLR0 | SPI 控制寄存器 0 | 0x28000200 |
| 0xF8 | DDR_DRIVE_EDGE | DDR 驱动边沿寄存器 | 0x00000000 |
| 0xFC | XIP_MODE_BITS | XIP 模式位寄存器 | 0x00000000 |
| 0x100 | XIP_INCR_INST | XIP INCR 操作码寄存器 | 0x00000000 |
| 0x104 | XIP_WRAP_INST | XIP WRAP 操作码寄存器 | 0x00000004 |
| 0x10C | XIP_SER | XIP 从设备使能寄存器 | 0x00000000 |
| 0x118 | SPI_CTRLR1 | SPI 控制寄存器 1 | 0x00000000 |
| 0x120 | SPIDR | SPI 设备寄存器 | 0x00000000 |
| 0x124 | SPIAR | SPI 地址寄存器 | 0x00000000 |
| 0x140 | XIP_WRITE_INCR_INST | XIP 写 INCR 操作码寄存器 | 0x00000000 |
| 0x144 | XIP_WRITE_WRAP_INST | XIP 写 WRAP 操作码寄存器 | 0x00000000 |
| 0x148 | XIP_WRITE_CTRL | XIP 写控制寄存器 | 0x00000072 |

## 3. Flash 内存窗口

### 3.1 地址映射

K230 SPI 控制器为 XIP（eXecute In Place）模式提供了内存映射窗口：

| 区域 | 大小 | 描述 |
|------|------|------|
| 寄存器空间 | 0x200 | SPI 控制器寄存器 |
| XIP 窗口 | 0x08000000（128MB） | Flash 内存映射区域 |

### 3.2 XIP 读操作

当 XIP 模式启用（`SPI_CTRLR0.XIP_INST_EN = 1`）且控制器启用（`SSIENR.SSI_EN = 1`）时，对 XIP 窗口地址空间的读取会自动生成 SPI 事务：

1. **指令阶段**：如果设置了 `XIP_INST_EN`，发送 `XIP_INCR_INST` 或 `XIP_WRAP_INST` 中的操作码
2. **地址阶段**：根据 `SPI_CTRLR0.ADDR_L` 将内存地址转换为 SPI Flash 地址
3. **模式位阶段**：如果设置了 `XIP_MD_BIT_EN`，发送 `XIP_MODE_BITS` 中的模式位
4. **数据阶段**：从 SPI Flash 读取请求的数据

### 3.3 XIP 写操作

XIP 写操作遵循类似的模式，但在实际写操作之前需要发送写使能（WREN）命令（0x06）：

1. 发送 WREN 命令（0x06）
2. 发送写指令（来自 `XIP_WRITE_INCR_INST`）
3. 发送地址
4. 发送数据字节

## 4. 传输模式

### 4.1 标准 SPI 模式（FRF_SSIC = 0）

在标准 SPI 模式下，数据通过 MOSI/MISO 引脚逐位传输。控制器支持以下传输模式：

| 模式 | TMOD 值 | 描述 |
|------|---------|------|
| 发送/接收 | 0x0 | 同时发送和接收 |
| 仅发送 | 0x1 | 仅发送模式 |
| 仅接收 | 0x2 | 仅接收模式 |
| EEPROM | 0x3 | 增强型外设模式 |

### 4.2 增强型 SPI 模式

控制器支持 Dual、Quad 和 Octal SPI 模式：

| 模式 | FRF_SSIC 值 | 数据宽度 |
|------|------------|----------|
| Dual SPI | 0x1 | 2 位 |
| Quad SPI | 0x2 | 4 位 |
| Octal SPI | 0x3 | 8 位 |

数据帧大小通过 `CTRLR0.DFS`（位 4-6）配置。

## 5. 中断处理

### 5.1 中断源

控制器支持以下中断源：

| 位 | 中断 | 描述 |
|----|------|------|
| 0 | TXEIS | TX FIFO 空 |
| 1 | TXOIS | TX FIFO 溢出 |
| 2 | RXOIS | RX FIFO 溢出 |
| 3 | RXUIS | RX FIFO 下溢 |
| 4 | MSTIS | 多主机竞争 |
| 5 | RXFI | RX FIFO 阈值 |

### 5.2 中断寄存器

- **IMR（中断掩码寄存器）**：使能/禁用各个中断
- **RISR（原始中断状态寄存器）**：显示原始中断状态，不受掩码影响
- **ISR（中断状态寄存器）**：显示经过掩码后的中断状态（`ISR = RISR & IMR`）

### 5.3 中断清除机制

与许多使用 W1C（写 1 清零）机制的控制器不同，Cadence SSIC 使用专用的中断清除寄存器：

| 寄存器 | 清除的中断 |
|--------|-----------|
| TXEICR | TXEIS |
| RXOICR | RXOIS |
| RXUICR | RXUIS |
| MSTICR | MSTIS |
| ICR | 所有中断 |

向相应的位写入 1 即可清除对应的中断。

## 6. FIFO 管理

### 6.1 FIFO 配置

- **TXFTLR**：TX FIFO 阈值级别（默认值为 0）
- **RXFTLR**：RX FIFO 阈值级别（默认值为 0）

### 6.2 FIFO 状态

- **TXFLR**：TX FIFO 当前字节数
- **RXFLR**：RX FIFO 当前字节数

### 6.3 状态寄存器（SR）

| 位 | 字段 | 描述 |
|----|------|------|
| 0 | TFE | TX FIFO 空 |
| 1 | TNF | TX FIFO 未满 |
| 2 | RNE | RX FIFO 非空 |
| 3 | RFF | RX FIFO 满 |
| 4 | BUSY | 控制器忙 |

## 7. 设备模型结构

### 7.1 内存区域

```
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

### 7.2 主要函数

| 函数 | 描述 |
|------|------|
| `k230_spi_reset` | 将控制器复位到初始状态 |
| `k230_spi_reg_read` | 读取寄存器值 |
| `k230_spi_reg_write` | 写入寄存器值 |
| `k230_spi_do_transfer` | 执行 SPI 传输 |
| `k230_spi_xip_read` | XIP 窗口读处理函数 |
| `k230_spi_xip_write` | XIP 窗口写处理函数 |
| `k230_spi_update_status` | 更新 FIFO 状态寄存器 |
| `k230_spi_set_irq` | 设置中断输出 |

## 8. 与 Xilinx Versal OSPI 的差异

K230 SPI 控制器使用 Cadence SSIC IP，与 Xilinx Versal OSPI 控制器有本质区别：

| 方面 | K230（Cadence SSIC） | Xilinx Versal OSPI |
|------|---------------------|--------------------|
| 寄存器命名 | CTRLR0, CTRLR1, SSIENR | CONFIG_REG, DEV_INSTR_RD_CONFIG_REG |
| 中断清除 | 专用清除寄存器（TXEICR, RXOICR 等） | W1C（写 1 清零） |
| 数据传输 | 基于 FIFO（通过 DR 寄存器） | 间接读写控制寄存器 |
| IP 核 | Cadence SSIC | Xilinx OSPI |
| 版本寄存器 | SSIC_VERSION_ID | MODULE_ID_REG |

## 9. 参考资料

- K230 技术参考手册 V0.3.1（2024-11-18）
- Cadence SSIC 产品文档
