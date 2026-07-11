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

#include "qemu/osdep.h"
#include "qemu/bitops.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "hw/core/qdev-properties.h"
#include "hw/core/irq.h"
#include "hw/riscv/k230_spi.h"
#include "hw/block/flash.h"
#include "migration/vmstate.h"

#define DEFAULT_FLASH_SIZE (16 * 1024 * 1024)

static void k230_spi_reset(DeviceState *dev)
{
    K230SpiState *s = K230_SPI(dev);

    memset(s->regs, 0, sizeof(s->regs));
    s->regs[K230_SPI_CTRL / 4] = 0;
    s->regs[K230_SPI_READ_CFG / 4] = 0x6B;
    s->regs[K230_SPI_WRITE_CFG / 4] = 0x02;
    s->regs[K230_SPI_DEV_SIZE / 4] = (1 << 4) | (256 << 4) | (4096 << 12) | (3 << 20);
    s->regs[K230_SPI_VERSION / 4] = 0x30000001;

    fifo8_reset(&s->rx_fifo);
    fifo8_reset(&s->tx_fifo);

    s->xip_enabled = false;
    s->direct_access_enabled = false;
}

static void k230_spi_set_irq(K230SpiState *s, uint32_t mask)
{
    s->regs[K230_SPI_IRQ_STATUS / 4] |= s->regs[K230_SPI_IRQ_MASK / 4] & mask;
    qemu_set_irq(s->irq, !!(s->regs[K230_SPI_IRQ_STATUS / 4] &
                           s->regs[K230_SPI_IRQ_MASK / 4]));
}

static uint32_t k230_spi_get_addr_bytes(K230SpiState *s)
{
    return extract32(s->regs[K230_SPI_DEV_SIZE / 4], 20, 2) + 1;
}

static uint8_t k230_spi_get_rd_opcode(K230SpiState *s)
{
    return extract32(s->regs[K230_SPI_READ_CFG / 4], 0, 8);
}

static uint8_t k230_spi_get_wr_opcode(K230SpiState *s)
{
    return extract32(s->regs[K230_SPI_WRITE_CFG / 4], 0, 8);
}

static void k230_spi_tx_fifo_push_addr(K230SpiState *s, uint32_t addr)
{
    int addr_bytes = k230_spi_get_addr_bytes(s);

    if (addr_bytes == 4) {
        fifo8_push(&s->tx_fifo, addr >> 24);
    }
    if (addr_bytes >= 3) {
        fifo8_push(&s->tx_fifo, addr >> 16);
    }
    if (addr_bytes >= 2) {
        fifo8_push(&s->tx_fifo, addr >> 8);
    }
    fifo8_push(&s->tx_fifo, addr);
}

static void k230_spi_flush_txfifo(K230SpiState *s)
{
    while (!fifo8_is_empty(&s->tx_fifo)) {
        uint32_t tx_rx = fifo8_pop(&s->tx_fifo);
        tx_rx = ssi_transfer(s->spi, tx_rx);
        fifo8_push(&s->rx_fifo, tx_rx);
    }
}

static void k230_spi_select_cs(K230SpiState *s, bool select)
{
    qemu_set_irq(s->cs_line, !select);
}

static void k230_spi_do_read(K230SpiState *s, uint32_t addr, uint32_t len)
{
    uint8_t opcode = k230_spi_get_rd_opcode(s);
    int dummy_cycles = extract32(s->regs[K230_SPI_READ_CFG / 4], 16, 5);
    int i;

    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);

    fifo8_push(&s->tx_fifo, opcode);
    k230_spi_tx_fifo_push_addr(s, addr);

    for (i = 0; i < dummy_cycles; i++) {
        fifo8_push(&s->tx_fifo, 0);
    }

    k230_spi_select_cs(s, true);
    k230_spi_flush_txfifo(s);

    fifo8_reset(&s->rx_fifo);
    for (i = 0; i < len; i++) {
        fifo8_push(&s->tx_fifo, 0);
    }
    k230_spi_flush_txfifo(s);

    k230_spi_select_cs(s, false);
}

