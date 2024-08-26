/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024/06/25     flyingcys    first version
 */

#include <rtthread.h>
#include <stdio.h>
#include <drivers/pin.h>

#ifndef EXAMPLE_BLINK_PIN
#define EXAMPLE_BLINK_PIN     "E2" /* Onboard LED pins */
#endif

int main(void)
{
    /* LED pin: C24 */
    rt_uint16_t led = rt_pin_get(EXAMPLE_BLINK_PIN);

    /* set LED pin mode to output */
    rt_pin_mode(led, PIN_MODE_OUTPUT);

    while (1)
    {
        rt_pin_write(led, PIN_HIGH);

        rt_thread_mdelay(500);

        rt_pin_write(led, PIN_LOW);

        rt_thread_mdelay(500);
    }
    return 0;
}
