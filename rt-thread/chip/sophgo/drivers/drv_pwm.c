/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024/02/19     flyingcys    first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "drv_pwm.h"

#define DBG_LEVEL   DBG_LOG
#include <rtdbg.h>
#define LOG_TAG "DRV.PWM"

struct cvi_pwm_dev
{
    struct rt_device_pwm device;
    const char *name;
    rt_ubase_t reg_base;
};

static const uint64_t count_unit = 100000000;  // 100M count per second
static const uint64_t NSEC_COUNT = 1000000000;  // ns

static void cvi_pwm_set_config(rt_ubase_t reg_base, struct rt_pwm_configuration *cfg)
{
    unsigned long long duty_clk, period_clk;

    cvi_pwm_set_polarity_high_ch(reg_base, (cfg->channel & PWM_MAX_CH));

    duty_clk = (cfg->pulse * count_unit) / NSEC_COUNT;
    cvi_pwm_set_high_period_ch(reg_base, (cfg->channel & PWM_MAX_CH), duty_clk);

    period_clk = (cfg->period * count_unit) / NSEC_COUNT;
    cvi_pwm_set_period_ch(reg_base, (cfg->channel & PWM_MAX_CH), period_clk);

    cvi_pwm_output_en_ch(reg_base, cfg->channel & PWM_MAX_CH);
}

static void cvi_pwm_get_config(rt_ubase_t reg_base, struct rt_pwm_configuration *cfg)
{
    unsigned long long duty_clk, period_clk;

    duty_clk = cvi_pwm_get_high_period_ch(reg_base, (cfg->channel & PWM_MAX_CH));
    cfg->pulse = duty_clk * NSEC_COUNT / count_unit;

    period_clk = cvi_pwm_get_period_ch(reg_base, (cfg->channel & PWM_MAX_CH));
    cfg->period = period_clk * NSEC_COUNT / count_unit;
}

static rt_err_t _pwm_control(struct rt_device_pwm *device, int cmd, void *arg)
{
    struct rt_pwm_configuration *cfg = (struct rt_pwm_configuration *)arg;
    struct cvi_pwm_dev *pwm_dev = (struct cvi_pwm_dev *)device->parent.user_data;
    unsigned long long duty_clk, period_clk;
    const uint64_t count_unit = 100000000;  // 100M count per second
    const uint64_t NSEC_COUNT = 1000000000;  // ns

    if (cfg->channel >= PWM_CHANNEL_NUM)
        return -RT_EINVAL;

    switch (cmd)
    {
        case PWM_CMD_ENABLE:
            cvi_pwm_start_en_ch(pwm_dev->reg_base, cfg->channel & PWM_MAX_CH);
        break;

        case PWM_CMD_DISABLE:
            cvi_pwm_start_dis_ch(pwm_dev->reg_base, cfg->channel & PWM_MAX_CH);
        break;

        case PWM_CMD_SET:
            cvi_pwm_set_config(pwm_dev->reg_base, cfg);
        break;

        case PWM_CMD_GET:
            cvi_pwm_get_config(pwm_dev->reg_base, cfg);
        break;

        case PWM_CMD_SET_PERIOD:
            period_clk = (cfg->period * count_unit) / NSEC_COUNT;
            cvi_pwm_set_period_ch(pwm_dev->reg_base, (cfg->channel & PWM_MAX_CH), period_clk);
        break;

        case PWM_CMD_SET_PULSE:
            duty_clk = (cfg->pulse * count_unit) / NSEC_COUNT;
            cvi_pwm_set_high_period_ch(pwm_dev->reg_base, (cfg->channel & PWM_MAX_CH), duty_clk);
        break;

        default:
        LOG_D("cmd: %x channel: %d period: %d pulse: %d", cmd, cfg->channel, cfg->period, cfg->pulse);
        break;
    }

    return RT_EOK;
}

const static struct rt_pwm_ops cvi_pwm_ops =
{
    .control = &_pwm_control
};