static void k230_spi_do_write(K230SpiState *s, uint32_t addr, const uint8_t *data, uint32_t len)
{
    uint8_t opcode = k230_spi_get_wr_opcode(s);
    int i;

    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);

    fifo8_push(&s->tx_fifo, 0x06);
    k230_spi_select_cs(s, true);
    k230_spi_flush_txfifo(s);
    k230_spi_select_cs(s, false);

    fifo8_reset(&s->tx_fifo);
    fifo8_push(&s->tx_fifo, opcode);
    k230_spi_tx_fifo_push_addr(s, addr);

    for (i = 0; i < len; i++) {
        fifo8_push(&s->tx_fifo, data[i]);
    }

    k230_spi_select_cs(s, true);
    k230_spi_flush_txfifo(s);
    k230_spi_select_cs(s, false);
}

static uint64_t k230_spi_reg_read(void *opaque, hwaddr addr, unsigned int size)
{
    K230SpiState *s = K230_SPI(opaque);
    uint32_t reg_addr = addr >> 2;

    if (reg_addr >= K230_SPI_REG_MAX) {
        return 0;
    }

    return s->regs[reg_addr];
}

static void k230_spi_stig_exec(K230SpiState *s)
{
    uint8_t opcode = s->regs[K230_SPI_STIG_OPCODE / 4];
    uint32_t addr = s->regs[K230_SPI_STIG_ADDR / 4];
    bool en_addr = extract32(s->regs[K230_SPI_STIG_CTRL / 4], 1, 1);
    bool en_rd_data = extract32(s->regs[K230_SPI_STIG_CTRL / 4], 2, 1);
    bool en_wr_data = extract32(s->regs[K230_SPI_STIG_CTRL / 4], 3, 1);
    uint64_t wr_data = ((uint64_t)s->regs[K230_SPI_STIG_DATA_HIGH / 4] << 32) |
                       s->regs[K230_SPI_STIG_DATA_LOW / 4];
    uint8_t data[8] = {};
    int i;

    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);

    fifo8_push(&s->tx_fifo, opcode);

    if (en_addr) {
        k230_spi_tx_fifo_push_addr(s, addr);
    }

    if (en_wr_data) {
        for (i = 0; i < 8; i++) {
            fifo8_push(&s->tx_fifo, wr_data >> (i * 8));
        }
    }

    k230_spi_select_cs(s, true);

    if (en_rd_data) {
        k230_spi_flush_txfifo(s);
        fifo8_reset(&s->rx_fifo);
        for (i = 0; i < 8; i++) {
            fifo8_push(&s->tx_fifo, 0);
        }
        k230_spi_flush_txfifo(s);

        for (i = 0; i < 8; i++) {
            data[i] = fifo8_pop(&s->rx_fifo);
        }

        s->regs[K230_SPI_STIG_DATA_LOW / 4] = ldl_le_p(data);
        s->regs[K230_SPI_STIG_DATA_HIGH / 4] = ldl_le_p(data + 4);
    } else {
        k230_spi_flush_txfifo(s);
    }

    k230_spi_select_cs(s, false);

    s->regs[K230_SPI_STIG_CTRL / 4] &= ~K230_SPI_STIG_CTRL_EXEC;
    k230_spi_set_irq(s, K230_SPI_IRQ_STATUS_STIG_DONE);
}

static void k230_spi_ind_exec(K230SpiState *s)
{
    uint32_t addr = s->regs[K230_SPI_IND_START_ADDR / 4];
    uint32_t num_bytes = s->regs[K230_SPI_IND_NUM_BYTES / 4];

    s->regs[K230_SPI_IND_CTRL / 4] |= K230_SPI_IND_CTRL_BUSY;

    k230_spi_do_read(s, addr, num_bytes);

    s->regs[K230_SPI_IND_CTRL / 4] &= ~K230_SPI_IND_CTRL_BUSY;
    s->regs[K230_SPI_IND_CTRL / 4] |= K230_SPI_IND_CTRL_DONE;
    s->regs[K230_SPI_IND_CTRL / 4] &= ~K230_SPI_IND_CTRL_START;

    k230_spi_set_irq(s, K230_SPI_IRQ_STATUS_IND_DONE);
}

