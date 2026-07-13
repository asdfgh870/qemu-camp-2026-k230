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
#define FIFO_DEPTH 256

static void k230_spi_update_status(K230SpiState *s);
static void k230_spi_set_irq(K230SpiState *s);
static void k230_spi_do_xip_read(K230SpiState *s, uint32_t addr, uint32_t len);

static void k230_spi_reset(DeviceState *dev)
{
    K230SpiState *s = K230_SPI(dev);

    memset(s->regs, 0, sizeof(s->regs));
    s->regs[K230_SPI_CTRLR0 / 4] = 0x00004007;
    s->regs[K230_SPI_SR / 4] = 0x00000006;
    s->regs[K230_SPI_IMR / 4] = 0x0000003f;
    s->regs[K230_SPI_SPI_CTRLR0 / 4] = 0x28000200;
    s->regs[K230_SPI_SSIC_VERSION_ID / 4] = 0x3130332a;
    s->regs[K230_SPI_IDR / 4] = 0xa1b2c3d5;

    fifo8_reset(&s->rx_fifo);
    fifo8_reset(&s->tx_fifo);

    s->xip_enabled = false;
    s->ssi_enabled = false;
}

static void k230_spi_update_status(K230SpiState *s)
{
    uint32_t sr = 0;

    if (fifo8_is_empty(&s->tx_fifo)) {
        sr |= K230_SPI_SR_TFE;
    }
    if (!fifo8_is_full(&s->tx_fifo)) {
        sr |= K230_SPI_SR_TNF;
    }
    if (!fifo8_is_empty(&s->rx_fifo)) {
        sr |= K230_SPI_SR_RNE;
    }
    if (fifo8_is_full(&s->rx_fifo)) {
        sr |= K230_SPI_SR_RFF;
    }

    s->regs[K230_SPI_SR / 4] = sr;
    s->regs[K230_SPI_TXFLR / 4] = fifo8_num_used(&s->tx_fifo);
    s->regs[K230_SPI_RXFLR / 4] = fifo8_num_used(&s->rx_fifo);
}

static void k230_spi_set_irq(K230SpiState *s)
{
    uint32_t imr = s->regs[K230_SPI_IMR / 4];
    uint32_t isr = s->regs[K230_SPI_ISR / 4];

    qemu_set_irq(s->irq, !!(isr & imr));
}

static void k230_spi_clear_interrupt(K230SpiState *s, uint32_t mask)
{
    s->regs[K230_SPI_RISR / 4] &= ~mask;
    s->regs[K230_SPI_ISR / 4] &= ~mask;
    k230_spi_set_irq(s);
}

static void k230_spi_trigger_interrupt(K230SpiState *s, uint32_t mask)
{
    s->regs[K230_SPI_RISR / 4] |= mask;
    s->regs[K230_SPI_ISR / 4] |= mask;
    k230_spi_set_irq(s);
}

static void k230_spi_select_cs(K230SpiState *s, bool select)
{
    qemu_set_irq(s->cs_line, !select);
}

