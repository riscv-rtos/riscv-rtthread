/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023/06/25     flyingcys    first version
 */

#include <rtthread.h>
#include <stdio.h>

int main(void)
{
    extern void mpy_main(const char *filename);
	mpy_main(NULL);
    
    return 0;
}