static void k230_spi_reg_write(void *opaque, hwaddr addr, uint64_t value, unsigned int size)
{
    K230SpiState *s = K230_SPI(opaque);
    uint32_t reg_addr = addr >> 2;

    if (reg_addr >= K230_SPI_REG_MAX) {
        return;
    }

    switch (addr) {
    case K230_SPI_CTRL:
        s->regs[reg_addr] = value;
        s->xip_enabled = extract32(value, 1, 1);
        s->direct_access_enabled = extract32(value, 2, 1);
        break;

    case K230_SPI_IND_CTRL:
        s->regs[reg_addr] = value & ~(K230_SPI_IND_CTRL_BUSY | K230_SPI_IND_CTRL_DONE);
        if (value & K230_SPI_IND_CTRL_START) {
            k230_spi_ind_exec(s);
        }
        if (value & K230_SPI_IND_CTRL_CANCEL) {
            s->regs[reg_addr] &= ~K230_SPI_IND_CTRL_CANCEL;
            s->regs[reg_addr] &= ~K230_SPI_IND_CTRL_BUSY;
        }
        break;

    case K230_SPI_STIG_CTRL:
        s->regs[reg_addr] = value;
        if (value & K230_SPI_STIG_CTRL_EXEC) {
            k230_spi_stig_exec(s);
        }
        break;

    case K230_SPI_IRQ_STATUS:
        s->regs[reg_addr] &= ~value;
        k230_spi_set_irq(s, 0);
        break;

    default:
        s->regs[reg_addr] = value;
        break;
    }
}

static const MemoryRegionOps k230_spi_reg_ops = {
    .read = k230_spi_reg_read,
    .write = k230_spi_reg_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
        .unaligned = false,
    },
};

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

    uint64_t result = 0;
    for (int i = 0; i < size && !fifo8_is_empty(&s->rx_fifo); i++) {
        result |= (uint64_t)fifo8_pop(&s->rx_fifo) << (i * 8);
    }

    return result;
}

static void k230_spi_xip_write(void *opaque, hwaddr addr, uint64_t value, unsigned int size)
{
    K230SpiState *s = K230_SPI(opaque);

    if (!s->xip_enabled || !s->direct_access_enabled) {
        qemu_log_mask(LOG_GUEST_ERROR, "K230 SPI XIP write while disabled\n");
        return;
    }

    if (s->regs[K230_SPI_CTRL / 4] & K230_SPI_CTRL_ADDR_REMAP) {
        addr += s->regs[K230_SPI_REMAP_ADDR / 4];
    }

    if (addr >= s->flash_size) {
        return;
    }

    uint8_t data[8] = {};
    for (int i = 0; i < size; i++) {
        data[i] = value >> (i * 8);
    }

    k230_spi_do_write(s, addr, data, size);
}

static const MemoryRegionOps k230_spi_xip_ops = {
    .read = k230_spi_xip_read,
    .write = k230_spi_xip_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void k230_spi_realize(DeviceState *dev, Error **errp)
{
    K230SpiState *s = K230_SPI(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    s->spi = ssi_create_bus(dev, "spi");
    sysbus_init_irq(sbd, &s->cs_line);

    fifo8_create(&s->rx_fifo, 256);
    fifo8_create(&s->tx_fifo, 256);

    s->flash_size = DEFAULT_FLASH_SIZE;
}

static void k230_spi_init(Object *obj)
{
    K230SpiState *s = K230_SPI(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &k230_spi_reg_ops, s,
                          TYPE_K230_SPI, K230_SPI_REG_SIZE);
    sysbus_init_mmio(sbd, &s->iomem);

    memory_region_init_io(&s->iomem_xip, obj, &k230_spi_xip_ops, s,
                          TYPE_K230_SPI "-xip", K230_SPI_XIP_SIZE);
    sysbus_init_mmio(sbd, &s->iomem_xip);

    sysbus_init_irq(sbd, &s->irq);
}

static const VMStateDescription vmstate_k230_spi = {
    .name = "k230.spi",
    .unmigratable = 1,
};

static void k230_spi_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = k230_spi_realize;
    device_class_set_legacy_reset(dc, k230_spi_reset);
    dc->vmsd = &vmstate_k230_spi;
    dc->desc = "K230 Octal SPI Controller";
}

static const TypeInfo k230_spi_type_info = {
    .name = TYPE_K230_SPI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(K230SpiState),
    .instance_init = k230_spi_init,
    .class_init = k230_spi_class_init,
};

static void k230_spi_register_types(void)
{
    type_register_static(&k230_spi_type_info);
}

type_init(k230_spi_register_types)