static void k230_spi_do_transfer(K230SpiState *s)
{
    if (!s->ssi_enabled) {
        return;
    }

    uint32_t ctrlr0 = s->regs[K230_SPI_CTRLR0 / 4];
    uint32_t frf_ssic = extract32(ctrlr0, 11, 2);
    uint32_t dfs = extract32(ctrlr0, 0, 5);
    uint32_t tmod = extract32(ctrlr0, 7, 2);

    uint32_t ndf = s->regs[K230_SPI_CTRLR1 / 4] & 0xffff;
    uint32_t total_frames = ndf + 1;

    k230_spi_select_cs(s, true);

    while (total_frames > 0 && !fifo8_is_empty(&s->tx_fifo)) {
        uint32_t tx_data = fifo8_pop(&s->tx_fifo);

        if (frf_ssic == K230_SPI_CTRLR0_FRF_SPI_STD) {
            uint32_t rx_data = ssi_transfer(s->spi, tx_data);
            if (tmod != K230_SPI_CTRLR0_TMOD_TO) {
                if (!fifo8_is_full(&s->rx_fifo)) {
                    fifo8_push(&s->rx_fifo, rx_data);
                } else {
                    k230_spi_trigger_interrupt(s, K230_SPI_ISR_RXOIS);
                }
            }
        } else {
            int bits_per_frame = dfs + 1;
            int bytes_per_frame = (bits_per_frame + 7) / 8;

            uint8_t tx_buf[8] = {};
            uint8_t rx_buf[8] = {};

            tx_buf[0] = tx_data;
            if (bytes_per_frame > 1) {
                tx_buf[1] = tx_data >> 8;
            }
            if (bytes_per_frame > 2) {
                tx_buf[2] = tx_data >> 16;
            }
            if (bytes_per_frame > 3) {
                tx_buf[3] = tx_data >> 24;
            }

            for (int i = 0; i < bytes_per_frame; i++) {
                rx_buf[i] = ssi_transfer(s->spi, tx_buf[i]);
            }

            if (tmod != K230_SPI_CTRLR0_TMOD_TO) {
                uint32_t rx_data = rx_buf[0];
                if (bytes_per_frame > 1) {
                    rx_data |= (uint32_t)rx_buf[1] << 8;
                }
                if (bytes_per_frame > 2) {
                    rx_data |= (uint32_t)rx_buf[2] << 16;
                }
                if (bytes_per_frame > 3) {
                    rx_data |= (uint32_t)rx_buf[3] << 24;
                }

                if (!fifo8_is_full(&s->rx_fifo)) {
                    fifo8_push(&s->rx_fifo, rx_data);
                } else {
                    k230_spi_trigger_interrupt(s, K230_SPI_ISR_RXOIS);
                }
            }
        }

        total_frames--;
    }

    k230_spi_select_cs(s, false);
    k230_spi_update_status(s);

    uint32_t rx_thres = s->regs[K230_SPI_RXFTLR / 4] & 0xff;
    if (fifo8_num_used(&s->rx_fifo) >= rx_thres) {
        k230_spi_trigger_interrupt(s, K230_SPI_ISR_RXFI);
    }
}

static uint64_t k230_spi_reg_read(void *opaque, hwaddr addr, unsigned int size)
{
    K230SpiState *s = K230_SPI(opaque);
    uint32_t reg_addr = addr >> 2;

    if (reg_addr >= K230_SPI_REG_MAX) {
        return 0;
    }

    if (addr >= K230_SPI_DR && addr < K230_SPI_DR + 0x20) {
        if (!fifo8_is_empty(&s->rx_fifo)) {
            uint32_t data = fifo8_pop(&s->rx_fifo);
            k230_spi_update_status(s);
            return data;
        }
        return 0;
    }

    return s->regs[reg_addr];
}

