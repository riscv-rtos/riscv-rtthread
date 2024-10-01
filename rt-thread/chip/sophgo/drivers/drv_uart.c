/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023/06/25     flyingcys    first version
 */
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include "board.h"
#include "drv_uart.h"

#define DBG_TAG "DRV.UART"
#define DBG_LVL DBG_WARNING
#include <rtdbg.h>

/*
 * Divide positive or negative dividend by positive divisor and round
 * to closest integer. Result is undefined for negative divisors and
 * for negative dividends if the divisor variable type is unsigned.
 */
#define DIV_ROUND_CLOSEST(x, divisor)(          \
{                           \
    typeof(x) __x = x;              \
    typeof(divisor) __d = divisor;          \
    (((typeof(x))-1) > 0 ||             \
     ((typeof(divisor))-1) > 0 || (__x) > 0) ?  \
        (((__x) + ((__d) / 2)) / (__d)) :   \
        (((__x) - ((__d) / 2)) / (__d));    \
}                           \
)

#define BOTH_EMPTY (UART_LSR_TEMT | UART_LSR_THRE)

struct hw_uart_device
{
    rt_ubase_t hw_base;
    rt_uint32_t irqno;
};

#define BSP_DEFINE_UART_DEVICE(no)                  \
static struct hw_uart_device _uart##no##_device =   \
{                                                   \
    UART##no##_BASE,                           \
    UART##no##_IRQ                                  \
};                                                  \
static struct rt_serial_device _serial##no;

#ifdef BSP_USING_UART0
BSP_DEFINE_UART_DEVICE(0);
#endif

#ifdef BSP_USING_UART1
BSP_DEFINE_UART_DEVICE(1);
#endif

#ifdef BSP_USING_UART2
BSP_DEFINE_UART_DEVICE(2);
#endif

#ifdef BSP_USING_UART3
BSP_DEFINE_UART_DEVICE(3);
#endif

#ifdef BSP_USING_UART4
BSP_DEFINE_UART_DEVICE(4);
#endif

rt_inline rt_uint32_t dw8250_read32(rt_ubase_t addr, rt_ubase_t offset)
{
    return *((volatile rt_uint32_t *)(addr + (offset << UART_REG_SHIFT)));
}

rt_inline void dw8250_write32(rt_ubase_t addr, rt_ubase_t offset, rt_uint32_t value)
{
    *((volatile rt_uint32_t *)(addr + (offset << UART_REG_SHIFT))) = value;

    if (offset == UART_LCR)
    {
        int tries = 1000;

        /* Make sure LCR write wasn't ignored */
        while (tries--)
        {
            unsigned int lcr = dw8250_read32(addr, UART_LCR);

            if ((value & ~UART_LCR_STKP) == (lcr & ~UART_LCR_STKP))
            {
                return;
            }

            dw8250_write32(addr, UART_FCR, UART_FCR_DEFVAL);
            dw8250_read32(addr, UART_RX);

            *((volatile rt_uint32_t *)(addr + (offset << UART_REG_SHIFT))) = value;
        }
    }
}

static void dw8250_uart_setbrg(rt_ubase_t addr, int baud_divisor)
{
    /* to keep serial format, read lcr before writing BKSE */
    int lcr_val = dw8250_read32(addr, UART_LCR) & ~UART_LCR_BKSE;

    dw8250_write32(addr, UART_LCR, UART_LCR_BKSE | lcr_val);
    dw8250_write32(addr, UART_DLL, baud_divisor & 0xff);

    dw8250_write32(addr, UART_DLM, (baud_divisor >> 8) & 0xff);
    dw8250_write32(addr, UART_LCR, lcr_val);
}

static rt_err_t dw8250_uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    rt_base_t base;
    struct hw_uart_device *uart;
    int clock_divisor;
    int last_ier_state;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct hw_uart_device *)serial->parent.user_data;
    base = uart->hw_base;

    while (!(dw8250_read32(base, UART_LSR) & UART_LSR_TEMT));

    last_ier_state = dw8250_read32(base, UART_IER);
    dw8250_write32(base, UART_IER, 0);
    dw8250_write32(base, UART_MCR, UART_MCRVAL);
    dw8250_write32(base, UART_FCR, UART_FCR_DEFVAL);

    /* initialize serial config to 8N1 before writing baudrate */
    dw8250_write32(base, UART_LCR, UART_LCR_8N1);

    clock_divisor = DIV_ROUND_CLOSEST(UART_INPUT_CLK, 16 * serial->config.baud_rate);
    dw8250_uart_setbrg(base, clock_divisor);

    dw8250_write32(base, UART_IER, last_ier_state);

    return RT_EOK;
}

