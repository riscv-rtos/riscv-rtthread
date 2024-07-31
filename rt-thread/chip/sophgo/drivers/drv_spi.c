/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-03-28     qiujingbao   first version
 * 2024/06/08     flyingcys    fix transmission failure
 */

#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>

#include "board.h"
#include "drv_spi.h"

#define DBG_LEVEL   DBG_LOG
#include <rtdbg.h>
#define LOG_TAG "drv.spi"

struct _device_spi
{
    struct rt_spi_bus spi_bus;
    struct dw_spi dws;
    char *device_name;
};

static struct _device_spi _spi_obj[] =
{
#ifdef BSP_USING_SPI0
    {
        .dws.regs = (void *)DW_SPI0_BASE,
        .dws.irq = DW_SPI0_IRQn,
        .dws.index = 0,
        .device_name = "spi0",
    },
#endif /* BSP_USING_SPI0 */
#ifdef BSP_USING_SPI1
    {
        .dws.regs = (void *)DW_SPI1_BASE,
        .dws.irq = DW_SPI1_IRQn,
        .dws.index = 0,
        .device_name = "spi1",
    },
#endif /* BSP_USING_SPI1 */
#ifdef BSP_USING_SPI2
    {
        .dws.regs = (void *)DW_SPI2_BASE,
        .dws.irq = DW_SPI2_IRQn,
        .dws.index = 0,
        .device_name = "spi2",
    },
#endif /* BSP_USING_SPI2 */
#ifdef BSP_USING_SPI3
    {
        .dws.regs = (void *)DW_SPI3_BASE,
        .dws.irq = DW_SPI3_IRQn,
        .dws.index = 0,
        .device_name = "spi3",
    },
#endif /* BSP_USING_SPI3 */
};

static rt_err_t spi_configure(struct rt_spi_device *device, struct rt_spi_configuration *cfg)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    RT_ASSERT(device->bus->parent.user_data != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    rt_err_t ret = RT_EOK;
    struct _device_spi *spi = (struct _device_spi *)device->bus->parent.user_data;
    struct dw_spi *dws = &spi->dws;

    rt_uint8_t mode;

    LOG_D("spi_configure input");

    /* set cs low when spi idle */
    writel(0, (void *)0x030001d0);

    if (cfg->mode & RT_SPI_SLAVE)
    {
        LOG_E("invalid mode: %d", cfg->mode);
        return -RT_EINVAL;
    }

    spi_reset_chip(dws);
    spi_hw_init(dws);
    spi_enable_chip(dws, 0);

    LOG_D("cfg->max_hz: %d", cfg->max_hz);
    dw_spi_set_clock(dws, SPI_REF_CLK, cfg->max_hz);

    LOG_D("cfg->data_width: %d", cfg->data_width);
    if (dw_spi_set_data_frame_len(dws, (uint32_t)cfg->data_width) < 0)
    {
        LOG_E("dw_spi_set_data_frame_len failed...\n");
        return -RT_ERROR;
    }

    LOG_D("cfg->mode: %08x", cfg->mode);
    switch (cfg->mode & RT_SPI_MODE_3)
    {
        case RT_SPI_MODE_0:
            mode = SPI_FORMAT_CPOL0_CPHA0;
            break;

        case RT_SPI_MODE_1:
            mode = SPI_FORMAT_CPOL0_CPHA1;
            break;

        case RT_SPI_MODE_2:
            mode = SPI_FORMAT_CPOL1_CPHA0;
            break;

        case RT_SPI_MODE_3:
            mode = SPI_FORMAT_CPOL1_CPHA1;
            break;

        default:
            LOG_E("spi configure mode error %x\n", cfg->mode);
            break;
    }

    dw_spi_set_polarity_and_phase(dws, mode);

    dw_spi_set_cs(dws, 1, 0);

    spi_enable_chip(dws, 1);

    return RT_EOK;
}