static struct cvi_pwm_dev cvi_pwm[] =
{
#if defined(BSP_USING_PWM0) || defined(BSP_USING_PWM1) || defined(BSP_USING_PWM2) || defined(BSP_USING_PWM3)
    {
        .name = "pwm0",
        .reg_base = CVI_PWM0_BASE,
    },
#endif

#if defined(BSP_USING_PWM4) || defined(BSP_USING_PWM5) || defined(BSP_USING_PWM6) || defined(BSP_USING_PWM7)
    {
        .name = "pwm1",
        .reg_base = CVI_PWM1_BASE,
    },
#endif

#if defined(BSP_USING_PWM8) || defined(BSP_USING_PWM8) || defined(BSP_USING_PWM10) || defined(BSP_USING_PWM11)
    {
        .name = "pwm2",
        .reg_base = CVI_PWM2_BASE,
    },
#endif

#if defined(BSP_USING_PWM12) || defined(BSP_USING_PWM13) || defined(BSP_USING_PWM14) || defined(BSP_USING_PWM15)
    {
        .name = "pwm3",
        .reg_base = CVI_PWM3_BASE,
    },
#endif
};


static void rt_hw_pwm_pinmux_config(void)
{
#ifdef BSP_USING_PWM0
#ifdef PWM0_USING_PWM0_BUCK
    PINMUX_CONFIG(PWM0_BUCK, PWM_0);
#else
    LOG_E("PWM0 pinmux config failed");
#endif
#endif /* BSP_USING_PWM0 */

#ifdef BSP_USING_PWM1
#ifdef PWM1_USING_GPIO_RTX___EPHY_RTX
    PINMUX_CONFIG(GPIO_RTX___EPHY_RTX, PWM_1);
#elif defined(PWM1_USING_VIVO_D10)
    PINMUX_CONFIG(VIVO_D10, PWM_1);
#else
    LOG_E("PWM1 pinmux config failed");
#endif
#endif /* BSP_USING_PWM1 */

#ifdef BSP_USING_PWM2
#ifdef PWM2_USING_VIVO_D2
    PINMUX_CONFIG(VIVO_D2, PWM_2);
#elif defined(PWM2_USING_CLK32K)
    PINMUX_CONFIG(CLK32K, PWM_2);
#elif defined(PWM2_USING_GPIO_ZQ___PAD_ZQ)
    PINMUX_CONFIG(GPIO_ZQ___PAD_ZQ, PWM_2);
#elif defined(PWM2_USING_VIVO_D9)
    PINMUX_CONFIG(VIVO_D9, PWM_2);
#else
    LOG_E("PWM2 pinmux config failed");
#endif
#endif /* BSP_USING_PWM2 */

#ifdef BSP_USING_PWM3
#ifdef PWM3_USING_ADC1
    PINMUX_CONFIG(ADC1, PWM_3);
#elif defined(PWM3_USING_VIVO_D1)
    PINMUX_CONFIG(VIVO_D1, PWM_3);
#elif defined(PWM3_USING_VIVO_D10)
    PINMUX_CONFIG(VIVO_D8, PWM_3);
#elif defined(PWM3_USING_CLK25M)
    PINMUX_CONFIG(CLK25M, PWM_3);
#else
    LOG_E("PWM3 pinmux config failed");
#endif
#endif /* BSP_USING_PWM3 */

#ifdef BSP_USING_PWM4
#ifdef PWM4_USING_UART0_TX
    PINMUX_CONFIG(UART0_TX, PWM_4);
#elif defined(PWM4_USING_USB_VBUS_DET)
    PINMUX_CONFIG(USB_VBUS_DET, PWM_4);
#elif defined(PWM4_USING_SD1_D3)
    PINMUX_CONFIG(SD1_D3, PWM_4);
#else
    LOG_E("PWM4 pinmux config failed");
#endif
#endif /* BSP_USING_PWM4 */

#ifdef BSP_USING_PWM5
#ifdef PWM5_USING_UART0_RX
    PINMUX_CONFIG(UART0_RX, PWM_5);
#elif defined(PWM5_USING_SD1_D2)
    PINMUX_CONFIG(SD1_D2, PWM_5);
#else
    LOG_E("PWM5 pinmux config failed");
#endif
#endif /* BSP_USING_PWM5 */

#ifdef BSP_USING_PWM6
#ifdef PWM6_USING_JTAG_CPU_TCK
    PINMUX_CONFIG(JTAG_CPU_TCK, PWM_6);
#elif defined(PWM6_USING_SD1_D1)
    PINMUX_CONFIG(SD1_D1, PWM_6);
#else
    LOG_E("PWM6 pinmux config failed");
#endif
#endif /* BSP_USING_PWM6 */

#ifdef BSP_USING_PWM7
#ifdef PWM7_USING_JTAG_CPU_TMS
    PINMUX_CONFIG(JTAG_CPU_TMS, PWM_7);
#elif defined(PWM7_USING_SD1_D0)
    PINMUX_CONFIG(SD1_D0, PWM_7);
#else
    LOG_E("PWM7 pinmux config failed");
#endif
#endif /* BSP_USING_PWM7 */

#ifdef BSP_USING_PWM8
#ifdef PWM8_USING_PWR_GPIO0
    PINMUX_CONFIG(PWR_GPIO0, PWM_8);
#elif defined(PWM8_USING_MUX_SPI1_MOSI)
    PINMUX_CONFIG(MUX_SPI1_MOSI, PWM_8);
#elif defined(PWM8_USING_SD1_CMD)
    PINMUX_CONFIG(SD1_CMD, PWM_8);
#elif defined(PWM8_USING_UART2_RTS)
    PINMUX_CONFIG(UART2_RTS, PWM_8);
#elif defined(PWM8_USING_PAD_MIPI_TXM2)
    PINMUX_CONFIG(PAD_MIPI_TXM2, PWM_8);
#else
    LOG_E("PWM8 pinmux config failed");
#endif
#endif /* BSP_USING_PWM8 */

#ifdef BSP_USING_PWM9
#ifdef PWM9_USING_PWR_GPIO1
    PINMUX_CONFIG(PWR_GPIO1, PWM_9);
#elif defined(PWM9_USING_MUX_SPI1_MISO)
    PINMUX_CONFIG(MUX_SPI1_MISO, PWM_9);
#elif defined(PWM9_USING_SD1_CLK)
    PINMUX_CONFIG(SD1_CLK, PWM_9);
#elif defined(PWM9_USING_UART2_CTS)
    PINMUX_CONFIG(UART2_CTS, PWM_9);
#elif defined(PWM9_USING_PAD_MIPI_TXP2)
    PINMUX_CONFIG(PAD_MIPI_TXP2, PWM_9);    
#else
    LOG_E("PWM9 pinmux config failed");
#endif
#endif /* BSP_USING_PWM9 */

#ifdef BSP_USING_PWM10
#ifdef PWM10_USING_SD1_GPIO1
    PINMUX_CONFIG(SD1_GPIO1, PWM_10);
#elif defined(PWM10_USING_PWR_GPIO2)
    PINMUX_CONFIG(PWR_GPIO2, PWM_10);
#elif defined(PWM10_USING_MUX_SPI1_SCK)
    PINMUX_CONFIG(MUX_SPI1_SCK, PWM_10);
#elif defined(PWM10_USING_UART2_RX)
    PINMUX_CONFIG(UART2_RX, PWM_10);
#elif defined(PWM10_USING_SD0_D3)
    PINMUX_CONFIG(SD0_D3, PWM_10);
#elif defined(PWM10_USING_PAD_MIPI_TXM1)
    PINMUX_CONFIG(PAD_MIPI_TXM1, PWM_10);    
#else
    LOG_E("PWM10 pinmux config failed");
#endif
#endif /* BSP_USING_PWM10 */

#ifdef BSP_USING_PWM11
#ifdef PWM11_USING_SD1_GPIO0
    PINMUX_CONFIG(SD1_GPIO0, PWM_11);
#elif defined(PWM11_USING_MUX_SPI1_CS)
    PINMUX_CONFIG(MUX_SPI1_CS, PWM_11);
#elif defined(PWM11_USING_UART2_TX)
    PINMUX_CONFIG(UART2_TX, PWM_11);
#elif defined(PWM11_USING_SD0_D2)
    PINMUX_CONFIG(SD0_D2, PWM_11);
#elif defined(PWM11_USING_PAD_MIPI_TXP1)
    PINMUX_CONFIG(PAD_MIPI_TXP1, PWM_11);
#else
    LOG_E("PWM11 pinmux config failed");
#endif
#endif /* BSP_USING_PWM11 */

#ifdef BSP_USING_PWM12
#ifdef PWM12_USING_ADC3
    PINMUX_CONFIG(ADC3, PWM_12);
#elif defined(PWM12_USING_PAD_ETH_TXM___EPHY_RXP)
    PINMUX_CONFIG(PAD_ETH_TXM___EPHY_RXP, PWM_12);
#elif defined(PWM12_USING_SD0_D1)
    PINMUX_CONFIG(SD0_D1, PWM_12);
#elif defined(PWM12_USING_PAD_MIPI_TXM4)
    PINMUX_CONFIG(PAD_MIPI_TXM4, PWM_12);
#else
    LOG_E("PWM12 pinmux config failed");
#endif
#endif /* BSP_USING_PWM12 */

#ifdef BSP_USING_PWM13
#ifdef PWM13_USING_ADC2
    PINMUX_CONFIG(ADC2, PWM_13);
#elif defined(PWM13_USING_PAD_ETH_TXP___EPHY_RXN)
    PINMUX_CONFIG(PAD_ETH_TXP___EPHY_RXN, PWM_13);
#elif defined(PWM13_USING_SD0_D0)
    PINMUX_CONFIG(SD0_D0, PWM_13);
#elif defined(PWM13_USING_PAD_MIPI_TXP4)
    PINMUX_CONFIG(PAD_MIPI_TXP4, PWM_13);   
#else
    LOG_E("PWM13 pinmux config failed");
#endif
#endif /* BSP_USING_PWM13 */

#ifdef BSP_USING_PWM14
#ifdef PWM14_USING_PAD_ETH_RXM___EPHY_TXP
    PINMUX_CONFIG(PAD_ETH_RXM___EPHY_TXP, PWM_14);
#elif defined(PWM14_USING_IIC2_SCL)
    PINMUX_CONFIG(IIC2_SCL, PWM_14);
#elif defined(PWM14_USING_SD0_CMD)
    PINMUX_CONFIG(SD0_CMD, PWM_14);
#elif defined(PWM14_USING_PAD_MIPI_TXM3)
    PINMUX_CONFIG(PAD_MIPI_TXM3, PWM_14);
#elif defined(PWM14_USING_PAD_MIPI_TXM0)
    PINMUX_CONFIG(PAD_MIPI_TXM0, PWM_14);    
#else
    LOG_E("PWM14 pinmux config failed");
#endif
#endif /* BSP_USING_PWM14 */

#ifdef BSP_USING_PWM15
#ifdef PWM15_USINGPAD_ETH_RXP___EPHY_TXN
    PINMUX_CONFIG(PAD_ETH_RXP___EPHY_TXN, PWM_15);
#elif defined(PWM15_USING_IIC2_SDA)
    PINMUX_CONFIG(IIC2_SDA, PWM_15);
#elif defined(PWM15_USING_SD0_CLK)
    PINMUX_CONFIG(SD0_CLK, PWM_15);
#elif defined(PWM15_USING_PAD_MIPI_TXP3)
    PINMUX_CONFIG(PAD_MIPI_TXP3, PWM_15);
#elif defined(PWM15_USING_PAD_MIPI_TXP0)
    PINMUX_CONFIG(PAD_MIPI_TXP0, PWM_15);
#else
    LOG_E("PWM15 pinmux config failed");
#endif
#endif /* BSP_USING_PWM15 */
}

int rt_hw_pwm_init(void)
{
    int result = RT_EOK;
    uint8_t i;

    rt_hw_pwm_pinmux_config();

    for (i = 0; i < sizeof(cvi_pwm) / sizeof(cvi_pwm[0]); i++)
    {
        result = rt_device_pwm_register(&cvi_pwm[i].device, cvi_pwm[i].name, &cvi_pwm_ops, &cvi_pwm[i]);
        if (result != RT_EOK)
        {
            LOG_E("device %s register failed", cvi_pwm[i].name);
            return -RT_ERROR;
        }
    }
    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_pwm_init);
