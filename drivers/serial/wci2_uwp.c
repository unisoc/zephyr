/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <uart.h>
#include <irq_nextlevel.h>

#include "uwp_hal.h"

#define DIV_ROUND(n, d) (((n) + ((d)/2)) / (d))

/* convenience defines */
#define DEV_CFG(dev) \
	((const struct uart_device_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct wci2_uwp_dev_data_t * const)(dev)->driver_data)
#define WCI2_STRUCT(dev) \
	((volatile struct uwp_wci2 *)(DEV_CFG(dev))->base)

#ifdef CONFIG_WCI2_UWP
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void wci2_uwp_isr(void *arg);
static struct device DEVICE_NAME_GET(wci2_uwp);
static void uwp_config_wci2_irq(struct device *dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

/*private data structure*/
struct wci2_uwp_dev_data_t {
	u32_t baud_rate;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t cb; /**< Callback function pointer */
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static struct wci2_uwp_dev_data_t wci2_uwp_dev_data = {
	.baud_rate = DT_WCI2_UWP_SPEED,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.cb = NULL,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static const struct uart_device_config wci2_uwp_dev_cfg = {
	.base = (void *)DT_WCI2_UWP_BASE,
	.sys_clk_freq = DT_WCI2_UWP_CLOCK,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = uwp_config_wci2_irq,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};
#endif /* CONFIG_WCI2_UWP */

static int wci2_uwp_poll_in(struct device *dev, unsigned char *c)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);

	if (uwp_wci2_rx_ready(uart)) {
		*c = uwp_wci2_read(uart);
		return 0;
	}

	return -1;
}

static void wci2_uwp_poll_out(struct device *dev, unsigned char c)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);

	if (uwp_wci2_tx_ready(uart)) {
		uwp_wci2_write(uart, c);
		while (!uwp_wci2_trans_over(uart))
			;
	};
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int wci2_uwp_fifo_fill(struct device *dev, const u8_t *tx_data,
				 int size)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);
	unsigned int num_tx = 0;

	while ((size - num_tx) > 0) {
		/* Send a character */
		if (uwp_wci2_tx_ready(uart)) {
			uwp_wci2_write(uart, tx_data[num_tx]);
			num_tx++;
		} else {
			break;
		}
	}

	return (int)num_tx;
}

static int wci2_uwp_fifo_read(struct device *dev, u8_t *rx_data,
				 const int size)
{
	unsigned int num_rx = 0;
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);

	while (((size - num_rx) > 0) &&
			uwp_wci2_rx_ready(uart)) {

		/* Receive a character */
		rx_data[num_rx++] = uwp_wci2_read(uart);
	}

	return num_rx;
}

static void wci2_uwp_irq_tx_enable(struct device *dev)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);

	uwp_wci2_int_enable(uart, BIT(UART_TXF_EMPTY));
}

static void wci2_uwp_irq_tx_disable(struct device *dev)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);

	uwp_wci2_int_disable(uart, BIT(UART_TXF_EMPTY));
}

static int wci2_uwp_irq_tx_ready(struct device *dev)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);
	u32_t status;

	status = uwp_wci2_status(uart);

	return (status & BIT(WCI2_TXF_EMPTY));
}

static void wci2_uwp_irq_rx_enable(struct device *dev)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);

	uwp_wci2_int_enable(uart, BIT(UART_RXF_FULL));
}

static void wci2_uwp_irq_rx_disable(struct device *dev)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);

	uwp_wci2_int_disable(uart, BIT(UART_RXD));
}

static int wci2_uwp_irq_tx_complete(struct device *dev)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);
	u32_t status;

	status = uwp_wci2_status(uart);

	return (status & BIT(WCI2_TXF_EMPTY));
}

static int wci2_uwp_irq_rx_ready(struct device *dev)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);

	return uwp_wci2_rx_ready(uart);
}

static void wci2_uwp_irq_err_enable(struct device *dev)
{
	/* Not yet used in zephyr */
}

