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

static const char *const wifimgr_setting_keynames[] = {
	WIFIMGR_SETTING_NAME_AUTORUN,
	WIFIMGR_SETTING_NAME_SSID,
	WIFIMGR_SETTING_NAME_BSSID,
	WIFIMGR_SETTING_NAME_PSK,
	WIFIMGR_SETTING_NAME_BAND,
	WIFIMGR_SETTING_NAME_CHANNEL,
	WIFIMGR_SETTING_NAME_CHANNEL_WIDTH,
};

static struct wifimgr_settings_map *wifimgr_sta_settings_map;
static struct wifimgr_settings_map *wifimgr_ap_settings_map;
static struct wifimgr_settings_map *settings;
static char *settings_category;

static int wifimgr_settings_set(int argc, char **argv, char *val)
{
	int cnt = ARRAY_SIZE(wifimgr_setting_keynames);
	int i;

	if (argc >= 1) {
		for (i = 0; i < cnt; i++) {
			wifimgr_dbg
			    ("argv[%d]:%s, argv[%d]:%s, settings[%d].name:%s\n",
			     argc - 2, argv[argc - 2], argc - 1, argv[argc - 1],
			     i, settings[i].name);

			if (!strcmp(argv[argc - 2], settings_category)
			    && !strcmp(argv[argc - 1], settings[i].name)) {
				if (settings[i].mask)
					continue;

				if (settings[i].type == SETTINGS_STRING) {
					memset(settings[i].valptr, 0,
					       settings[i].vallen);
					settings_bytes_from_str(val,
								settings[i].
								valptr,
								&settings[i].
								vallen);
					wifimgr_dbg("val: %s\n",
						    settings[i].valptr);
				} else if (settings[i].type == SETTINGS_INT8) {
					SETTINGS_VALUE_SET(val, SETTINGS_INT8,
							   *settings[i].valptr);
					wifimgr_dbg("val: %d\n",
						    *settings[i].valptr);
				}

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

static void wifimgr_settings_map_init(struct wifimgr_settings_map *settings_map,
				      struct wifimgr_config *conf, char *path)
{
	int i = 0;
	/* Initialize Autorun setting map */
	settings_map[i].type = SETTINGS_INT8;
	strcpy(settings_map[i].name, WIFIMGR_SETTING_NAME_AUTORUN);
	settings_map[i].valptr = &conf->autorun;
	i++;
	/* Initialize SSID setting map */
	settings_map[i].type = SETTINGS_STRING;
	strcpy(settings_map[i].name, WIFIMGR_SETTING_NAME_SSID);
	settings_map[i].valptr = conf->ssid;
	settings_map[i].vallen = sizeof(conf->ssid);
	i++;
	/* Initialize BSSID setting map */
	settings_map[i].type = SETTINGS_STRING;
	strcpy(settings_map[i].name, WIFIMGR_SETTING_NAME_BSSID);
	settings_map[i].valptr = conf->bssid;
	settings_map[i].vallen = sizeof(conf->bssid);
	if (!strcmp(path, WIFIMGR_SETTING_AP_PATH))
		settings_map[i].mask = true;
	i++;
	/* Initialize PSK setting map */
	settings_map[i].type = SETTINGS_STRING;
	strcpy(settings_map[i].name, WIFIMGR_SETTING_NAME_PSK);
	settings_map[i].valptr = conf->passphrase;
	settings_map[i].vallen = sizeof(conf->passphrase);
	i++;
	/* Initialize Band setting map */
	settings_map[i].type = SETTINGS_INT8;
	strcpy(settings_map[i].name, WIFIMGR_SETTING_NAME_BAND);
	settings_map[i].valptr = &conf->band;
	i++;
	/* Initialize Channel setting map */
	settings_map[i].type = SETTINGS_INT8;
	strcpy(settings_map[i].name, WIFIMGR_SETTING_NAME_CHANNEL);
	settings_map[i].valptr = &conf->channel;
	i++;
	/* Initialize Channel width setting map */
	settings_map[i].type = SETTINGS_INT8;
	strcpy(settings_map[i].name, WIFIMGR_SETTING_NAME_CHANNEL_WIDTH);
	settings_map[i].valptr = &conf->ch_width;
	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH))
		settings_map[i].mask = true;
}

int wifimgr_settings_save_one(struct wifimgr_settings_map *setting, char *path,
			      bool clear)
{
	char abs_path[WIFIMGR_SETTING_NAME_LEN + 1];
	char val[WIFIMGR_SETTING_VAL_LEN];
	char *valptr = NULL;
	int ret = 0;

	if (setting->mask)
		return 0;

	if (setting->type == SETTINGS_STRING) {
		if (!strcmp(setting->name, WIFIMGR_SETTING_NAME_BSSID)
		    && is_zero_ether_addr(setting->valptr) && !clear)
			return 0;
		else if (!strlen(setting->valptr) && !clear)
			return 0;

		wifimgr_dbg("name:%s, val:%s\n", setting->name,
			    setting->valptr);
		valptr =
		    settings_str_from_bytes(setting->valptr, setting->vallen,
					    val, sizeof(val));
	} else if (setting->type == SETTINGS_INT8) {
		if ((*setting->valptr == 0) && !clear)
			return 0;

		wifimgr_dbg("name:%s, val:%d\n", setting->name,
			    *setting->valptr);
		valptr =
		    settings_str_from_value(SETTINGS_INT8, setting->valptr, val,
					    sizeof(val));
	}

	if (!valptr) {
		wifimgr_err("failed to convert %s!\n", setting->name);
		return -EINVAL;
	}

	snprintf(abs_path, sizeof(abs_path), "%s/%s", path, setting->name);
	ret = settings_save_one(abs_path, valptr);
	if (ret)
		wifimgr_err("failed to save %s, ret: %d!\n", abs_path, ret);

	return ret;
}

/*int wifimgr_settings_save(bool clear, void *handle, char *path)*/
int wifimgr_settings_save(void *handle, char *path, bool clear)
{
	int cnt = ARRAY_SIZE(wifimgr_setting_keynames);
	int i;
	int ret;

	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH)) {
		settings = wifimgr_sta_settings_map;
	} else if (!strcmp(path, WIFIMGR_SETTING_AP_PATH)) {
		settings = wifimgr_ap_settings_map;
	} else {
		wifimgr_err("unsupported path %s!\n", path);
		return -ENOENT;
	}

	for (i = 0; i < cnt; i++) {
		ret = wifimgr_settings_save_one(&settings[i], path, clear);
		if (ret)
			break;
	}

	return ret;
}

