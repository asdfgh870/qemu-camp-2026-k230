/*
 * K230 SPI (Flash Memory Controller - Cadence SSIC)
 *
 * K230 Technical Reference Manual V0.3.1 (2024-11-18):
 * https://github.com/revyos/external-docs/blob/master/K230/en-us/K230_Technical_Reference_Manual_V0.3.1_20241118.pdf
 *
 * Copyright (c) 2025 Chao Liu <chao.liu.zevorn@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_K230_SPI_H
#define HW_K230_SPI_H

#include "hw/core/sysbus.h"
#include "hw/ssi/ssi.h"
#include "qemu/fifo8.h"
#include "qom/object.h"

#define TYPE_K230_SPI "riscv.k230.spi"
OBJECT_DECLARE_SIMPLE_TYPE(K230SpiState, K230_SPI)

#define K230_SPI_REG_SIZE 0x200
#define K230_SPI_XIP_SIZE 0x08000000

#define K230_SPI_CTRLR0            0x00
#define K230_SPI_CTRLR0_FRF        GENMASK(2, 0)
#define K230_SPI_CTRLR0_FRF_SPI    0x0
#define K230_SPI_CTRLR0_FRF_TI     0x1
#define K230_SPI_CTRLR0_FRF_MICRO  0x2
#define K230_SPI_CTRLR0_DFS        GENMASK(6, 4)
#define K230_SPI_CTRLR0_TMOD       GENMASK(8, 7)
#define K230_SPI_CTRLR0_TMOD_TR   0x0
#define K230_SPI_CTRLR0_TMOD_TO   0x1
#define K230_SPI_CTRLR0_TMOD_RO   0x2
#define K230_SPI_CTRLR0_TMOD_EP   0x3
#define K230_SPI_CTRLR0_SCPH       BIT(9)
#define K230_SPI_CTRLR0_SCPOL      BIT(10)
#define K230_SPI_CTRLR0_FRF_SSIC   GENMASK(12, 11)
#define K230_SPI_CTRLR0_FRF_SPI_STD  0x0
#define K230_SPI_CTRLR0_FRF_SPI_DUAL 0x1
#define K230_SPI_CTRLR0_FRF_SPI_QUAD 0x2
#define K230_SPI_CTRLR0_FRF_SPI_OCTAL 0x3
#define K230_SPI_CTRLR0_TCS        BIT(13)
#define K230_SPI_CTRLR0_SLVOE      BIT(14)
#define K230_SPI_CTRLR0_MASTER     BIT(15)
#define K230_SPI_CTRLR0_SLV_OE     BIT(16)
#define K230_SPI_CTRLR0_TRANS_LEN  GENMASK(31, 17)

#define K230_SPI_CTRLR1            0x04
#define K230_SPI_CTRLR1_NDF        GENMASK(15, 0)

#define K230_SPI_SSIENR            0x08
#define K230_SPI_SSIENR_SSI_EN     BIT(0)

#define K230_SPI_MWCR              0x0C
#define K230_SPI_MWCR_MICROWIRE_EN BIT(0)

#define K230_SPI_SER               0x10
#define K230_SPI_SER_SLAVE_EN      BIT(0)

#define K230_SPI_BAUDR             0x14
#define K230_SPI_BAUDR_DIV         GENMASK(15, 0)

#define K230_SPI_TXFTLR            0x18
#define K230_SPI_TXFTLR_TX_THRES   GENMASK(7, 0)

#define K230_SPI_RXFTLR            0x1C
#define K230_SPI_RXFTLR_RX_THRES   GENMASK(7, 0)

#define K230_SPI_TXFLR             0x20
#define K230_SPI_TXFLR_TX_LEVEL    GENMASK(7, 0)

#define K230_SPI_RXFLR             0x24
#define K230_SPI_RXFLR_RX_LEVEL    GENMASK(7, 0)

#define K230_SPI_SR                0x28
#define K230_SPI_SR_TFE            BIT(0)
#define K230_SPI_SR_TNF            BIT(1)
#define K230_SPI_SR_RNE            BIT(2)
#define K230_SPI_SR_RFF            BIT(3)
#define K230_SPI_SR_BUSY           BIT(4)

#define K230_SPI_IMR               0x2C
#define K230_SPI_IMR_TXEIM         BIT(0)
#define K230_SPI_IMR_TXOIM         BIT(1)
#define K230_SPI_IMR_RXOIM         BIT(2)
#define K230_SPI_IMR_RXUIM         BIT(3)
#define K230_SPI_IMR_MSTIM         BIT(4)
#define K230_SPI_IMR_RXFI          BIT(5)

#define K230_SPI_ISR               0x30
#define K230_SPI_ISR_TXEIS         BIT(0)
#define K230_SPI_ISR_TXOIS         BIT(1)
#define K230_SPI_ISR_RXOIS         BIT(2)
#define K230_SPI_ISR_RXUIS         BIT(3)
#define K230_SPI_ISR_MSTIS         BIT(4)
#define K230_SPI_ISR_RXFI          BIT(5)

#define K230_SPI_RISR              0x34
#define K230_SPI_RISR_TXERIS       BIT(0)
#define K230_SPI_RISR_TXORIS       BIT(1)
#define K230_SPI_RISR_RXORIS       BIT(2)
#define K230_SPI_RISR_RXURIS       BIT(3)
#define K230_SPI_RISR_MSTRIS       BIT(4)
#define K230_SPI_RISR_RXFRIS       BIT(5)

#define K230_SPI_TXEICR            0x38
#define K230_SPI_TXEICR_TXEIC      BIT(0)

#define K230_SPI_RXOICR            0x3C
#define K230_SPI_RXOICR_RXOIC      BIT(0)

#define K230_SPI_RXUICR            0x40
#define K230_SPI_RXUICR_RXUIC      BIT(0)

#define K230_SPI_MSTICR            0x44
#define K230_SPI_MSTICR_MSTIC      BIT(0)

#define K230_SPI_ICR               0x48
#define K230_SPI_ICR_ALLIC         BIT(0)

#define K230_SPI_DMACR             0x4C
#define K230_SPI_DMACR_RDMAE       BIT(0)
#define K230_SPI_DMACR_TDMAE       BIT(1)

#define K230_SPI_IDR               0x58
#define K230_SPI_SSIC_VERSION_ID   0x5C

#define K230_SPI_DR                0x60

#define K230_SPI_SSI_CTRL          0x68
#define K230_SPI_SSI_CTRL_SOFT_RST BIT(0)

#define K230_SPI_RX_SAMPLE_DELAY   0xF0

#define K230_SPI_SPI_CTRLR0        0xF4
#define K230_SPI_SPI_CTRLR0_XIP_PREFETCH_EN BIT(0)
#define K230_SPI_SPI_CTRLR0_XIP_MBL        GENMASK(3, 1)
#define K230_SPI_SPI_CTRLR0_CONT_XFER_EN   BIT(4)
#define K230_SPI_SPI_CTRLR0_XIP_INST_EN    BIT(5)
#define K230_SPI_SPI_CTRLR0_XIP_DFS_HC     BIT(6)
#define K230_SPI_SPI_CTRLR0_XIP_MD_BIT_EN  BIT(7)
#define K230_SPI_SPI_CTRLR0_INST_L         GENMASK(15, 8)
#define K230_SPI_SPI_CTRLR0_ADDR_L         GENMASK(23, 16)
#define K230_SPI_SPI_CTRLR0_OPCODE_L       GENMASK(31, 24)

#define K230_SPI_DDR_DRIVE_EDGE    0xF8

#define K230_SPI_XIP_MODE_BITS     0xFC
#define K230_SPI_XIP_MODE_BITS_MD  GENMASK(7, 0)

#define K230_SPI_XIP_INCR_INST     0x100
#define K230_SPI_XIP_WRAP_INST     0x104

#define K230_SPI_XIP_SER           0x10C

#define K230_SPI_SPI_CTRLR1        0x118
#define K230_SPI_SPI_CTRLR1_CK_PHASE GENMASK(3, 0)

#define K230_SPI_SPIDR             0x120
#define K230_SPI_SPIAR             0x124

#define K230_SPI_XIP_WRITE_INCR_INST 0x140
#define K230_SPI_XIP_WRITE_WRAP_INST 0x144
#define K230_SPI_XIP_WRITE_CTRL      0x148

#define K230_SPI_REG_MAX (0x148 / 4 + 1)

struct K230SpiState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    MemoryRegion iomem_xip;

    SSIBus *spi;
    qemu_irq cs_line;

    Fifo8 rx_fifo;
    Fifo8 tx_fifo;

    qemu_irq irq;

    uint32_t regs[K230_SPI_REG_MAX];

    bool xip_enabled;
    bool ssi_enabled;

    uint8_t *flash_cache;
    size_t flash_size;
};

#endif
