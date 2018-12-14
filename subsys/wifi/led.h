/*
 * @file
 * @brief LED lighting header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_LED_H_
#define _WIFIMGR_LED_H_

#include <led.h>

#ifdef CONFIG_WIFIMGR_LED
int wifimgr_sta_led_on(void);
int wifimgr_sta_led_off(void);
int wifimgr_ap_led_on(void);
int wifimgr_ap_led_off(void);
#else
#define wifimgr_sta_led_on(...)
#define wifimgr_sta_led_off(...)
#define wifimgr_ap_led_on(...)
#define wifimgr_ap_led_off(...)
#endif

#endif
