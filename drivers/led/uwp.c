/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(led_uwp);

#include <zephyr.h>
#include <gpio.h>
#include <led.h>
#include <device.h>


#define DEV_DATA(dev) \
	    ((struct device *)(dev)->driver_data)

#define LED_ON_VALUE (0)
#define LED_OFF_VALUE (1)

static struct device gpio;

static int led_uwp_off(struct device *dev, u32_t led)
{
	struct device *gpio = DEV_DATA(dev);

	if (!gpio) {
		LOG_ERR("No gpio device");
		return -ENXIO;
	}

	return gpio_pin_write(gpio, led, LED_OFF_VALUE);
}

static int led_uwp_on(struct device *dev, u32_t led)
{
	struct device *gpio = DEV_DATA(dev);

	if (!gpio) {
		LOG_ERR("No gpio device");
		return -ENXIO;
	}

	return gpio_pin_write(gpio, led, LED_ON_VALUE);
}

static int led_uwp_init(struct device *dev)
{
	struct device *gpio = DEV_DATA(dev);

	gpio = device_get_binding(CONFIG_GPIO_UWP_P0_NAME);
	if (!gpio) {
		LOG_ERR("Unable to find %s", CONFIG_GPIO_UWP_P0_NAME);
		return -ENODEV;
	}

	gpio_pin_configure(gpio, CONFIG_LED_PIN1,
			GPIO_DIR_OUT | GPIO_PUD_PULL_DOWN);

	gpio_pin_configure(gpio, CONFIG_LED_PIN3,
			GPIO_DIR_OUT | GPIO_PUD_PULL_DOWN);

	return 0;
}

static const struct led_driver_api led_uwp_api = {
	.on = led_uwp_on,
	.off = led_uwp_off,
};

DEVICE_AND_API_INIT(led_uwp, CONFIG_LED_DRV_NAME, led_uwp_init,
			&gpio, NULL, POST_KERNEL,
			CONFIG_LED_INIT_PRIORITY, &led_uwp_api);
