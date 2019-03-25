/*
 * @file
 * @brief Autorun WiFi repeater, using both STA and AP.
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
#include <net/wifi_api.h>
#include <posix/sys/types.h>
#include <posix/time.h>

#ifdef CONFIG_WIFIMGR_STA
#define WIFIMGR_AUTORUN_STA_RETRY	(3)
struct k_work sta_autowork;
timer_t sta_autorun_timerid;
struct wifi_config sta_config;
struct wifi_status sta_status;
#endif

#ifdef CONFIG_WIFIMGR_AP
struct k_work ap_autowork;
timer_t ap_autorun_timerid;
struct wifi_config ap_config;
struct wifi_status ap_status;
#endif

static void wifi_autorun_timer_handler(void *sival_ptr)
{
	struct k_work *work = (struct k_work *)sival_ptr;

	k_work_submit(work);
}

static int wifi_autorun_set_timer(timer_t timerid, unsigned int sec)
{
	struct itimerspec todelay;
	int ret;

	/* Start, restart, or stop the timer */
	todelay.it_value.tv_sec = sec;
	todelay.it_value.tv_nsec = 0;
	todelay.it_interval.tv_sec = 0;
	todelay.it_interval.tv_nsec = 0;

	ret = timer_settime(timerid, 0, &todelay, NULL);
	if (ret == -1)
		ret = -errno;

	return ret;
}

static void wifi_autorun_scan_result(struct wifi_scan_result *scan_res)
{
	/* Find specified AP */
	if (!strcmp(scan_res->ssid, sta_config.ssid)) {
		printf("SSID:\t\t%s\n", sta_config.ssid);
		/* Choose the first match when BSSID is not specified */
		if (is_zero_ether_addr(sta_config.bssid))
			sta_status.u.sta.host_found = 1;
		else if (!strncmp
			 (scan_res->bssid, sta_config.bssid, WIFI_MAC_ADDR_LEN))
			sta_status.u.sta.host_found = 1;
	}
}

#ifdef CONFIG_WIFIMGR_STA
static void wifi_autorun_notify_connect(union wifi_notifier_val val)
{
	int status = val.val_char;

	if (!status) {
		printf("connect successfully!\n");
		sta_status.state = WIFI_STATE_STA_CONNECTED;
	} else {
		printf("failed to connect!\n");
	}

	wifi_autorun_set_timer(sta_autorun_timerid, 0);
}