static rt_err_t dw8250_uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct hw_uart_device *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct hw_uart_device *)serial->parent.user_data;

    switch (cmd)
    {
        case RT_DEVICE_CTRL_CLR_INT:
            /* Disable rx irq */
            dw8250_write32(uart->hw_base, UART_IER, !UART_IER_RDI);
            rt_hw_interrupt_mask(uart->irqno);
            break;

        case RT_DEVICE_CTRL_SET_INT:
            /* Enable rx irq */
            dw8250_write32(uart->hw_base, UART_IER, UART_IER_RDI);
            rt_hw_interrupt_umask(uart->irqno);
            break;
    }

    return RT_EOK;
}

static int dw8250_uart_putc(struct rt_serial_device *serial, char c)
{
    rt_base_t base;
    struct hw_uart_device *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct hw_uart_device *)serial->parent.user_data;
    base = uart->hw_base;

    while ((dw8250_read32(base, UART_LSR) & BOTH_EMPTY) != BOTH_EMPTY);

    dw8250_write32(base, UART_TX, c);

    return 1;
}

static int dw8250_uart_getc(struct rt_serial_device *serial)
{
    int ch = -1;
    rt_base_t base;
    struct hw_uart_device *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct hw_uart_device *)serial->parent.user_data;
    base = uart->hw_base;

    if (dw8250_read32(base, UART_LSR) & UART_LSR_DR)
    {
        ch = dw8250_read32(base, UART_RX) & 0xff;
    }

    return ch;
}

static const struct rt_uart_ops _uart_ops =
{
    dw8250_uart_configure,
    dw8250_uart_control,
    dw8250_uart_putc,
    dw8250_uart_getc,
};

static void rt_hw_uart_isr(int irqno, void *param)
{
    unsigned int iir, status;
    struct rt_serial_device *serial = (struct rt_serial_device *)param;
    struct hw_uart_device *uart = (struct hw_uart_device *)serial->parent.user_data;

    iir = dw8250_read32(uart->hw_base, UART_IIR);

    /* If don't do this in non-DMA mode then the "RX TIMEOUT" interrupt will fire forever. */
    if ((iir & 0x3f) == UART_IIR_RX_TIMEOUT)
    {
        status = dw8250_read32(uart->hw_base, UART_LSR);

        if (!(status & (UART_LSR_DR | UART_LSR_BI)))
        {
            dw8250_read32(uart->hw_base, UART_RX);
        }
    }

    if (!(iir & UART_IIR_NO_INT))
    {
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
    }

    if ((iir & UART_IIR_BUSY) == UART_IIR_BUSY)
    {
        /* Clear the USR */
        dw8250_read32(uart->hw_base, UART_USR);

        return;
    }
}

