/*
 * @file
 * @brief Configuration header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFI_CONFIG_H_
#define _WIFI_CONFIG_H_

#include <settings/settings.h>

#define WIFIMGR_SETTING_NAME_LEN	63
#define WIFIMGR_SETTING_VAL_LEN		63

#define WIFIMGR_SETTING_NAME_SSID	"ssid"
#define WIFIMGR_SETTING_NAME_BSSID	"bssid"
#define WIFIMGR_SETTING_NAME_PSK	"psk"
#define WIFIMGR_SETTING_NAME_BAND	"band"
#define WIFIMGR_SETTING_NAME_CHANNEL	"channel"

#define WIFIMGR_SETTING_PATH		"wifimgr"
#define WIFIMGR_SETTING_STA_PATH	"wifimgr/sta"
#define WIFIMGR_SETTING_AP_PATH		"wifimgr/ap"

struct wifimgr_settings_map {
	char name[WIFIMGR_SETTING_NAME_LEN + 1];
	char *valptr;
	enum settings_type type;
};

#ifdef CONFIG_WIFIMGR_CONFIG
int wifimgr_config_init(void *handle);
int wifimgr_load_config(void *handle, char *path);
int wifimgr_save_config(void *handle, char *path);
#else
#define wifimgr_config_init(...)
#define wifimgr_load_config(...)
#define wifimgr_save_config(...)
#endif

#endif
