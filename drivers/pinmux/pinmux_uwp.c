/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief
 *
 * A common driver for uwp pinmux. Each SoC must implement a SoC
 * specific part of the driver.
 */

#include <errno.h>
#include <init.h>
#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <pinmux.h>
#include <uwp_hal.h>

struct uwp_gpio_config {
	/* base address of GPIO port */
	u32_t port_base;
	/* GPIO IRQ number */
	u32_t irq_num;
};

struct uwp_gpio_data {
	/* list of registered callbacks */
	sys_slist_t callbacks;
	/* callback enable pin bitmask */
	u32_t pin_callback_enables;
};

static int uwp_pinmux_set(struct device *dev, u32_t pin_reg,
					u32_t func)
{
	uwp_pmux_func_clear(pin_reg);
	uwp_pmux_func_set(pin_reg, func);
	return 0;
}

static int uwp_pinmux_get(struct device *dev, u32_t pin_reg,
					u32_t *func)
{
	uwp_pmux_get(pin_reg, func);
	return 0;
}

static int uwp_pinmux_input(struct device *dev, u32_t pin_reg,
					u8_t func)
{
	return -ENOTSUP;
}

static int uwp_pinmux_pullup(struct device *dev, u32_t pin_reg,
					u8_t func)
{
	if (func == PMUX_FPU_EN) {
		uwp_pmux_pin_pullup(pin_reg);
	} else if (func == PMUX_FPD_EN) {
		uwp_pmux_pin_pulldown(pin_reg);
	}
	return 0;
}

/* when use functions in pinmux.h, 2nd parameter is pin_reg*/
const struct pinmux_driver_api uwp_pinmux_api = {
	.set = uwp_pinmux_set,
	.get = uwp_pinmux_get,
	.pullup = uwp_pinmux_pullup,
	.input = uwp_pinmux_input,
};

DEVICE_AND_API_INIT(uwp_pmux_dev, CONFIG_PINMUX_UWP_DEV_NAME, uwp_pinmux_init,
			NULL, NULL, PRE_KERNEL_1,
			CONFIG_PINMUX_INIT_PRIORITY,
			&uwp_pinmux_api);
