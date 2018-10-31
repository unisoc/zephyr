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

#ifdef CONFIG_LED
#define WIFIMGR_LED_NAME CONFIG_LED_DRV_NAME
#define WIFIMGR_LED_PIN1 CONFIG_LED_PIN1
#define WIFIMGR_LED_PIN3 CONFIG_LED_PIN3
#else
#define WIFIMGR_LED_NAME ""
#define WIFIMGR_LED_PIN1 (0)
#define WIFIMGR_LED_PIN3 (0)
#endif

#define WIFIMGR_LED_OFF_VALUE (1)
#define WIFIMGR_LED_ON_VALUE (0)



#endif /* _WIFI_LED_H_ */
