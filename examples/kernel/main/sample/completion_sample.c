/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-12     chenyingchun the first version
 */

/*
 * Program List: Completion Example
 *
 * The program initializes thread1, thread2, and a completion object.
 * Thread1 waits for thread2 to send a completion signal.
*/

#include <rtthread.h>
#include <rtdevice.h>

#define THREAD_PRIORITY     EXAMPLE_THREAD_PRIORITY
#define THREAD_TIMESLICE    EXAMPLE_THREAD_TIMESLICE

/* Completion control block */
static struct rt_completion completion;

#ifdef rt_align
rt_align(RT_ALIGN_SIZE)
#else
ALIGN(RT_ALIGN_SIZE)
#endif
static char thread1_stack[EXAMPLE_THREAD_STACK_SIZE];
static struct rt_thread thread1;
static char thread2_stack[EXAMPLE_THREAD_STACK_SIZE];
static struct rt_thread thread2;

/* Entry function for thread1 */
static void thread1_completion_wait(void *param)
{
    /* Wait for completion */
    rt_kprintf("thread1: completion is waiting\n");
    rt_completion_wait(&completion, RT_WAITING_FOREVER);
    rt_kprintf("thread1: completion waiting done\n");
    rt_kprintf("thread1 leave.\n");
}

/* Entry function for thread2 */
static void thread2_completion_done(void *param)
{
    rt_kprintf("thread2: completion done\n");
    rt_completion_done(&completion);
    rt_kprintf("thread2 leave.\n");
}

int completion_sample(void)
{
    /* Initialize the completion object */
    rt_completion_init(&completion);

    rt_thread_init(&thread1,
                   "thread1",
                   thread1_completion_wait,
                   RT_NULL,
                   &thread1_stack[0],
                   sizeof(thread1_stack),
                   THREAD_PRIORITY - 1, THREAD_TIMESLICE);
    rt_thread_startup(&thread1);

    rt_thread_init(&thread2,
                   "thread2",
                   thread2_completion_done,
                   RT_NULL,
                   &thread2_stack[0],
                   sizeof(thread2_stack),
                   THREAD_PRIORITY, THREAD_TIMESLICE);
    rt_thread_startup(&thread2);

    return 0;
}

/* Export to the msh command list */
MSH_CMD_EXPORT(completion_sample, completion sample);