static void rt_hw_uart_pinmux_config(void)
{
// uart0
#ifdef BSP_USING_UART0

#ifdef UART0_RX_USING_UART0_RX
    PINMUX_CONFIG(UART0_RX, UART0_RX);
#else
    LOG_E("UART0_RX pinmux config failed");
#endif

#ifdef UART0_TX_USING_UART0_TX
    PINMUX_CONFIG(UART0_TX, UART0_TX);
#else
    LOG_E("UART0_TX pinmux config failed");
#endif

#endif  /* BSP_USING_UART0 */

// uart1 pinmux config
#ifdef BSP_USING_UART1

#ifdef UART1_RX_USING_SD0_D2
    PINMUX_CONFIG(SD0_D2, UART1_RX);
#elif defined(UART1_RX_USING_UART0_RX)
    PINMUX_CONFIG(UART0_RX, UART1_RX);
#elif defined(UART1_RX_USING_IIC0_SDA)
    PINMUX_CONFIG(IIC0_SDA, UART1_RX);
#elif defined(UART1_RX_USING_PWR_BUTTON1)
    PINMUX_CONFIG(PWR_BUTTON1, UART1_RX);
#elif defined(UART1_RX_USING_JTAG_CPU_TCK)
    PINMUX_CONFIG(JTAG_CPU_TCK, UART1_RX);
#elif defined(UART1_RX_USING_PWR_ON)
    PINMUX_CONFIG(PWR_ON, UART1_RX);
#else
    LOG_E("UART1_RX pinmux config failed");
#endif

#ifdef UART1_TX_USING_SD0_D1
    PINMUX_CONFIG(SD0_D1, UART1_TX);
#elif defined(UART1_TX_USING_UART0_TX)
    PINMUX_CONFIG(UART0_TX, UART1_TX);
#elif defined(UART1_TX_USING_IIC0_SCL)
    PINMUX_CONFIG(IIC0_SCL, UART1_TX);
#elif defined(UART1_TX_USING_PWR_WAKEUP0)
    PINMUX_CONFIG(PWR_WAKEUP0, UART1_TX);
#elif defined(UART1_TX_USING_JTAG_CPU_TMS)
    PINMUX_CONFIG(JTAG_CPU_TMS, UART1_TX);
#elif defined(UART1_TX_USING_PWR_WAKEUP1)
    PINMUX_CONFIG(PWR_WAKEUP1, UART1_TX);
#else
    LOG_E("UART1_TX pinmux config failed");
#endif

#endif  /* BSP_USING_UART1 */

// uart2
#ifdef BSP_USING_UART2

#ifdef UART2_RX_USING_IIC0_SDA
    PINMUX_CONFIG(IIC0_SDA, UART2_RX);
#elif defined(UART2_RX_USING_SD1_D1)
    PINMUX_CONFIG(SD1_D1, UART2_RX);
#elif defined(UART2_RX_USING_PWR_GPIO1)
    PINMUX_CONFIG(PWR_GPIO1, UART2_RX);
#elif defined(UART2_RX_USING_UART2_RX)
    PINMUX_CONFIG(UART2_RX, UART2_RX);
#elif defined(UART2_RX_USING_IIC2_SCL)
    PINMUX_CONFIG(IIC2_SCL, UART2_RX);
#elif defined(UART2_RX_USING_VIVO_D5)
    PINMUX_CONFIG(VIVO_D5, UART2_RX);
#elif defined(UART2_RX_USING_VIVO_D9)
    PINMUX_CONFIG(VIVO_D9, UART2_RX);
#elif defined(UART2_RX_USING_VIVO_CLK)
    PINMUX_CONFIG(VIVO_CLK, UART2_RX);
#else
    LOG_E("UART2_RX pinmux config failed");
#endif

#ifdef UART2_TX_USING_IIC0_SCL
    PINMUX_CONFIG(IIC0_SCL, UART2_TX);
#elif defined(UART2_TX_USING_SD1_D2)
    PINMUX_CONFIG(SD1_D2, UART2_TX);
#elif defined(UART2_TX_USING_PWR_GPIO0)
    PINMUX_CONFIG(PWR_GPIO0, UART2_TX);
#elif defined(UART2_TX_USING_UART2_TX)
    PINMUX_CONFIG(UART2_TX, UART2_TX);
#elif defined(UART2_TX_USING_IIC2_SDA)
    PINMUX_CONFIG(IIC2_SDA, UART2_TX);
#elif defined(UART2_TX_USING_VIVO_D6)
    PINMUX_CONFIG(VIVO_D6, UART2_TX);
#elif defined(UART2_TX_USING_VIVO_D10)
    PINMUX_CONFIG(VIVO_D10, UART2_TX);
#elif defined(UART2_TX_USING_VIVO_D2)
    PINMUX_CONFIG(VIVO_D2, UART2_TX);
#else
    LOG_E("UART2_TX pinmux config failed");
#endif

#endif  /* BSP_USING_UART2 */

// uart3
#ifdef BSP_USING_UART3

#ifdef UART3_RX_USING_MUX_SPI1_MOSI
    PINMUX_CONFIG(SPI1_MOSI, UART3_RX);
#elif defined(UART3_RX_USING_PAD_ETH_TXP___EPHY_RXN)
    PINMUX_CONFIG(PAD_ETH_TXP___EPHY_RXN, UART3_RX);
#elif defined(UART3_RX_USING_SD0_D3)
    PINMUX_CONFIG(SD0_D3, UART3_RX);
#elif defined(UART3_RX_USING_SD1_D1)
    PINMUX_CONFIG(SD1_D1, UART3_RX);
#elif defined(UART3_RX_USING_ADC2)
    PINMUX_CONFIG(ADC2, UART3_RX);
#else
    LOG_E("UART3_RX pinmux config failed");
#endif

#ifdef UART3_TX_USING_MUX_SPI1_SCK
    PINMUX_CONFIG(SPI1_SCK, UART3_TX);
#elif defined(UART3_TX_USING_PAD_ETH_RXP___EPHY_TXN)
    PINMUX_CONFIG(PAD_ETH_RXP___EPHY_TXN, UART3_TX);
#elif defined(UART3_TX_USING_SD0_D0)
    PINMUX_CONFIG(SD0_D0, UART3_TX);
#elif defined(UART3_TX_USING_SD1_D2)
    PINMUX_CONFIG(SD1_D2, UART3_TX);
#elif defined(UART3_TX_USING_ADC3)
    PINMUX_CONFIG(ADC3, UART3_TX);
#else
    LOG_E("UART3_TX pinmux config failed");
#endif

#endif  /* BSP_USING_UART3 */

// uart4
#ifdef BSP_USING_UART4

#ifdef UART4_RX_USING_SD1_GPIO0
    PINMUX_CONFIG(SD1_GPIO0, UART4_RX);
#elif defined(UART4_RX_USING_UART2_RX)
    PINMUX_CONFIG(UART2_RX, UART4_RX);
#else
    LOG_E("UART4_RX pinmux config failed");
#endif

#ifdef UART4_TX_USING_SD1_GPIO1
    PINMUX_CONFIG(SD1_GPIO1, UART4_TX);
#elif defined(UART4_TX_USING_UART2_TX)
    PINMUX_CONFIG(UART2_TX, UART4_TX);
#else
    LOG_E("UART4_TX pinmux config failed");
#endif

#endif  /* BSP_USING_UART4 */
}

