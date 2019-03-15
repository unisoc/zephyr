/*
 * @file
 * @brief Autorun of WiFi manager, including both STA and AP.
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_WIFIMGR_STA) || defined(CONFIG_WIFIMGR_AP)

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#include <init.h>
#include <net/wifimgr_api.h>

#include "wifimgr.h"

/*#define WIFIMGR_AUTORUN_PRIORITY       91*/

#ifdef CONFIG_WIFIMGR_STA
#define WIFIMGR_AUTORUN_STA_RETRY	(3)
wifimgr_work sta_work;
timer_t sta_autorun_timerid;
struct wifi_config sta_config;
struct wifi_status sta_status;
static bool sta_connected;
#endif

#ifdef CONFIG_WIFIMGR_AP
wifimgr_work ap_work;
timer_t ap_autorun_timerid;
struct wifi_config ap_config;
struct wifi_status ap_status;
#endif

#ifdef CONFIG_WIFIMGR_STA
static void wifimgr_autorun_scan_result(struct wifi_scan_result *scan_res)
{
	/* Find specified AP */
	if (!strcmp(scan_res->ssid, sta_config.ssid)) {
		printf("SSID:\t\t%s\n", sta_config.ssid);
		/* Choose the first match when BSSID is not specified */
		if (is_zero_ether_addr(sta_config.bssid))
			sta_status.u.sta.host_found = 1;
		else if (!strncmp(scan_res->bssid, sta_config.bssid, WIFIMGR_ETH_ALEN))
			sta_status.u.sta.host_found = 1;
	}
}

static void wifimgr_autorun_notify_connect(union wifi_notifier_val val)
{
	int status = val.val_char;

	printf("%s status %d\n", __func__, status);
	wifimgr_timer_stop(sta_autorun_timerid);
	if (!status)
		sta_connected = true;
}