static rt_err_t dw_spi_transfer_one(struct dw_spi *dws, const void *tx_buf, void *rx_buf, uint32_t len, enum transfer_type  tran_type)
{
    uint8_t imask = 0;
    uint16_t txlevel = 0;

    dws->tx = NULL;
    dws->tx_end = NULL;
    dws->rx = NULL;
    dws->rx_end = NULL;

    if (tx_buf != NULL) {
        dws->tx = tx_buf;
        dws->tx_end = dws->tx + len;
    }

    if (rx_buf != NULL) {
        dws->rx = rx_buf;
        dws->rx_end = dws->rx + len;
    }

    dws->rx_len = len / dws->n_bytes;
    dws->tx_len = len / dws->n_bytes;

    spi_enable_chip(dws, 0);

    /* For poll mode just disable all interrupts */
    spi_mask_intr(dws, 0xff);

    /* set tran mode */
    set_tran_mode(dws);

    /* cs0 */
    dw_spi_set_cs(dws, true, 0);

    /* enable spi */
    spi_enable_chip(dws, 1);

    rt_hw_us_delay(10);

    if (tran_type == POLL_TRAN)
    {
        if (poll_transfer(dws) < 0)
            return -RT_ERROR;
    }
    else
    {
        return -RT_ENOSYS;
    }

    return RT_EOK;
}

static rt_ssize_t spi_xfer(struct rt_spi_device *device, struct rt_spi_message *message)
{
    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    RT_ASSERT(device->bus->parent.user_data != RT_NULL);
    RT_ASSERT(message != RT_NULL);

    struct _device_spi *spi = (struct _device_spi *)device->bus->parent.user_data;
    struct dw_spi *dws = &spi->dws;
    int32_t ret = 0;

    if (message->send_buf && message->recv_buf)
    {
        ret = dw_spi_transfer_one(dws, message->send_buf, message->recv_buf, message->length, POLL_TRAN);

    }
    else if (message->send_buf)
    {
        ret = dw_spi_transfer_one(dws, message->send_buf, RT_NULL, message->length, POLL_TRAN);

    }
    else if (message->recv_buf)
    {
        ret = dw_spi_transfer_one(dws, RT_NULL, message->recv_buf, message->length, POLL_TRAN);

    }

    return message->length;
}

static const struct rt_spi_ops _spi_ops =
{
    .configure = spi_configure,
    .xfer = spi_xfer,
};

