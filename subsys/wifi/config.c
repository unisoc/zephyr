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
	WIFIMGR_SETTING_NAME_SSID,
	WIFIMGR_SETTING_NAME_BSSID,
	WIFIMGR_SETTING_NAME_SECURITY,
	WIFIMGR_SETTING_NAME_PSPHR,
	WIFIMGR_SETTING_NAME_BAND,
	WIFIMGR_SETTING_NAME_CHANNEL,
	WIFIMGR_SETTING_NAME_CHANNEL_WIDTH,
	WIFIMGR_SETTING_NAME_AUTORUN,
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
								settings
								[i].valptr,
								&settings
								[i].vallen);
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

static struct settings_handler wifimgr_settings_handler = {
	.name = WIFIMGR_SETTING_PATH,
	.h_set = wifimgr_settings_set,
};

int wifimgr_settings_save_one(struct wifimgr_settings_map *setting, char *path,
			      bool clear)
{
	char abs_path[WIFIMGR_SETTING_NAME_LEN + 1];
	char val[WIFIMGR_SETTING_VAL_LEN];
	char *valptr = NULL;
	int ret;

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
		wifimgr_err("failed to save %s! %d\n", abs_path, ret);

	return ret;
}

int wifimgr_settings_save(void *handle, char *path, bool clear)
{
	int cnt = ARRAY_SIZE(wifimgr_setting_keynames);
	int i;
	int ret;

	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH)
	    && wifimgr_sta_settings_map) {
		settings = wifimgr_sta_settings_map;
	} else if (!strcmp(path, WIFIMGR_SETTING_AP_PATH)
		   && wifimgr_ap_settings_map) {
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

static
void wifimgr_settings_init_one(struct wifimgr_settings_map *setting,
			       const char *name, char *valptr, int vallen,
			       enum settings_type type, bool mask)
{
	strcpy(setting->name, name);
	setting->valptr = valptr;
	setting->vallen = vallen;
	setting->type = type;
	setting->mask = mask;
}

static int wifimgr_settings_init(struct wifimgr_config *conf, char *path)
{
	int settings_size = sizeof(struct wifimgr_settings_map) *
	    ARRAY_SIZE(wifimgr_setting_keynames);
	bool mask;
	int i = 0;

	/* Allocate memory for settings map */
	settings = malloc(settings_size);
	if (!settings)
		return -ENOMEM;

	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH)) {
		wifimgr_sta_settings_map = settings;
	} else if (!strcmp(path, WIFIMGR_SETTING_AP_PATH)) {
		wifimgr_ap_settings_map = settings;
	} else {
		wifimgr_err("unsupported path %s!\n", path);
		return -ENOENT;
	}

	memset(settings, 0, settings_size);

	/* Initialize SSID setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  conf->ssid, sizeof(conf->ssid),
				  SETTINGS_STRING, false);
	i++;
	/* Initialize BSSID setting map */
	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH))
		mask = false;
	else
		mask = true;
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  conf->bssid, sizeof(conf->bssid),
				  SETTINGS_STRING, mask);
	i++;
	/* Initialize Security setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  &conf->security, sizeof(conf->security),
				  SETTINGS_INT8, false);
	i++;
	/* Initialize Passphrase setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  conf->passphrase, sizeof(conf->passphrase),
				  SETTINGS_STRING, false);
	i++;
	/* Initialize Band setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  &conf->band, sizeof(conf->band),
				  SETTINGS_INT8, false);
	i++;
	/* Initialize Channel setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  &conf->channel, sizeof(conf->channel),
				  SETTINGS_INT8, false);
	i++;
	/* Initialize Channel width setting map */
	if (!strcmp(path, WIFIMGR_SETTING_AP_PATH))
		mask = false;
	else
		mask = true;
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  &conf->ch_width, sizeof(conf->ch_width),
				  SETTINGS_INT8, false);
	i++;
	/* Initialize Autorun setting map */
	wifimgr_settings_init_one(&settings[i], wifimgr_setting_keynames[i],
				  &conf->autorun, sizeof(conf->autorun),
				  SETTINGS_INT8, false);

	return 0;
}

int wifimgr_config_load(void *handle, char *path)
{
	int ret;

	settings_category = strstr(path, "/");
	settings_category++;

	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH)
	    && wifimgr_sta_settings_map) {
		settings = wifimgr_sta_settings_map;
	} else if (!strcmp(path, WIFIMGR_SETTING_AP_PATH)
		   && wifimgr_ap_settings_map) {
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

int wifimgr_config_init(void *handle, char *path)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		wifimgr_err("failed to init settings subsys! %d\n", ret);
		return ret;
	}

	ret = settings_register(&wifimgr_settings_handler);
	if (ret) {
		wifimgr_err("failed to register setting handlers! %d\n", ret);
		return ret;
	}

	ret = wifimgr_settings_init(conf, path);
	if (ret)
		wifimgr_err("failed to init settings map! %d\n", ret);

	return ret;
}

void wifimgr_config_exit(char *path)
{
	if (!strcmp(path, WIFIMGR_SETTING_STA_PATH)) {
		settings = wifimgr_sta_settings_map;
		wifimgr_sta_settings_map = NULL;
	} else if (!strcmp(path, WIFIMGR_SETTING_AP_PATH)) {
		settings = wifimgr_ap_settings_map;
		wifimgr_ap_settings_map = NULL;
	} else {
		wifimgr_err("unsupported path %s!\n", path);
		return;
	}

	free(settings);
}
