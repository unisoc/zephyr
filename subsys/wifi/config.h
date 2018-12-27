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
#define WIFIMGR_SETTING_VAL_LEN		(((((WIFIMGR_SETTING_NAME_LEN) / 3) * 4) + 4) + 1)	/*Due to base64 encoding*/

#define WIFIMGR_SETTING_NAME_AUTORUN		"autorun"
#define WIFIMGR_SETTING_NAME_SSID		"ssid"
#define WIFIMGR_SETTING_NAME_BSSID		"bssid"
#define WIFIMGR_SETTING_NAME_PSK		"psk"
#define WIFIMGR_SETTING_NAME_BAND		"band"
#define WIFIMGR_SETTING_NAME_CHANNEL		"channel"
#define WIFIMGR_SETTING_NAME_CHANNEL_WIDTH	"ch_width"

#define WIFIMGR_SETTING_PATH		"wifimgr"
#define WIFIMGR_SETTING_STA_PATH	"wifimgr/sta"
#define WIFIMGR_SETTING_AP_PATH		"wifimgr/ap"

struct wifimgr_settings_map {
	enum settings_type type;
	char name[WIFIMGR_SETTING_NAME_LEN + 1];
	char *valptr;
	int vallen;
	bool mask;
};

#ifdef CONFIG_WIFIMGR_CONFIG
int wifimgr_config_init(void *handle);
int wifimgr_config_load(void *handle, char *path);
int wifimgr_settings_save(void *handle, char *path, bool clear);
#define wifimgr_config_save(...)	wifimgr_settings_save(__VA_ARGS__, false)
#define wifimgr_config_clear(...)	wifimgr_settings_save(__VA_ARGS__, true)
#else
#define wifimgr_config_init(...)
#define wifimgr_config_load(...)
#define wifimgr_config_save(...)
#define wifimgr_config_clear(...)
#endif

#endif
