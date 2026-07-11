/*
 * K230 SPI (Octal SPI) Controller
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

#define K230_SPI_REG_SIZE 0x1000
#define K230_SPI_XIP_SIZE 0x08000000

#define K230_SPI_CTRL              0x00
#define K230_SPI_CTRL_ENABLE       BIT(0)
#define K230_SPI_CTRL_XIP_MODE     BIT(1)
#define K230_SPI_CTRL_DIR_EN       BIT(2)
#define K230_SPI_CTRL_ADDR_REMAP   BIT(3)
#define K230_SPI_CTRL_BAUD_DIV     GENMASK(7, 4)

#define K230_SPI_READ_CFG          0x04
#define K230_SPI_READ_CFG_OPCODE   GENMASK(7, 0)
#define K230_SPI_READ_CFG_ADDR_BYTES GENMASK(9, 8)
#define K230_SPI_READ_CFG_DUMMY_CYCLES GENMASK(20, 16)
#define K230_SPI_READ_CFG_DATA_MODE GENMASK(25, 24)

#define K230_SPI_WRITE_CFG         0x08
#define K230_SPI_WRITE_CFG_OPCODE  GENMASK(7, 0)
#define K230_SPI_WRITE_CFG_ADDR_BYTES GENMASK(9, 8)

#define K230_SPI_DELAY             0x0C
#define K230_SPI_DELAY_CS_SETUP    GENMASK(7, 0)
#define K230_SPI_DELAY_CS_HOLD     GENMASK(15, 8)
#define K230_SPI_DELAY_BUS_DELAY   GENMASK(23, 16)

#define K230_SPI_DEV_SIZE          0x10
#define K230_SPI_DEV_SIZE_FLASH_SIZE GENMASK(3, 0)
#define K230_SPI_DEV_SIZE_PAGE_SIZE GENMASK(11, 4)
#define K230_SPI_DEV_SIZE_SECT_SIZE GENMASK(19, 12)
#define K230_SPI_DEV_SIZE_ADDR_BYTES GENMASK(21, 20)

#define K230_SPI_REMAP_ADDR        0x14

#define K230_SPI_IND_CTRL          0x20
#define K230_SPI_IND_CTRL_START    BIT(0)
#define K230_SPI_IND_CTRL_CANCEL   BIT(1)
#define K230_SPI_IND_CTRL_DONE     BIT(2)
#define K230_SPI_IND_CTRL_BUSY     BIT(3)

#define K230_SPI_IND_START_ADDR    0x24
#define K230_SPI_IND_NUM_BYTES     0x28

#define K230_SPI_STIG_CTRL         0x30
#define K230_SPI_STIG_CTRL_EXEC    BIT(0)
#define K230_SPI_STIG_CTRL_EN_ADDR BIT(1)
#define K230_SPI_STIG_CTRL_EN_RD_DATA BIT(2)
#define K230_SPI_STIG_CTRL_EN_WR_DATA BIT(3)

#define K230_SPI_STIG_OPCODE       0x34
#define K230_SPI_STIG_ADDR         0x38
#define K230_SPI_STIG_DATA_LOW     0x3C
#define K230_SPI_STIG_DATA_HIGH    0x40

#define K230_SPI_IRQ_STATUS        0x50
#define K230_SPI_IRQ_STATUS_IND_DONE BIT(0)
#define K230_SPI_IRQ_STATUS_STIG_DONE BIT(1)
#define K230_SPI_IRQ_STATUS_ERROR  BIT(2)

#define K230_SPI_IRQ_MASK          0x54

#define K230_SPI_VERSION           0xFC

#define K230_SPI_REG_MAX (0xFC / 4 + 1)

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

    uint8_t *flash_cache;
    size_t flash_size;

    bool xip_enabled;
    bool direct_access_enabled;
};

#endif