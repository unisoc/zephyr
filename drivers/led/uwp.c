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
	    ((struct uwp_led_priv *)(dev)->driver_data)

#define LED_ON_VALUE (0)
#define LED_OFF_VALUE (1)

struct uwp_led_priv {
	struct device *gpio;
};

static struct uwp_led_priv uwp_led_data;

static int uwp_led_off(struct device *dev, u32_t led)
{
	struct uwp_led_priv *priv = DEV_DATA(dev);

	if (!priv) {
		LOG_ERR("Unable to find priv data");
		return -ENODATA;
	}

	if (!priv->gpio) {
		LOG_ERR("No gpio device");
		return -ENXIO;
	}

	return gpio_pin_write(priv->gpio, led, LED_OFF_VALUE);
}

static int uwp_led_on(struct device *dev, u32_t led)
{
	struct uwp_led_priv *priv = DEV_DATA(dev);

	if (!priv) {
		LOG_ERR("Unable to find priv data");
		return -ENODATA;
	}

	if (!priv->gpio) {
		LOG_ERR("No gpio device");
		return -ENXIO;
	}

	return gpio_pin_write(priv->gpio, led, LED_ON_VALUE);
}

static int uwp_led_init(struct device *dev)
{
	struct uwp_led_priv *priv = DEV_DATA(dev);

	if (!priv) {
		LOG_ERR("Unable to find priv data");
		return -ENODATA;
	}

	priv->gpio = device_get_binding(CONFIG_GPIO_UWP_P0_NAME);
	if (!priv->gpio) {
		LOG_ERR("Unable to find %s", CONFIG_GPIO_UWP_P0_NAME);
		return -ENODEV;
	}

	gpio_pin_configure(priv->gpio, CONFIG_LED_PIN1,
			GPIO_DIR_OUT | GPIO_PUD_PULL_DOWN);

	gpio_pin_configure(priv->gpio, CONFIG_LED_PIN2,
			GPIO_DIR_OUT | GPIO_PUD_PULL_DOWN);

	gpio_pin_configure(priv->gpio, CONFIG_LED_PIN3,
			GPIO_DIR_OUT | GPIO_PUD_PULL_DOWN);

	return 0;
}

static const struct led_driver_api uwp_led_api = {
	.on = uwp_led_on,
	.off = uwp_led_off,
};

DEVICE_AND_API_INIT(led_uwp, CONFIG_LED_DRV_NAME, uwp_led_init,
			&uwp_led_data, NULL, POST_KERNEL,
			CONFIG_LED_INIT_PRIORITY, &uwp_led_api);
