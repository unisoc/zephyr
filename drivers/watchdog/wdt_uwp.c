/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <watchdog.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>
#include <kernel.h>

#include "uwp_hal.h"

#define DEV_CFG(dev) \
	((const struct wdg_uwp_config *)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct wdg_uwp_data *)(dev)->driver_data)

struct wdg_uwp_data {
	wdt_callback_t cb;
	u32_t timeout;
	u32_t reload;
	u32_t mode;
};

struct wdg_uwp_config {
	void (*irq_config_func)(struct device *dev);
};

static struct device DEVICE_NAME_GET(wdg_uwp);

static int wdg_uwp_disable(struct device *dev)
{
	uwp_wdg_disable();

	return 0;
}

static int wdg_uwp_install_timeout(struct device *dev,
				      const struct wdt_timeout_cfg *cfg)
{
	struct wdg_uwp_data * const dev_data = DEV_DATA(dev);
	ARG_UNUSED(dev);
	if (cfg->flags != WDT_FLAG_RESET_SOC) {
		return -ENOTSUP;
	}

	if (cfg->window.min != 0 || cfg->window.max == 0) {
		return -EINVAL;
	}

	dev_data->timeout = cfg->window.max;

	dev_data->mode = (cfg->callback == NULL) ?
			WDT_MODE_RESET : WDT_MODE_INTERRUPT_RESET;
	dev_data->cb = cfg->callback;
	return 0;
}

/*  channel_id: irq timeout  */
static int wdg_uwp_feed(struct device *dev, int channel_id)
{
	struct wdg_uwp_data * const dev_data = DEV_DATA(dev);
	uwp_wdg_load(dev_data->timeout);
	uwp_wdg_load_irq(channel_id);


	return 0;
}

/*  options: irq timeout  */
static int wdg_uwp_setup(struct device *dev, u8_t options)
{
	struct wdg_uwp_data * const dev_data = DEV_DATA(dev);
	u32_t mode;

	if (dev_data->mode == WDT_MODE_RESET)
		mode = WDG_MODE_RESET;
	else
		mode = WDG_MODE_MIX;
	uwp_wdg_set_mode(mode);
	wdg_uwp_feed(dev, options);

	uwp_wdg_enable();

	return 0;
}

static void wdg_uwp_isr(void *arg)
{
	struct device *dev = arg;
	struct wdg_uwp_data * const dev_data = DEV_DATA(dev);
	if (dev_data->cb) {
		dev_data->cb(dev, 0);
	}

	/* Clear interrupts here */
	uwp_wdg_int_clear();
}

static const struct wdt_driver_api wdg_uwp_api = {
	.setup = wdg_uwp_setup,
	.disable = wdg_uwp_disable,
	.install_timeout = wdg_uwp_install_timeout,
	.feed = wdg_uwp_feed,
};

#ifdef CONFIG_WDT_UWP

static struct wdg_uwp_data uwp_dev_data = {
	.cb = NULL,
	.timeout = 3000,
};

static void uwp_config_irq(struct device *dev)
{
	IRQ_CONNECT(DT_WDT_UWP_IRQ, DT_WDT_UWP_IRQ_PRIO,
			wdg_uwp_isr,
			DEVICE_GET(wdg_uwp), 0);
	irq_enable(DT_WDT_UWP_IRQ);
}

static struct wdg_uwp_config uwp_dev_cfg = {
	.irq_config_func = uwp_config_irq,
};

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
		    &wdg_uwp_init, &uwp_dev_data, &uwp_dev_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &wdg_uwp_api);
#endif