static void wci2_uwp_irq_err_disable(struct device *dev)
{
	/* Not yet used in zephyr */
}

static int wci2_uwp_irq_is_pending(struct device *dev)
{
	volatile struct uwp_wci2 *uart = WCI2_STRUCT(dev);
	u32_t status;

	status = uwp_wci2_status(uart);

	return (status & (UART_TXF_EMPTY | UART_RXF_FULL));
}

static int wci2_uwp_irq_update(struct device *dev)
{
	return 1;
}

static void wci2_uwp_irq_callback_set(struct device *dev,
					 uart_irq_callback_user_data_t cb,
					 void *user_data)
{
	struct wci2_uwp_dev_data_t * const dev_data = DEV_DATA(dev);

	dev_data->cb = cb;
	dev_data->user_data = user_data;
}

static void wci2_uwp_isr(void *arg)
{
	struct device *dev = arg;
	struct wci2_uwp_dev_data_t * const dev_data = DEV_DATA(dev);

	if (dev_data->cb) {
		dev_data->cb(dev_data->user_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api wci2_uwp_driver_api = {
	.poll_in = wci2_uwp_poll_in,
	.poll_out = wci2_uwp_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill	  = wci2_uwp_fifo_fill,
	.fifo_read	  = wci2_uwp_fifo_read,
	.irq_tx_enable	  = wci2_uwp_irq_tx_enable,
	.irq_tx_disable	  = wci2_uwp_irq_tx_disable,
	.irq_tx_ready	  = wci2_uwp_irq_tx_ready,
	.irq_rx_enable	  = wci2_uwp_irq_rx_enable,
	.irq_rx_disable	  = wci2_uwp_irq_rx_disable,
	.irq_tx_complete  = wci2_uwp_irq_tx_complete,
	.irq_rx_ready	  = wci2_uwp_irq_rx_ready,
	.irq_err_enable	  = wci2_uwp_irq_err_enable,
	.irq_err_disable  = wci2_uwp_irq_err_disable,
	.irq_is_pending	  = wci2_uwp_irq_is_pending,
	.irq_update	  = wci2_uwp_irq_update,
	.irq_callback_set = wci2_uwp_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};


#ifdef CONFIG_WCI2_UWP
static void uwp_config_wci2_irq(struct device *dev)
{
	IRQ_CONNECT(DT_WCI2_UWP_IRQ,
			DT_WCI2_UWP_IRQ_PRIO,
			wci2_uwp_isr,
			DEVICE_GET(wci2_uwp), 0);
	irq_enable(DT_WCI2_UWP_IRQ);
}

static int wci2_uwp_init(struct device *dev)
{
	volatile struct uwp_wci2 *wci2 = WCI2_STRUCT(dev);
	const struct uart_device_config * const dev_cfg = DEV_CFG(dev);
	struct wci2_uwp_dev_data_t * const dev_data = DEV_DATA(dev);

	uwp_sys_reset(BIT(APB_RST_WCI2));
	uwp_sys_enable(BIT(APB_EB_WCI2));

	uwp_wci2_set_cdk(wci2, DIV_ROUND(dev_cfg->sys_clk_freq,
			dev_data->baud_rate));
	uwp_wci2_set_stop_bit_num(wci2, 1);
	uwp_wci2_set_byte_len(wci2, 3);

	uwp_wci2_init(wci2);

	/* wci2 work as uart in non-real time mode*/
	uwp_wci2_set_mode((u32_t)dev_cfg->base);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	dev_cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

DEVICE_AND_API_INIT(wci2_uwp, DT_WCI2_UWP_NAME,
		    wci2_uwp_init, &wci2_uwp_dev_data,
		    &wci2_uwp_dev_cfg,
		    PRE_KERNEL_1, 10,
		    (void *)&wci2_uwp_driver_api);
#endif /* CONFIG_WCI2_UWP */