static void wifi_autorun_notify_disconnect(union wifi_notifier_val val)
{
	int reason_code = val.val_char;

	printf("disconnect! reason: %d\n", reason_code);
	sta_status.state = WIFI_STATE_STA_READY;
	wifi_autorun_set_timer(sta_autorun_timerid, sta_config.autorun);
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static void wifi_autorun_notify_new_station(union wifi_notifier_val val)
{
	char *mac = val.val_ptr;

	printf("Station (" MACSTR ") connected!\n", MAC2STR(mac));
}

static void wifi_autorun_notify_station_leave(union wifi_notifier_val val)
{
	char *mac = val.val_ptr;

	printf("Station (" MACSTR ") disconnected!\n", MAC2STR(mac));
}
#endif

#ifdef CONFIG_WIFIMGR_STA
static void wifi_autorun_sta(struct k_work *work)
{
	int cnt;
	int ret;

	/* Get STA config */
	ret = wifi_sta_get_conf(&sta_config);
	if (ret) {
		printf("failed to get_conf! %d\n", ret);
		goto exit;
	}
	if (sta_config.autorun <= 0) {
		sta_config.autorun = 0;
		if (sta_config.autorun < 0)
			printf("STA autorun disabled!\n");
		goto exit;
	}
	if (!sta_config.ssid || !strlen(sta_config.ssid)) {
		printf("no STA config!\n");
		goto exit;
	}

	/* Get STA status */
	ret = wifi_sta_get_status(&sta_status);
	if (ret) {
		printf("failed to get_status! %d\n", ret);
		goto exit;
	}

	switch (sta_status.state) {
	case WIFI_STATE_STA_NODEV:
		/* Open STA */
		ret = wifi_sta_open();
		if (ret) {
			printf("failed to open! %d\n", ret);
			goto exit;
		}
	case WIFI_STATE_STA_READY:
		/* Trigger STA scan */
		for (cnt = 0; cnt < WIFIMGR_AUTORUN_STA_RETRY; cnt++) {
			ret = wifi_sta_scan(wifi_autorun_scan_result);
			if (ret)
				printf("failed to scan! %d\n", ret);
			if (sta_status.u.sta.host_found)
				break;
		}
		if (cnt == WIFIMGR_AUTORUN_STA_RETRY) {
			printf("maximum scan retry reached!\n");
			goto exit;
		}

		/* Connect the AP */
		ret = wifi_sta_connect();
		if (ret) {
			printf("failed to connect! %d\n", ret);
			goto exit;
		}
	case WIFI_STATE_STA_CONNECTED:
			goto out;

		break;
	default:
		printf("nothing else to do under %s!\n",
			    sta_sts2str(sta_status.state));
		break;
	}
exit:
	/* Set timer for the next run */
	wifi_autorun_set_timer(sta_autorun_timerid, sta_config.autorun);
out:
	return;
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static void wifi_autorun_ap(struct k_work *work)
{
	int ret;

	/* Get AP config */
	ret = wifi_ap_get_conf(&ap_config);
	if (ret) {
		printf("failed to get_conf! %d\n", ret);
		goto exit;
	}
	if (ap_config.autorun <= 0) {
		ap_config.autorun = 0;
		if (ap_config.autorun < 0)
			printf("AP autorun disabled!\n");
		goto exit;
	}
	if (!ap_config.ssid || !strlen(ap_config.ssid)) {
		printf("no AP config!\n");
		goto exit;
	}

	/* Get AP status */
	ret = wifi_ap_get_status(&ap_status);
	if (ret) {
		printf("failed to get_status! %d\n", ret);
		goto exit;
	}

	switch (ap_status.state) {
	case WIFI_STATE_AP_NODEV:
		/* Open AP */
		ret = wifi_ap_open();
		if (ret) {
			printf("failed to open! %d\n", ret);
			goto exit;
		}
	case WIFI_STATE_AP_READY:
		/* Start AP */
		ret = wifi_ap_start_ap();
		if (!ret) {
			printf("done!\n");
			goto out;
		}

		printf("failed to start AP! %d\n", ret);
		goto exit;

		break;
	default:
		printf("nothing else to do under %s!\n",
			    ap_sts2str(ap_status.state));
		goto exit;
	}

exit:
	/* Set timer for the next run */
	wifi_autorun_set_timer(ap_autorun_timerid, ap_config.autorun);
out:
	return;
}
#endif

int wifi_autorun_init(void)
{
	struct k_work *autowork;
	struct sigevent toevent;
	int ret;
#ifdef CONFIG_WIFIMGR_STA
	autowork = &sta_autowork;
	k_work_init(autowork, (void *)wifi_autorun_sta);
	/* Create a POSIX timer to handle timeouts */
	toevent.sigev_value.sival_ptr = autowork;
	toevent.sigev_notify = SIGEV_SIGNAL;
	toevent.sigev_notify_function = (void *)wifi_autorun_timer_handler;
	toevent.sigev_notify_attributes = NULL;

	ret = timer_create(CLOCK_MONOTONIC, &toevent, &sta_autorun_timerid);
	if (ret == -1) {
		printf("failed to init STA autorun!\n");
		ret = -errno;
	}

	wifi_register_connection_notifier(wifi_autorun_notify_connect);
	wifi_register_disconnection_notifier(wifi_autorun_notify_disconnect);

	/* Set timer for the first run */
	ret = wifi_autorun_set_timer(sta_autorun_timerid, 1);
	if (ret < 0) {
		printf("failed to start STA autorun!\n");
		wifi_unregister_connection_notifier
		    (wifi_autorun_notify_connect);
		wifi_unregister_disconnection_notifier
		    (wifi_autorun_notify_disconnect);
	}

	printf("start STA autorun!\n");
#endif
#ifdef CONFIG_WIFIMGR_AP
	autowork = &ap_autowork;
	k_work_init(autowork, (void *)wifi_autorun_ap);
	/* Create a POSIX timer to handle timeouts */
	toevent.sigev_value.sival_ptr = autowork;
	toevent.sigev_notify = SIGEV_SIGNAL;
	toevent.sigev_notify_function = (void *)wifi_autorun_timer_handler;
	toevent.sigev_notify_attributes = NULL;

	ret = timer_create(CLOCK_MONOTONIC, &toevent, &ap_autorun_timerid);
	if (ret == -1) {
		printf("failed to init AP autorun!\n");
		ret = -errno;
	}

	wifi_register_new_station_notifier
	    (wifi_autorun_notify_new_station);
	wifi_register_station_leave_notifier
	    (wifi_autorun_notify_station_leave);
	/* Set timer for the first run */
	ret = wifi_autorun_set_timer(ap_autorun_timerid, 1);
	if (ret < 0) {
		printf("failed to start AP autorun!\n");
		wifi_unregister_new_station_notifier
		    (wifi_autorun_notify_new_station);
		wifi_unregister_station_leave_notifier
		    (wifi_autorun_notify_station_leave);
	}

	printf("start AP autorun!\n");
#endif
	return ret;
}

#endif
