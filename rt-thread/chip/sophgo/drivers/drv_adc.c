/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024/02/22     flyingcys    first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include "drv_adc.h"

#define DBG_LEVEL   DBG_LOG
#include <rtdbg.h>
#define LOG_TAG "DRV.ADC"

struct cvi_adc_dev
{
    struct rt_adc_device device;
    const char *name;
    rt_ubase_t base;
};

static struct cvi_adc_dev adc_dev_config[] =
{
    {
        .name = "adc1",
        .base = SARADC_BASE
    },
};

static rt_err_t _adc_enabled(struct rt_adc_device *device, rt_int8_t channel, rt_bool_t enabled)
{
    struct cvi_adc_dev *adc_dev = (struct cvi_adc_dev *)device->parent.user_data;
    uint32_t value;

    RT_ASSERT(adc_dev != RT_NULL);

    if (channel > SARADC_CH_MAX)
        return -RT_EINVAL;

    if (enabled)
    {
        //set channel
        cvi_set_saradc_ctrl(adc_dev->base, (rt_uint32_t)channel << (SARADC_CTRL_SEL_POS + 1));

        //set saradc clock cycle
        cvi_set_cyc(adc_dev->base);

        //start
        cvi_set_saradc_ctrl(adc_dev->base, SARADC_CTRL_START);
        LOG_D("enable saradc...");
    }
    else
    {
        cvi_reset_saradc_ctrl(adc_dev->base, (rt_uint32_t)channel << (SARADC_CTRL_SEL_POS + 1));
        LOG_D("disable saradc...");
    }
    return RT_EOK;
}

static rt_err_t _adc_convert(struct rt_adc_device *device, rt_int8_t channel, rt_uint32_t *value)
{
    struct cvi_adc_dev *adc_dev = (struct cvi_adc_dev *)device->parent.user_data;
    rt_uint32_t result;
    rt_uint32_t cnt = 0;

    RT_ASSERT(adc_dev != RT_NULL);

    if (channel > SARADC_CH_MAX)
        return -RT_EINVAL;

    while (cvi_get_saradc_status(adc_dev->base) & SARADC_STATUS_BUSY)
    {
        rt_thread_delay(10);
        LOG_D("wait saradc ready");
        cnt ++;
        if (cnt > 100)
            return -RT_ETIMEOUT;
    }

    result = mmio_read_32(adc_dev->base + SARADC_RESULT(channel - 1));
    if (result & SARADC_RESULT_VALID)
    {
        *value = result & SARADC_RESULT_MASK;
        LOG_D("saradc channel %d value: %04x", channel, *value);
    }
    else
    {
        LOG_E("saradc channel %d read failed. result:0x%04x", channel, result);
        return -RT_ERROR;
    }
    return RT_EOK;
}

static const struct rt_adc_ops _adc_ops =
{
    .enabled = _adc_enabled,
    .convert = _adc_convert,
};

static void rt_hw_adc_pinmux_config(void)
{
#ifdef BSP_USING_ADC1_ACTIVE
#ifdef ADC1_ACTIVE_USING_XGPIOB_3
	PINMUX_CONFIG(ADC1, XGPIOB_3);
#else
    LOG_E("ADC1 active pinmux config failed");
#endif
#endif

#ifdef BSP_USING_ADC2_ACTIVE
#ifdef ADC2_ACTIVE_USING_XGPIOB_6
	PINMUX_CONFIG(USB_VBUS_DET, XGPIOB_6);
#elif defined(ADC2_ACTIVE_USING_XGPIOB_2)
	PINMUX_CONFIG(ADC2, XGPIOB_2);
#else
    LOG_E("ADC2 active pinmux config failed");
#endif
#endif

#ifdef BSP_USING_ADC3_ACTIVE
#ifdef ADC3_ACTIVE_USING_XGPIOB_1
	PINMUX_CONFIG(ADC3, XGPIOB_1);
#else
    LOG_E("ADC3 active pinmux config failed");
#endif
#endif

#ifdef BSP_USING_ADC1_NODIE
#ifdef ADC1_NODIE_USING_PWR_GPIO2
    PINMUX_CONFIG(PWR_GPIO2, PWR_GPIO2);
#else
    LOG_E("ADC1 nodoie pinmux config failed");
#endif
#endif

#ifdef BSP_USING_ADC2_NODIE
#ifdef ADC2_NODIE_USING_PWR_GPIO1
    PINMUX_CONFIG(PWR_GPIO1, PWR_GPIO1);
#else
    LOG_E("ADC2 nodoie pinmux config failed");
#endif
#endif

#ifdef BSP_USING_ADC3_NODIE
#ifdef ADC3_NODIE_USING_PWR_VBAT_DET
    PINMUX_CONFIG(PWR_VBAT_DET, PWR_VBAT_DET);
#else
    LOG_E("ADC3 nodoie pinmux config failed");]
#endif
#endif
}

int rt_hw_adc_init(void)
{
    rt_uint8_t i;

    rt_hw_adc_pinmux_config();

    for (i = 0; i < sizeof(adc_dev_config) / sizeof(adc_dev_config[0]); i++)
    {
        cvi_do_calibration(adc_dev_config[i].base);
    }

    for (i = 0; i < sizeof(adc_dev_config) / sizeof(adc_dev_config[0]); i++)
    {
        if (rt_hw_adc_register(&adc_dev_config[i].device, adc_dev_config[i].name, &_adc_ops, &adc_dev_config[i]) != RT_EOK)
        {
            LOG_E("%s register failed!", adc_dev_config[i].name);
            return -RT_ERROR;
        }
    }

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_adc_init);