static void k230_spi_reg_write(void *opaque, hwaddr addr, uint64_t value, unsigned int size)
{
    K230SpiState *s = K230_SPI(opaque);
    uint32_t reg_addr = addr >> 2;

    if (reg_addr >= K230_SPI_REG_MAX) {
        return;
    }

    if (!s->ssi_enabled && addr != K230_SPI_SSIENR && addr != K230_SPI_SSI_CTRL) {
        return;
    }

    switch (addr) {
    case K230_SPI_SSIENR:
        s->ssi_enabled = value & K230_SPI_SSIENR_SSI_EN;
        s->regs[reg_addr] = value;
        break;

    case K230_SPI_SSI_CTRL:
        if (value & K230_SPI_SSI_CTRL_SOFT_RST) {
            k230_spi_reset(DEVICE(s));
        }
        break;

    case K230_SPI_TXEICR:
        if (value & K230_SPI_TXEICR_TXEIC) {
            k230_spi_clear_interrupt(s, K230_SPI_ISR_TXEIS);
        }
        break;

    case K230_SPI_RXOICR:
        if (value & K230_SPI_RXOICR_RXOIC) {
            k230_spi_clear_interrupt(s, K230_SPI_ISR_RXOIS);
        }
        break;

    case K230_SPI_RXUICR:
        if (value & K230_SPI_RXUICR_RXUIC) {
            k230_spi_clear_interrupt(s, K230_SPI_ISR_RXUIS);
        }
        break;

    case K230_SPI_MSTICR:
        if (value & K230_SPI_MSTICR_MSTIC) {
            k230_spi_clear_interrupt(s, K230_SPI_ISR_MSTIS);
        }
        break;

    case K230_SPI_ICR:
        if (value & K230_SPI_ICR_ALLIC) {
            k230_spi_clear_interrupt(s, 0x3f);
        }
        break;

    case K230_SPI_SPI_CTRLR0:
        s->regs[reg_addr] = value;
        s->xip_enabled = extract32(value, 5, 1);
        break;

    default:
        if (addr >= K230_SPI_DR && addr < K230_SPI_DR + 0x20) {
            if (!fifo8_is_full(&s->tx_fifo)) {
                fifo8_push(&s->tx_fifo, value);
                k230_spi_update_status(s);

                uint32_t tmod = extract32(s->regs[K230_SPI_CTRLR0 / 4], 7, 2);
                if (tmod != K230_SPI_CTRLR0_TMOD_EP) {
                    k230_spi_do_transfer(s);
                }
            } else {
                k230_spi_trigger_interrupt(s, K230_SPI_ISR_TXOIS);
            }
            return;
        }

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

static void k230_spi_do_xip_read(K230SpiState *s, uint32_t addr, uint32_t len)
{
    if (!s->ssi_enabled || !s->xip_enabled) {
        return;
    }

    uint32_t spi_ctrlr0 = s->regs[K230_SPI_SPI_CTRLR0 / 4];
    bool xip_inst_en = extract32(spi_ctrlr0, 5, 1);
    bool xip_md_bit_en = extract32(spi_ctrlr0, 7, 1);
    uint32_t inst_l = extract32(spi_ctrlr0, 8, 8);
    uint32_t addr_l = extract32(spi_ctrlr0, 16, 8);

    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);

    if (xip_inst_en) {
        uint32_t incr_inst = s->regs[K230_SPI_XIP_INCR_INST / 4];
        for (int i = 0; i < inst_l; i++) {
            fifo8_push(&s->tx_fifo, incr_inst >> (i * 8));
        }
    }

    int addr_bytes = (addr_l + 7) / 8;
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

    if (xip_md_bit_en) {
        uint32_t mode_bits = s->regs[K230_SPI_XIP_MODE_BITS / 4];
        fifo8_push(&s->tx_fifo, mode_bits);
    }

    k230_spi_select_cs(s, true);
    k230_spi_do_transfer(s);

    fifo8_reset(&s->rx_fifo);
    for (int i = 0; i < len; i++) {
        fifo8_push(&s->tx_fifo, 0);
    }
    k230_spi_do_transfer(s);
    k230_spi_select_cs(s, false);
}

static uint64_t k230_spi_xip_read(void *opaque, hwaddr addr, unsigned int size)
{
    K230SpiState *s = K230_SPI(opaque);

    if (!s->ssi_enabled || !s->xip_enabled) {
        qemu_log_mask(LOG_GUEST_ERROR, "K230 SPI XIP read while disabled\n");
        return 0;
    }

    if (addr >= s->flash_size) {
        return 0;
    }

    k230_spi_do_xip_read(s, addr, size);

    uint64_t result = 0;
    for (int i = 0; i < size && !fifo8_is_empty(&s->rx_fifo); i++) {
        result |= (uint64_t)fifo8_pop(&s->rx_fifo) << (i * 8);
    }

    return result;
}

static void k230_spi_xip_write(void *opaque, hwaddr addr, uint64_t value, unsigned int size)
{
    K230SpiState *s = K230_SPI(opaque);

    if (!s->ssi_enabled || !s->xip_enabled) {
        qemu_log_mask(LOG_GUEST_ERROR, "K230 SPI XIP write while disabled\n");
        return;
    }

    if (addr >= s->flash_size) {
        return;
    }

    uint32_t spi_ctrlr0 = s->regs[K230_SPI_SPI_CTRLR0 / 4];
    bool xip_inst_en = extract32(spi_ctrlr0, 5, 1);
    uint32_t inst_l = extract32(spi_ctrlr0, 8, 8);
    uint32_t addr_l = extract32(spi_ctrlr0, 16, 8);

    fifo8_reset(&s->tx_fifo);

    fifo8_push(&s->tx_fifo, 0x06);
    k230_spi_select_cs(s, true);
    k230_spi_do_transfer(s);
    k230_spi_select_cs(s, false);

    fifo8_reset(&s->tx_fifo);

    if (xip_inst_en) {
        uint32_t write_incr_inst = s->regs[K230_SPI_XIP_WRITE_INCR_INST / 4];
        for (int i = 0; i < inst_l; i++) {
            fifo8_push(&s->tx_fifo, write_incr_inst >> (i * 8));
        }
    }

    int addr_bytes = (addr_l + 7) / 8;
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

    for (int i = 0; i < size; i++) {
        fifo8_push(&s->tx_fifo, value >> (i * 8));
    }

    k230_spi_select_cs(s, true);
    k230_spi_do_transfer(s);
    k230_spi_select_cs(s, false);
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

    fifo8_create(&s->rx_fifo, FIFO_DEPTH);
    fifo8_create(&s->tx_fifo, FIFO_DEPTH);

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
    dc->desc = "K230 Flash Memory Controller (Cadence SSIC)";
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