int rt_hw_uart_init(void)
{
    struct hw_uart_device* uart;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;

    config.baud_rate = 115200;

#define BSP_INSTALL_UART_DEVICE(no)     \
    uart = &_uart##no##_device;         \
    _serial##no.ops    = &_uart_ops;    \
    _serial##no.config = config;        \
    rt_hw_serial_register(&_serial##no, "uart" #no, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, uart); \
    rt_hw_interrupt_install(uart->irqno, rt_hw_uart_isr, &_serial##no, "uart" #no);

    rt_hw_uart_pinmux_config();

#ifdef BSP_USING_UART0

    BSP_INSTALL_UART_DEVICE(0);
#if defined(ARCH_ARM)
    uart->hw_base = (rt_size_t)rt_ioremap((void*)uart->hw_base, 0x10000);
#endif /* defined(ARCH_ARM) */
#endif

#ifdef BSP_USING_UART1
    BSP_INSTALL_UART_DEVICE(1);
#if defined(ARCH_ARM)
    uart->hw_base = (rt_size_t)rt_ioremap((void*)uart->hw_base, 0x10000);
#endif /* defined(ARCH_ARM) */
#endif

#ifdef BSP_USING_UART2
    BSP_INSTALL_UART_DEVICE(2);
#if defined(ARCH_ARM)
    uart->hw_base = (rt_size_t)rt_ioremap((void*)uart->hw_base, 0x10000);
#endif /* defined(ARCH_ARM) */
#endif

#ifdef BSP_USING_UART3
    BSP_INSTALL_UART_DEVICE(3);
#if defined(ARCH_ARM)
    uart->hw_base = (rt_size_t)rt_ioremap((void*)uart->hw_base, 0x10000);
#endif /* defined(ARCH_ARM) */
#endif

#ifdef BSP_USING_UART4
    BSP_INSTALL_UART_DEVICE(4);
#if defined(ARCH_ARM)
    uart->hw_base = (rt_size_t)rt_ioremap((void*)uart->hw_base, 0x10000);
#endif /* defined(ARCH_ARM) */
#endif

    return 0;
}