static void rt_hw_spi_pinmux_config(void)
{
#ifdef BSP_USING_SPI0
#ifdef SPI0_SCK_USING_SD0_CLK
    PINMUX_CONFIG(SD0_CLK, SPI0_SCK);
#elif defined(SPI0_SCK_USING_PAD_MIPI_TXM2)
    PINMUX_CONFIG(PAD_MIPI_TXM2, SPI0_SCK);
#else
    LOG_E("SPI0_SCK pinmux config failed");
#endif

#ifdef SPI0_SDI_USING_SD0_D0
    PINMUX_CONFIG(SD0_D0, SPI0_SDI);
#elif defined(SPI0_SDI_USING_PAD_MIPI_TXP1)
    PINMUX_CONFIG(PAD_MIPI_TXP1, SPI0_SDI);
#else
    LOG_E("SPI0_SDI pinmux config failed");
#endif

#ifdef SPI0_SDO_USING_SD0_CMD
    PINMUX_CONFIG(SD0_CMD, SPI0_SDO);
#elif defined(SPI0_SDO_USING_PAD_MIPI_TXM1)
    PINMUX_CONFIG(PAD_MIPI_TXM1, SPI0_SDO);
#else
    LOG_E("SPI0_SDO pinmux config failed");
#endif

#ifdef SPI0_CSX_USING_SD0_D3
    PINMUX_CONFIG(SD0_D3, SPI0_CS_X);
#elif defined(SPI0_CSX_USING_PAD_MIPI_TXP2)
    PINMUX_CONFIG(PAD_MIPI_TXP2, SPI0_CS_X);
#else
    LOG_E("SPI0_CS_X pinmux config failed");
#endif
#endif /* BSP_USING_SPI0 */

#ifdef BSP_USING_SPI1
#ifdef SPI1_SCK_USING_MUX_SPI1_SCK
    PINMUX_CONFIG(MUX_SPI1_SCK, SPI1_SCK);
#elif defined(SPI1_SCK_USING_PAD_ETH_RXP___EPHY_TXN)
    PINMUX_CONFIG(PAD_ETH_RXP___EPHY_TXN, SPI1_SCK);
#else
    LOG_E("SPI1_SCK pinmux config failed");
#endif

#ifdef SPI1_SDI_USING_MUX_SPI1_MISO
    PINMUX_CONFIG(MUX_SPI1_MISO, SPI1_SDI);
#elif defined(SPI1_SDI_USING_PAD_ETH_TXM___EPHY_RXP)
    PINMUX_CONFIG(PAD_ETH_TXM___EPHY_RXP, SPI1_SDI);
#else
    LOG_E("SPI1_SDI pinmux config failed");
#endif

#ifdef SPI1_SDO_USING_MUX_SPI1_MOSI
    PINMUX_CONFIG(MUX_SPI1_MOSI, SPI1_SDO);
#elif defined(SPI1_SDO_USING_PAD_ETH_TXP___EPHY_RXN)
    PINMUX_CONFIG(PAD_ETH_TXP___EPHY_RXN, SPI1_SDO);
#else
    LOG_E("SPI1_SDO pinmux config failed");
#endif

#ifdef SPI1_CSX_USING_MUX_SPI1_CS
    PINMUX_CONFIG(MUX_SPI1_CS, SPI1_CS_X);
#elif defined(SPI1_CSX_USING_PAD_ETH_RXM___EPHY_TXP)
    PINMUX_CONFIG(PAD_ETH_RXM___EPHY_TXP, SPI1_CS_X);
#else
    LOG_E("SPI1_CS_X pinmux config failed");
#endif
#endif /* BSP_USING_SPI1 */

#ifdef BSP_USING_SPI2
#ifdef SPI2_SCK_USING_SD1_CLK
    PINMUX_CONFIG(SD1_CLK, SPI2_SCK);
#else
    LOG_E("SPI2_SCK pinmux config failed");
#endif

#ifdef SPI2_SDI_USING_MUX_SD1_D0
    PINMUX_CONFIG(SD1_D0, SPI2_SDI);
#else
    LOG_E("SPI2_SDI pinmux config failed");
#endif

#ifdef SPI2_SDO_USING_SD1_CMD
    PINMUX_CONFIG(SD1_CMD, SPI2_SDO);
#else
    LOG_E("SPI2_SDO pinmux config failed");
#endif

#ifdef SPI2_CSX_USING_SD1_D3
    PINMUX_CONFIG(SD1_D3, SPI2_CS_X);
#else
    LOG_E("SPI2_CS_X pinmux config failed");
#endif /* BSP_USING_SPI2 */
#endif

#ifdef BSP_USING_SPI3
#ifdef SPI3_SCK_USING_VIVO_D6
    PINMUX_CONFIG(VIVO_D6, SPI3_SCK);
#else
    LOG_E("SPI3_SDI pinmux config failed");
#endif

#ifdef SPI3_SDI_USING_VIVO_D7
    PINMUX_CONFIG(VIVO_D7, SPI3_SDI);
#else
    LOG_E("SPI3_SDI pinmux config failed");
#endif

#ifdef SPI3_SDO_USING_VIVO_D8
    PINMUX_CONFIG(VIVO_D8, SPI3_SDO);
#else
    LOG_E("SPI3_SDO pinmux config failed");
#endif

#ifdef SPI3_CSX_USING_VIVO_D5
    PINMUX_CONFIG(VIVO_D5, SPI3_CS_X);
#else
    LOG_E("SPI3_CS_X pinmux config failed");
#endif
#endif /* BSP_USING_SPI3 */
}

int rt_hw_spi_init(void)
{
    rt_err_t ret = RT_EOK;
    struct dw_spi *dws;

    rt_hw_spi_pinmux_config();

    for (rt_size_t i = 0; i < sizeof(_spi_obj) / sizeof(struct _device_spi); i++)
    {
        _spi_obj[i].spi_bus.parent.user_data = (void *)&_spi_obj[i];
        ret = rt_spi_bus_register(&_spi_obj[i].spi_bus, _spi_obj[i].device_name, &_spi_ops);
    }

    RT_ASSERT(ret == RT_EOK);

    return ret;
}
INIT_DEVICE_EXPORT(rt_hw_spi_init);
