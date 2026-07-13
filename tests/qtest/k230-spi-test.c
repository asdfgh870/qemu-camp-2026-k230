/*
 * QTest testcase for K230 SPI Controller (Cadence SSIC)
 *
 * Copyright (c) 2025 Chao Liu <chao.liu.zevorn@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/bitops.h"
#include "libqtest.h"
#include "hw/riscv/k230_spi.h"

#define K230_SPI_BASE    0x91584000
#define K230_SPI_XIP_BASE 0xC0000000

static void test_ctrlr0_register(void)
{
    QTestState *qts = qtest_init("-machine k230");

    uint32_t ctrlr0 = qtest_readl(qts, K230_SPI_BASE + K230_SPI_CTRLR0);
    g_assert_cmphex(ctrlr0, ==, 0x00004007);

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SSIENR, K230_SPI_SSIENR_SSI_EN);
    qtest_writel(qts, K230_SPI_BASE + K230_SPI_CTRLR0, 0x00000000);
    g_assert_cmphex(qtest_readl(qts, K230_SPI_BASE + K230_SPI_CTRLR0), ==, 0x00000000);

    qtest_quit(qts);
}

static void test_ssienr_register(void)
{
    QTestState *qts = qtest_init("-machine k230");

    uint32_t ssienr = qtest_readl(qts, K230_SPI_BASE + K230_SPI_SSIENR);
    g_assert_cmphex(ssienr & K230_SPI_SSIENR_SSI_EN, ==, 0);

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SSIENR, K230_SPI_SSIENR_SSI_EN);
    ssienr = qtest_readl(qts, K230_SPI_BASE + K230_SPI_SSIENR);
    g_assert_cmphex(ssienr & K230_SPI_SSIENR_SSI_EN, ==, K230_SPI_SSIENR_SSI_EN);

    qtest_quit(qts);
}

static void test_status_register(void)
{
    QTestState *qts = qtest_init("-machine k230");

    uint32_t sr = qtest_readl(qts, K230_SPI_BASE + K230_SPI_SR);
    g_assert_cmphex(sr & (K230_SPI_SR_TFE | K230_SPI_SR_TNF), !=, 0);

    qtest_quit(qts);
}

static void test_imr_register(void)
{
    QTestState *qts = qtest_init("-machine k230");

    uint32_t imr = qtest_readl(qts, K230_SPI_BASE + K230_SPI_IMR);
    g_assert_cmphex(imr, ==, 0x0000003f);

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SSIENR, K230_SPI_SSIENR_SSI_EN);
    qtest_writel(qts, K230_SPI_BASE + K230_SPI_IMR, 0x00000000);
    g_assert_cmphex(qtest_readl(qts, K230_SPI_BASE + K230_SPI_IMR), ==, 0x00000000);

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_IMR, 0x0000003f);
    g_assert_cmphex(qtest_readl(qts, K230_SPI_BASE + K230_SPI_IMR), ==, 0x0000003f);

    qtest_quit(qts);
}

static void test_spi_ctrlr0_register(void)
{
    QTestState *qts = qtest_init("-machine k230");

    uint32_t spi_ctrlr0 = qtest_readl(qts, K230_SPI_BASE + K230_SPI_SPI_CTRLR0);
    g_assert_cmphex(spi_ctrlr0, ==, 0x28000200);

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SSIENR, K230_SPI_SSIENR_SSI_EN);
    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SPI_CTRLR0, 0x00000020);
    g_assert_cmphex(qtest_readl(qts, K230_SPI_BASE + K230_SPI_SPI_CTRLR0), ==, 0x00000020);

    qtest_quit(qts);
}

static void test_ssic_version_register(void)
{
    QTestState *qts = qtest_init("-machine k230");

    uint32_t version = qtest_readl(qts, K230_SPI_BASE + K230_SPI_SSIC_VERSION_ID);
    g_assert_cmphex(version, ==, 0x3130332a);

    qtest_quit(qts);
}

static void test_idr_register(void)
{
    QTestState *qts = qtest_init("-machine k230");

    uint32_t idr = qtest_readl(qts, K230_SPI_BASE + K230_SPI_IDR);
    g_assert_cmphex(idr, ==, 0xa1b2c3d5);

    qtest_quit(qts);
}

static void test_xip_mode(void)
{
    QTestState *qts = qtest_init("-machine k230");

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SSIENR, K230_SPI_SSIENR_SSI_EN);
    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SPI_CTRLR0, 0x00000020);

    uint32_t spi_ctrlr0 = qtest_readl(qts, K230_SPI_BASE + K230_SPI_SPI_CTRLR0);
    g_assert_cmphex(spi_ctrlr0 & K230_SPI_SPI_CTRLR0_XIP_INST_EN, ==, K230_SPI_SPI_CTRLR0_XIP_INST_EN);

    uint8_t data = qtest_readb(qts, K230_SPI_XIP_BASE);
    (void)data;

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SPI_CTRLR0, 0x28000200);
    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SSIENR, 0);

    qtest_quit(qts);
}

static void test_interrupt_clear(void)
{
    QTestState *qts = qtest_init("-machine k230");

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SSIENR, K230_SPI_SSIENR_SSI_EN);

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_ICR, K230_SPI_ICR_ALLIC);

    uint32_t isr = qtest_readl(qts, K230_SPI_BASE + K230_SPI_ISR);
    g_assert_cmphex(isr, ==, 0);

    uint32_t risr = qtest_readl(qts, K230_SPI_BASE + K230_SPI_RISR);
    g_assert_cmphex(risr, ==, 0);

    qtest_quit(qts);
}

static void test_fifo_registers(void)
{
    QTestState *qts = qtest_init("-machine k230");

    uint32_t txflr = qtest_readl(qts, K230_SPI_BASE + K230_SPI_TXFLR);
    g_assert_cmphex(txflr, ==, 0);

    uint32_t rxflr = qtest_readl(qts, K230_SPI_BASE + K230_SPI_RXFLR);
    g_assert_cmphex(rxflr, ==, 0);

    qtest_quit(qts);
}

static void test_dfs_field(void)
{
    QTestState *qts = qtest_init("-machine k230");

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SSIENR, K230_SPI_SSIENR_SSI_EN);

    uint32_t dfs_values[] = {0x7, 0xF, 0x1F};
    uint32_t expected_bits[] = {8, 16, 32};

    for (int i = 0; i < sizeof(dfs_values) / sizeof(dfs_values[0]); i++) {
        uint32_t ctrlr0 = (K230_SPI_CTRLR0_FRF_SPI_OCTAL << 11) | dfs_values[i];
        qtest_writel(qts, K230_SPI_BASE + K230_SPI_CTRLR0, ctrlr0);

        uint32_t read_back = qtest_readl(qts, K230_SPI_BASE + K230_SPI_CTRLR0);
        uint32_t dfs_read = extract32(read_back, 0, 5);
        uint32_t bits_per_frame = dfs_read + 1;

        g_assert_cmphex(bits_per_frame, ==, expected_bits[i]);
    }

    qtest_writel(qts, K230_SPI_BASE + K230_SPI_CTRLR0, 0x00004007);
    qtest_writel(qts, K230_SPI_BASE + K230_SPI_SSIENR, 0);

    qtest_quit(qts);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    qtest_add_func("/k230-spi/ctrlr0_register", test_ctrlr0_register);
    qtest_add_func("/k230-spi/ssienr_register", test_ssienr_register);
    qtest_add_func("/k230-spi/status_register", test_status_register);
    qtest_add_func("/k230-spi/imr_register", test_imr_register);
    qtest_add_func("/k230-spi/spi_ctrlr0_register", test_spi_ctrlr0_register);
    qtest_add_func("/k230-spi/ssic_version_register", test_ssic_version_register);
    qtest_add_func("/k230-spi/idr_register", test_idr_register);
    qtest_add_func("/k230-spi/xip_mode", test_xip_mode);
    qtest_add_func("/k230-spi/interrupt_clear", test_interrupt_clear);
    qtest_add_func("/k230-spi/fifo_registers", test_fifo_registers);
    qtest_add_func("/k230-spi/dfs_field", test_dfs_field);

    return g_test_run();
}
