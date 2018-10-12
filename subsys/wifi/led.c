/*
   *@file
 * @brief LED lighting
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_GPIO_UWP)
#include "led.h"
#include <gpio.h>

static struct device *gpio;

void led_init(void)
{
	gpio = device_get_binding(GPIO_PORT0);
	gpio_pin_configure(gpio, LED1_GPIO_PIN, 1);
	gpio_pin_configure(gpio, LED3_GPIO_PIN, 1);
}

void led_uninit(void)
{
	gpio = NULL;
}

int light_turn_on(u32_t pin)
{
	int ret = -1;

	if (gpio) {
		ret = gpio_pin_write(gpio,
				     pin,
				     LED_ON_VALUE);
	}

	return ret;
}

int light_turn_off(u32_t pin)
{
	int ret = -1;

	if (gpio) {
		ret = gpio_pin_write(gpio,
				     pin,
				     LED_OFF_VALUE);
	}

	return ret;
}
#endif


