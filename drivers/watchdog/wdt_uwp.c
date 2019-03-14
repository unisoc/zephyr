/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <watchdog.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>

#include "uwp_hal.h"

#define DEV_CFG(dev) \
	((const struct wdg_uwp_config * const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct wdg_uwp_data *)(dev)->driver_data)

static void uwp_config_irq(struct device *dev);
struct wdg_uwp_data {
	void (*cb)(struct device *dev);
	u32_t timeout;
	u32_t reload;
	u32_t mode;
};

struct wdg_uwp_config {
	void (*irq_config_func)(struct device *dev);
};

static struct device DEVICE_NAME_GET(wdg_uwp);
static struct wdg_uwp_data uwp_dev_data = {
	.cb = NULL,
};

static struct wdg_uwp_config uwp_dev_cfg = {
	.irq_config_func = uwp_config_irq,
};

static void wdg_uwp_enable(struct device *dev)
{
	uwp_wdg_enable();
}

static int wdg_uwp_disable(struct device *dev)
{
	uwp_wdg_disable();

	return 0;
}

static void wdg_uwp_reload(struct device *dev)
{
	struct wdg_uwp_data * const dev_data = DEV_DATA(dev);

	uwp_wdg_load(dev_data->timeout);
	uwp_wdg_load_irq(dev_data->timeout >> 1);
}

static int wdg_uwp_set_config(struct device *dev,
				 struct wdt_config *config)
{
	struct wdg_uwp_data * const dev_data = DEV_DATA(dev);
	u32_t mode;

	if (config->mode == WDT_MODE_RESET)
		mode = WDG_MODE_RESET;
	else
		mode = WDG_MODE_MIX;

	dev_data->mode = config->mode;
	dev_data->timeout = config->timeout;
	dev_data->cb = config->interrupt_fn;

	uwp_wdg_set_mode(mode);
	wdg_uwp_reload(dev);

	return 0;
}

static void wdg_uwp_get_config(struct device *dev,
				  struct wdt_config *config)
{
	struct wdg_uwp_data * const dev_data = DEV_DATA(dev);

	config->timeout = dev_data->timeout;
	config->mode = dev_data->mode;
	config->interrupt_fn = dev_data->cb;
}

static void wdg_uwp_isr(void *arg)
{
	struct device *dev = arg;
	struct wdg_uwp_data * const dev_data = DEV_DATA(dev);

	if (dev_data->cb) {
		dev_data->cb(dev);
	}

	/* Clear interrupts here */
	uwp_wdg_int_clear();
}

static const struct wdt_driver_api wdg_uwp_api = {
	.enable = wdg_uwp_enable,
	.disable = wdg_uwp_disable,
	.get_config = wdg_uwp_get_config,
	.set_config = wdg_uwp_set_config,
	.reload = wdg_uwp_reload,
};

static void uwp_config_irq(struct device *dev)
{
	IRQ_CONNECT(DT_WDT_UWP_IRQ, DT_WDT_UWP_IRQ_PRIO,
			wdg_uwp_isr,
			DEVICE_GET(wdg_uwp), 0);
	irq_enable(DT_WDT_UWP_IRQ);
}

static int wdg_uwp_init(struct device *dev)
{
	const struct wdg_uwp_config * const config = DEV_CFG(dev);

	uwp_sys_enable(BIT(APB_EB_WDG)
			| BIT(APB_EB_WDG_RTC)
			| BIT(APB_EB_SYST_RTC));

	uwp_sys_reset(BIT(APB_EB_WDG));

	config->irq_config_func(dev);

	return 0;
}

DEVICE_AND_API_INIT(wdg_uwp, DT_WDT_UWP_DEVICE_NAME,
		    wdg_uwp_init, &uwp_dev_data, &uwp_dev_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdg_uwp_api);

