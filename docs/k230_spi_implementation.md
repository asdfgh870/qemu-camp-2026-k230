# K230 SPI Controller (Cadence SSIC) Implementation Document

## 1. Overview

This document describes the QEMU implementation of the K230 SPI Controller, which is based on the Cadence SSIC (SPI Serial Interface Controller) IP core. The implementation follows the K230 Technical Reference Manual V0.3.1 (2024-11-18).

### Key Features

- Standard SPI, Dual SPI, Quad SPI, and Octal SPI modes
- XIP (eXecute In Place) mode for direct flash memory access
- TX/RX FIFO with configurable thresholds
- Interrupt handling with dedicated clear registers
- Flash memory window mapping

## 2. Register Mapping

The K230 SPI Controller implements the following register layout as defined in the technical manual:

### 2.1 Core Control Registers (0x00 - 0x5C)

| Offset | Register | Description | Reset Value |
|--------|----------|-------------|-------------|
| 0x00 | CTRLR0 | Control Register 0 | 0x00004007 |
| 0x04 | CTRLR1 | Control Register 1 | 0x00000000 |
| 0x08 | SSIENR | SSI Enable Register | 0x00000000 |
| 0x0C | MWCR | Microwire Control | 0x00000000 |
| 0x10 | SER | Slave Enable Register | 0x00000000 |
| 0x14 | BAUDR | Baud Rate Select | 0x00000000 |
| 0x18 | TXFTLR | TX FIFO Threshold | 0x00000000 |
| 0x1C | RXFTLR | RX FIFO Threshold | 0x00000000 |
| 0x20 | TXFLR | TX FIFO Level | 0x00000000 |
| 0x24 | RXFLR | RX FIFO Level | 0x00000000 |
| 0x28 | SR | Status Register | 0x00000006 |
| 0x2C | IMR | Interrupt Mask Register | 0x0000003F |
| 0x30 | ISR | Interrupt Status Register | 0x00000000 |
| 0x34 | RISR | Raw Interrupt Status Register | 0x00000000 |
| 0x38 | TXEICR | TX Error Interrupt Clear | 0x00000000 |
| 0x3C | RXOICR | RX Overflow Interrupt Clear | 0x00000000 |
| 0x40 | RXUICR | RX Underflow Interrupt Clear | 0x00000000 |
| 0x44 | MSTICR | Multi-Master Interrupt Clear | 0x00000000 |
| 0x48 | ICR | Interrupt Clear Register | 0x00000000 |
| 0x4C | DMACR | DMA Control Register | 0x00000000 |
| 0x58 | IDR | Identification Register | 0xA1B2C3D5 |
| 0x5C | SSIC_VERSION_ID | Version Register | 0x3130332A |

### 2.2 Data Register (0x60)

| Offset | Register | Description |
|--------|----------|-------------|
| 0x60 | DR | Data Register (8 entries) |

### 2.3 Extended Control Registers (0x68 - 0x148)

| Offset | Register | Description | Reset Value |
|--------|----------|-------------|-------------|
| 0x68 | SSI_CTRL | Control Register | 0x00000000 |
| 0xF0 | RX_SAMPLE_DELAY | RX Sample Delay | 0x00000000 |
| 0xF4 | SPI_CTRLR0 | SPI Control Register 0 | 0x28000200 |
| 0xF8 | DDR_DRIVE_EDGE | DDR Drive Edge | 0x00000000 |
| 0xFC | XIP_MODE_BITS | XIP Mode Bits | 0x00000000 |
| 0x100 | XIP_INCR_INST | XIP INCR Opcode | 0x00000000 |
| 0x104 | XIP_WRAP_INST | XIP WRAP Opcode | 0x00000004 |
| 0x10C | XIP_SER | XIP Slave Enable | 0x00000000 |
| 0x118 | SPI_CTRLR1 | SPI Control Register 1 | 0x00000000 |
| 0x120 | SPIDR | SPI Device Register | 0x00000000 |
| 0x124 | SPIAR | SPI Address Register | 0x00000000 |
| 0x140 | XIP_WRITE_INCR_INST | XIP Write INCR Opcode | 0x00000000 |
| 0x144 | XIP_WRITE_WRAP_INST | XIP Write WRAP Opcode | 0x00000000 |
| 0x148 | XIP_WRITE_CTRL | XIP Write Control | 0x00000072 |

## 3. Flash Memory Window

### 3.1 Address Mapping

The K230 SPI Controller provides a memory-mapped window for XIP (eXecute In Place) mode:

| Region | Size | Description |
|--------|------|-------------|
| Register Space | 0x200 | SPI controller registers |
| XIP Window | 0x08000000 (128MB) | Flash memory mapped region |

### 3.2 XIP Read Operation

When XIP mode is enabled (`SPI_CTRLR0.XIP_INST_EN = 1`) and the controller is enabled (`SSIENR.SSI_EN = 1`), reads to the XIP window address space automatically generate SPI transactions:

1. **Instruction Phase**: If `XIP_INST_EN` is set, the opcode from `XIP_INCR_INST` or `XIP_WRAP_INST` is sent
2. **Address Phase**: The memory address is converted to a SPI flash address based on `SPI_CTRLR0.ADDR_L`
3. **Mode Bits Phase**: If `XIP_MD_BIT_EN` is set, mode bits from `XIP_MODE_BITS` are sent
4. **Data Phase**: The requested data is read from the SPI flash

