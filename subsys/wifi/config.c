/*
 * @file
 * @brief Configuration handling
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#include "wifimgr.h"

static const char *wifimgr_setting_keynames[] = {
	WIFIMGR_SETTING_NAME_SSID,
	WIFIMGR_SETTING_NAME_BSSID,
	WIFIMGR_SETTING_NAME_PSK,
	WIFIMGR_SETTING_NAME_BAND,
	WIFIMGR_SETTING_NAME_CHANNEL,
};

static struct wifimgr_settings_map *wifimgr_sta_settings_map;
static struct wifimgr_settings_map *wifimgr_ap_settings_map;
static struct wifimgr_settings_map *settings;
static char *settings_category;

static int wifimgr_settings_set(int argc, char **argv, char *val)
{
	int i, cnt;

	if (argc >= 1) {
		for (i = 0, cnt = ARRAY_SIZE(wifimgr_setting_keynames); i < cnt; i++) {
			wifimgr_dbg("argv[%d]:%s, argv[%d]:%s, settings[%d].name:%s\n", argc - 2, argv[argc - 2], argc - 1, argv[argc - 1], i, settings[i].name);

			if(!strcmp(argv[argc - 2], settings_category) && !strcmp(argv[argc - 1], settings[i].name)) {
				if (settings[i].type == SETTINGS_STRING)
					strcpy(settings[i].valptr, val);
				else if (settings[i].type == SETTINGS_INT8)
					SETTINGS_VALUE_SET(val, SETTINGS_INT8, *settings[i].valptr);
			wifimgr_dbg("val:%s, settings[%d].val:%p\n", val, i, settings[i].valptr);
					break;
				}
		}
	}

	return 0;
}

static struct settings_handler wifimgr_settings_handler[] = {
	{
		.name = WIFIMGR_SETTING_PATH,
		.h_set = wifimgr_settings_set,
	}
};

static void wifimgr_settings_map_init(struct wifimgr_settings_map *settings_map, struct wifimgr_config *conf)
{
	/* Initialize SSID setting map */
	strcpy(settings_map[0].name, WIFIMGR_SETTING_NAME_SSID);
	settings_map[0].valptr = conf->ssid;
	settings_map[0].type = SETTINGS_STRING;
	/* Initialize BSSID setting map */
	strcpy(settings_map[1].name, WIFIMGR_SETTING_NAME_BSSID);
	settings_map[1].valptr = conf->bssid;
	settings_map[1].type = SETTINGS_STRING;
	/* Initialize PSK setting map */
	strcpy(settings_map[2].name, WIFIMGR_SETTING_NAME_PSK);
	settings_map[2].valptr = conf->passphrase;
	settings_map[2].type = SETTINGS_STRING;
	/* Initialize Band setting map */
	strcpy(settings_map[3].name, WIFIMGR_SETTING_NAME_BAND);
	settings_map[3].valptr = &conf->band;
	settings_map[3].type = SETTINGS_INT8;
	/* Initialize Channel setting map */
	strcpy(settings_map[4].name, WIFIMGR_SETTING_NAME_CHANNEL);
	settings_map[4].valptr = &conf->channel;
	settings_map[4].type = SETTINGS_INT8;
}

int wifimgr_config_init(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int settings_size = sizeof(struct wifimgr_settings_map) * ARRAY_SIZE(wifimgr_setting_keynames);
	int ret;

	ret = settings_subsys_init();
	if (ret)
		wifimgr_err("failed to init settings subsys!\n");

	/* Allocate memory space for STA settings */
	wifimgr_sta_settings_map = malloc(settings_size);
	if (!wifimgr_sta_settings_map)
		return -ENOMEM;

	/* Initialize STA settings */
	memset(wifimgr_sta_settings_map, 0 , settings_size);
	wifimgr_settings_map_init(wifimgr_sta_settings_map, &mgr->sta_conf);

	/* Allocate memory space for AP settings */
	wifimgr_ap_settings_map = malloc(settings_size);
	if (!wifimgr_ap_settings_map) {
		free(wifimgr_sta_settings_map);
		return -ENOMEM;
	}

	/* Initialize AP settings */
	memset(wifimgr_ap_settings_map, 0 , settings_size);
	wifimgr_settings_map_init(wifimgr_ap_settings_map, &mgr->ap_conf);

	ret = settings_register(&(wifimgr_settings_handler[0]));
	if (ret) {
		wifimgr_err("failed to register setting handlers!\n");
		free(wifimgr_sta_settings_map);
		free(wifimgr_ap_settings_map);
	}

	return ret;
}

static char *wifimgr_strtok(char *str, const char *delim)
{
	char *ptr;

	ptr = strstr(str, delim);
	if (!ptr)
		return NULL;

	return (ptr + 1);
}

int wifimgr_load_config(void *handle, char *path)
{
	int ret;

	settings_category = wifimgr_strtok(path, "/");

	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH)) {
		settings = wifimgr_sta_settings_map;
	} else if (!strcmp(path, WIFIMGR_SETTING_AP_PATH)) {
		settings = wifimgr_ap_settings_map;
	} else {
		wifimgr_err("unsupported path %s!\n", path);
		return -ENOENT;
	}

	ret = settings_load();
	if (ret) {
		wifimgr_err("failed to load config!\n");
		return ret;
	}

	return ret;
}

int wifimgr_save_config(void *handle, char *path)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;
	char name[WIFIMGR_SETTING_NAME_LEN + 1];
	char val[WIFIMGR_SETTING_VAL_LEN + 1];
	int ret = 0;

	if (strcmp(path, WIFIMGR_SETTING_STA_PATH) && strcmp(path, WIFIMGR_SETTING_AP_PATH)) {
		wifimgr_err("wrong path: %s!\n", path);
		return -ENOENT;
	}

	/* Save SSID */
	snprintf(name, sizeof(name), "%s/%s", path, WIFIMGR_SETTING_NAME_SSID);
	if (strlen(conf->ssid))
		ret = settings_save_one(name, conf->ssid);
	else
		ret = settings_save_one(name, "");
	if (ret)
		wifimgr_err("failed to save %s!\n", name);

	/* Save BSSID */
	snprintf(name, sizeof(name), "%s/%s", path, WIFIMGR_SETTING_NAME_BSSID);
	if (!is_zero_ether_addr(conf->bssid))
		ret = settings_save_one(name, conf->bssid);
	else
		ret = settings_save_one(name, "");
	if (ret)
		wifimgr_err("failed to save %s!\n", name);

	/* Save passphrase */
	snprintf(name, sizeof(name), "%s/%s", path, WIFIMGR_SETTING_NAME_PSK);
	if (strlen(conf->passphrase))
		ret = settings_save_one(name, conf->passphrase);
	else
		ret = settings_save_one(name, "");
	if (ret)
		wifimgr_err("failed to save %s!\n", name);

	/* Save band */
	snprintf(name, sizeof(name), "%s/%s", path, WIFIMGR_SETTING_NAME_BAND);
	settings_str_from_value(SETTINGS_INT8, &conf->band, val, sizeof(val));
	ret = settings_save_one(name, val);
	if (ret)
		wifimgr_err("failed to save %s!\n", name);

	/* Save channel */
	snprintf(name, sizeof(name), "%s/%s", path, WIFIMGR_SETTING_NAME_CHANNEL);
	settings_str_from_value(SETTINGS_INT8, &conf->channel, val, sizeof(val));
	ret = settings_save_one(name, val);
	if (ret)
		wifimgr_err("failed to save %s!\n", name);

	return ret;
}