int wifimgr_config_init(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int settings_size = sizeof(struct wifimgr_settings_map) *
	    ARRAY_SIZE(wifimgr_setting_keynames);
	int ret;

	ret = settings_subsys_init();
	if (ret)
		wifimgr_err("failed to init settings subsys!\n");

	/* Allocate memory space for STA settings */
	wifimgr_sta_settings_map = malloc(settings_size);
	if (!wifimgr_sta_settings_map)
		return -ENOMEM;

	/* Initialize STA settings */
	memset(wifimgr_sta_settings_map, 0, settings_size);
	wifimgr_settings_map_init(wifimgr_sta_settings_map, &mgr->sta_conf,
				  WIFIMGR_SETTING_STA_PATH);

	/* Allocate memory space for AP settings */
	wifimgr_ap_settings_map = malloc(settings_size);
	if (!wifimgr_ap_settings_map) {
		free(wifimgr_sta_settings_map);
		return -ENOMEM;
	}

	/* Initialize AP settings */
	memset(wifimgr_ap_settings_map, 0, settings_size);
	wifimgr_settings_map_init(wifimgr_ap_settings_map, &mgr->ap_conf,
				  WIFIMGR_SETTING_AP_PATH);

	ret = settings_register(&(wifimgr_settings_handler[0]));
	if (ret) {
		wifimgr_err("failed to register setting handlers!\n");
		free(wifimgr_sta_settings_map);
		free(wifimgr_ap_settings_map);
	}

	return ret;
}

int wifimgr_config_load(void *handle, char *path)
{
	int ret;

	settings_category = strstr(path, "/");
	settings_category++;

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
