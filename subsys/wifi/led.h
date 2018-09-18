/*
 *@file
 * @brief LED lighting header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _WIFI_LED_H_
#define _WIFI_LED_H_





#if defined(CONFIG_GPIO_UWP)
#include <zephyr/types.h>
#define GPIO_PORT0	"UWP_GPIO_P0"
#define LED1_GPIO_PIN 1
#define LED3_GPIO_PIN 3
#define LED_OFF_VALUE 1
#define LED_ON_VALUE 0
void led_init(void);
void led_uninit(void);
int light_turn_on(u32_t pin);
int light_turn_off(u32_t pin);
#else
#define led_init(...)
#define led_uninit(...)
#define light_turn_on(...)
#define light_turn_off(...)
#endif


#endif /* _WIFI_LED_H_ */