### 3.3 XIP Write Operation

XIP writes follow a similar pattern but require a Write Enable (WREN) command (0x06) before the actual write operation:

1. Send WREN command (0x06)
2. Send write instruction (from `XIP_WRITE_INCR_INST`)
3. Send address
4. Send data bytes

## 4. Transfer Modes

### 4.1 Standard SPI Mode (FRF_SSIC = 0)

In standard SPI mode, data is transferred one bit at a time using the MOSI/MISO pins. The controller supports the following transfer modes:

| Mode | TMOD Value | Description |
|------|------------|-------------|
| TX/RX | 0x0 | Transmit and receive |
| TX Only | 0x1 | Transmit only |
| RX Only | 0x2 | Receive only |
| EEPROM | 0x3 | Enhanced peripheral mode |

### 4.2 Enhanced SPI Modes

The controller supports Dual, Quad, and Octal SPI modes:

| Mode | FRF_SSIC Value | Data Width |
|------|---------------|------------|
| Dual SPI | 0x1 | 2 bits |
| Quad SPI | 0x2 | 4 bits |
| Octal SPI | 0x3 | 8 bits |

The data frame size is configured via `CTRLR0.DFS` (bits 4-6).

## 5. Interrupt Handling

### 5.1 Interrupt Sources

The controller supports the following interrupt sources:

| Bit | Interrupt | Description |
|-----|-----------|-------------|
| 0 | TXEIS | TX FIFO Empty |
| 1 | TXOIS | TX FIFO Overflow |
| 2 | RXOIS | RX FIFO Overflow |
| 3 | RXUIS | RX FIFO Underflow |
| 4 | MSTIS | Multi-Master Contention |
| 5 | RXFI | RX FIFO Threshold |

### 5.2 Interrupt Registers

- **IMR (Interrupt Mask Register)**: Enables/disables individual interrupts
- **RISR (Raw Interrupt Status Register)**: Shows raw interrupt status regardless of mask
- **ISR (Interrupt Status Register)**: Shows masked interrupt status (`ISR = RISR & IMR`)

### 5.3 Interrupt Clear Mechanism

Unlike many controllers that use W1C (Write 1 to Clear), the Cadence SSIC uses dedicated interrupt clear registers:

| Register | Clears |
|----------|--------|
| TXEICR | TXEIS |
| RXOICR | RXOIS |
| RXUICR | RXUIS |
| MSTICR | MSTIS |
| ICR | All interrupts |

Writing 1 to the appropriate bit clears the corresponding interrupt.

## 6. FIFO Management

### 6.1 FIFO Configuration

- **TXFTLR**: TX FIFO threshold level (default 0)
- **RXFTLR**: RX FIFO threshold level (default 0)

### 6.2 FIFO Status

- **TXFLR**: Current number of bytes in TX FIFO
- **RXFLR**: Current number of bytes in RX FIFO

### 6.3 Status Register (SR)

| Bit | Field | Description |
|-----|-------|-------------|
| 0 | TFE | TX FIFO Empty |
| 1 | TNF | TX FIFO Not Full |
| 2 | RNE | RX FIFO Not Empty |
| 3 | RFF | RX FIFO Full |
| 4 | BUSY | Controller Busy |

## 7. Device Model Structure

### 7.1 Memory Regions

```
K230SpiState
├── iomem          : Register space (0x200 bytes)
├── iomem_xip      : XIP window (128MB)
├── spi            : SSI bus connection
├── cs_line        : Chip select IRQ
├── rx_fifo        : Receive FIFO (256 bytes)
├── tx_fifo        : Transmit FIFO (256 bytes)
├── irq            : Interrupt output
├── regs[]         : Register bank
├── xip_enabled    : XIP mode flag
├── ssi_enabled    : SSI enable flag
├── flash_cache    : Flash data cache
└── flash_size     : Flash size (default 16MB)
```

### 7.2 Key Functions

| Function | Description |
|----------|-------------|
| `k230_spi_reset` | Reset controller to initial state |
| `k230_spi_reg_read` | Read register value |
| `k230_spi_reg_write` | Write register value |
| `k230_spi_do_transfer` | Perform SPI transfer |
| `k230_spi_xip_read` | XIP window read handler |
| `k230_spi_xip_write` | XIP window write handler |
| `k230_spi_update_status` | Update FIFO status registers |
| `k230_spi_set_irq` | Set interrupt output |

## 8. Differences from Xilinx Versal OSPI

The K230 SPI Controller uses the Cadence SSIC IP, which is fundamentally different from the Xilinx Versal OSPI controller:

| Aspect | K230 (Cadence SSIC) | Xilinx Versal OSPI |
|--------|---------------------|-------------------|
| Register Naming | CTRLR0, CTRLR1, SSIENR | CONFIG_REG, DEV_INSTR_RD_CONFIG_REG |
| Interrupt Clear | Dedicated registers (TXEICR, RXOICR, etc.) | W1C (Write 1 to Clear) |
| Data Transfer | FIFO-based via DR register | Indirect read/write control registers |
| IP Core | Cadence SSIC | Xilinx OSPI |
| Version Register | SSIC_VERSION_ID | MODULE_ID_REG |

## 9. References

- K230 Technical Reference Manual V0.3.1 (2024-11-18)
- Cadence SSIC Product Documentation