static void wifimgr_autorun_notify_disconnect(union wifi_notifier_val val)
{
	int reason_code = val.val_char;

	printf("%s reason %d\n", __func__, reason_code);
	sta_connected = false;
	wifimgr_timer_start(sta_autorun_timerid, sta_config.autorun);
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static void wifimgr_autorun_notify_new_station(union wifi_notifier_val val)
{
	char *mac = val.val_ptr;

	printf("MAC:\t\t" MACSTR "\n", MAC2STR(mac));
}
#endif

#ifdef CONFIG_WIFIMGR_STA
static void wifimgr_autorun_sta(wifimgr_work *work)
{
	int cnt;
	int ret;

	/* Get STA config */
	ret = wifi_sta_get_conf(&sta_config);
	if (ret) {
		wifimgr_err("failed to get_conf! %d\n", ret);
		goto exit;
	}
	if (sta_config.autorun <= 0) {
		sta_config.autorun = 0;
		if (sta_config.autorun < 0)
			printf("STA autorun disabled!\n");
		goto exit;
	}
	if (!sta_config.ssid || !strlen(sta_config.ssid)) {
		wifimgr_warn("no STA config!\n");
		goto exit;
	}

	/* Get STA status */
	ret = wifi_sta_get_status(&sta_status);
	if (ret) {
		wifimgr_err("failed to get_status! %d\n", ret);
		goto exit;
	}

	switch (sta_status.state) {
	case WIFIMGR_SM_STA_NODEV:
		/* Open STA */
		ret = wifi_sta_open();
		if (ret) {
			wifimgr_err("failed to open! %d\n", ret);
			goto exit;
		}
	case WIFIMGR_SM_STA_READY:
		/* Trigger STA scan */
		for (cnt = 0; cnt < WIFIMGR_AUTORUN_STA_RETRY; cnt++) {
	printf("%s %d\n", __func__, __LINE__);
			ret = wifi_sta_scan(wifimgr_autorun_scan_result);
	printf("%s %d\n", __func__, __LINE__);
			if (ret)
				wifimgr_err("failed to scan! %d\n", ret);
			if (sta_status.u.sta.host_found)
				break;
		}
		if (cnt == WIFIMGR_AUTORUN_STA_RETRY) {
			wifimgr_err("maximum scan retry reached!\n");
			goto exit;
		}

		/* Connect the AP */
	printf("%s %d\n", __func__, __LINE__);
		ret = wifi_sta_connect();
	printf("%s %d\n", __func__, __LINE__);
		if (ret) {
			wifimgr_err("failed to connect! %d\n", ret);
			goto exit;
		}
	case WIFIMGR_SM_STA_CONNECTED:
		if (sta_connected == true)
			goto out;

		break;
	default:
		wifimgr_dbg("nothing else to do under %s!\n",
			    sta_sts2str(sta_status.state));
		break;
	}
exit:
	/* Set timer for the next run */
	wifimgr_timer_start(sta_autorun_timerid, sta_config.autorun);
out:
	return;
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static void wifimgr_autorun_ap(wifimgr_work *work)
{
	int ret;

	/*[> Get control of AP <]
	ret = wifimgr_get_ctrl(iface_name);
	if (ret) {
		wifimgr_err("failed to get ctrl! %d\n", ret);
		goto exit;
	}*/

	/* Get AP config */
	ret = wifi_ap_get_conf(&ap_config);
	if (ret) {
		wifimgr_err("failed to get_conf! %d\n", ret);
		goto exit;
	}
	if (ap_config.autorun <= 0) {
		ap_config.autorun = 0;
		if (ap_config.autorun < 0)
			printf("AP autorun disabled!\n");
		goto exit;
	}
	if (!ap_config.ssid || !strlen(ap_config.ssid)) {
		wifimgr_warn("no AP config!\n");
		goto exit;
	}

	/* Get AP status */
	ret = wifi_ap_get_status(&ap_status);
	if (ret) {
		wifimgr_err("failed to get_status! %d\n", ret);
		goto exit;
	}

	switch (ap_status.state) {
	case WIFIMGR_SM_AP_NODEV:
		/* Open AP */
		ret = wifi_ap_open();
		if (ret) {
			wifimgr_err("failed to open! %d\n", ret);
			goto exit;
		}
	case WIFIMGR_SM_AP_READY:
		/* Start AP */
		ret = wifi_ap_start_ap();
		if (ret) {
			wifimgr_err("failed to start AP! %d\n", ret);
			goto exit;
		}
		wifimgr_info("done!\n");

		break;
	default:
		wifimgr_dbg("nothing else to do under %s!\n",
			    ap_sts2str(ap_status.state));
		goto exit;
	}

exit:
	/* Set timer for the next run */
	wifimgr_timer_start(ap_autorun_timerid, ap_config.autorun);
	/* Release control of STA */
	/*wifimgr_release_ctrl(iface_name);*/
}
#endif

int wifimgr_autorun_init(void)
{
	int ret;
#ifdef CONFIG_WIFIMGR_STA
	wifimgr_init_work(&sta_work, (void *)wifimgr_autorun_sta);
	ret = wifimgr_timer_init(&sta_work, wifimgr_timeout,
				 &sta_autorun_timerid);
	if (ret < 0)
		wifimgr_err("failed to init STA autorun!\n");

	wifi_register_connection_notifier(wifimgr_autorun_notify_connect);
	wifi_register_disconnection_notifier(wifimgr_autorun_notify_disconnect);

	/* Set timer for the first run */
	ret = wifimgr_timer_start(sta_autorun_timerid, 1);
	if (ret < 0) {
		wifimgr_err("failed to start STA autorun!\n");
		wifi_unregister_connection_notifier(wifimgr_autorun_notify_connect);
		wifi_unregister_disconnection_notifier(wifimgr_autorun_notify_disconnect);
	}
	sta_config.autorun = 60;
#endif
#ifdef CONFIG_WIFIMGR_AP
	wifimgr_init_work(&ap_work, (void *)wifimgr_autorun_ap);
	ret = wifimgr_timer_init(&ap_work, wifimgr_timeout,
				 &ap_autorun_timerid);
	if (ret < 0)
		wifimgr_err("failed to init AP autorun!\n");

	wifimgr_register_new_station_notifier(wifimgr_autorun_notify_new_station);
	/* Set timer for the first run */
	/*ret = wifimgr_timer_start(ap_autorun_timerid, 1);*/
	if (ret < 0)
		wifimgr_err("failed to start AP autorun!\n");
#endif
	return ret;
}

	/*wifi_register_new_station_notifier(new_station_notifier);
	wifi_register_new_station_notifier(station_leave_notifier);

	wifi_unregister_new_station_notifier(new_station_notifier);
	wifi_unregister_new_station_notifier(station_leave_notifier);*/
#endif
