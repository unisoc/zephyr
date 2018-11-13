/*
 *@file
 * @brief LED indication
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "led.h"

static int wifimgr_led_on(const char *name, int pin)
{
	struct device *led;
	int ret = -1;

	led = device_get_binding(name);

	if (led) {
		ret = led_on(led, pin);
	}

	return ret;
}

static int wifimgr_led_off(const char *name, int pin)
{
	struct device *led;
	int ret = -1;

	led = device_get_binding(name);

	if (led) {
		led_off(led, pin);
	}

	return ret;
}

int wifimgr_sta_led_on(void)
{
	return wifimgr_led_on(CONFIG_WIFIMGR_LED_NAME, CONFIG_WIFIMGR_LED_STA);
}

int wifimgr_ap_led_on(void)
{
	return wifimgr_led_on(CONFIG_WIFIMGR_LED_NAME, CONFIG_WIFIMGR_LED_AP);
}

int wifimgr_sta_led_off(void)
{
	return wifimgr_led_off(CONFIG_WIFIMGR_LED_NAME, CONFIG_WIFIMGR_LED_STA);
}

int wifimgr_ap_led_off(void)
{
	return wifimgr_led_off(CONFIG_WIFIMGR_LED_NAME, CONFIG_WIFIMGR_LED_AP);
}
