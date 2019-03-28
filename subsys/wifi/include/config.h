/*
 * @file
 * @brief Configuration header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_CONFIG_H_
#define _WIFIMGR_CONFIG_H_

#include <settings/settings.h>

#include "os_adapter.h"

#define WIFIMGR_SETTING_NAME_LEN	63
#define WIFIMGR_SETTING_VAL_LEN	\
	(((((WIFIMGR_SETTING_NAME_LEN) / 3) * 4) + 4) + 1) /*base64 encoding*/

#define WIFIMGR_SETTING_NAME_SSID		"ssid"
#define WIFIMGR_SETTING_NAME_BSSID		"bssid"
#define WIFIMGR_SETTING_NAME_SECURITY		"security"
#define WIFIMGR_SETTING_NAME_PSPHR		"passphrase"
#define WIFIMGR_SETTING_NAME_BAND		"band"
#define WIFIMGR_SETTING_NAME_CHANNEL		"channel"
#define WIFIMGR_SETTING_NAME_CHANNEL_WIDTH	"ch_width"
#define WIFIMGR_SETTING_NAME_AUTORUN		"autorun"

#define WIFIMGR_SETTING_PATH		"wifimgr"
#define WIFIMGR_SETTING_STA_PATH	"wifimgr/sta"
#define WIFIMGR_SETTING_AP_PATH		"wifimgr/ap"

/**
 * Type of settings value.
 */
enum settings_type {
	SETTINGS_NONE = 0,
	SETTINGS_INT8,
	SETTINGS_INT16,
	SETTINGS_INT32,
	SETTINGS_INT64,
	SETTINGS_STRING,
	SETTINGS_BYTES,
	SETTINGS_FLOAT,
	SETTINGS_DOUBLE,
	SETTINGS_BOOL,
} __attribute__((__packed__));

struct wifimgr_settings_map {
	char name[WIFIMGR_SETTING_NAME_LEN + 1];
	void *valptr;
	int vallen;
	enum settings_type type;
	bool mask;
};

#ifdef CONFIG_WIFIMGR_CONFIG_SAVING
int wifimgr_settings_init(struct wifi_config *conf, char *path);
int wifimgr_config_init(void);
void wifimgr_config_exit(char *path);
int wifimgr_config_load(void *handle, char *path);
int wifimgr_settings_save(void *handle, char *path, bool clear);
#define wifimgr_config_save(...) \
	wifimgr_settings_save(__VA_ARGS__, false)
#define wifimgr_config_clear(...) \
	wifimgr_settings_save(__VA_ARGS__, true)
#else
#define wifimgr_config_exit(...)

static inline int wifimgr_settings_init(struct wifi_config *conf, char *path)
{
	return 0;
}

static inline int wifimgr_config_init(void)
{
	return 0;
}

static inline int wifimgr_config_load(void *handle, char *path)
{
	return 0;
}

static inline int wifimgr_config_save(void *handle, char *path)
{
	return 0;
}

static inline int wifimgr_config_clear(void *handle, char *path)
{
	return 0;
}
#endif

#endif
