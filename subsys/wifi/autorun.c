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

static sem_t sta_sem;
wifimgr_work sta_work;
timer_t sta_autorun_timerid;
static int sta_autorun = 60;
static char sta_state;
static char *host_ssid;
static char host_found;
static bool sta_connected;
#endif

#ifdef CONFIG_WIFIMGR_AP
static sem_t ap_sem;
wifimgr_work ap_work;
timer_t ap_autorun_timerid;
static char *ap_ssid;
static int ap_autorun;
static char ap_state;
#endif

#ifdef CONFIG_WIFIMGR_STA
static
void wifimgr_autorun_get_sta_conf_cb(char *ssid, char *bssid, char *passphrase,
				     unsigned char band, unsigned char channel,
				     enum wifimgr_security security,
				     int autorun)
{
	host_ssid = ssid;
	sta_autorun = autorun;
	sem_post(&sta_sem);
}

static
void wifimgr_autorun_get_sta_status_cb(char status, char *own_mac,
				       char *host_bssid, signed char host_rssi)
{
	sta_state = status;
	sem_post(&sta_sem);
}

static void wifimgr_autorun_notify_scan_done(char result)
{
	host_found = result;
	sem_post(&sta_sem);
}


static void wifimgr_autorun_notify_disconnect(union wifi_notifier_val val)
{
	int reason_code = val.val_char;

	printf("%s reason %d\n", __func__, reason_code);
	sta_connected = false;
	wifimgr_timer_start(sta_autorun_timerid, sta_autorun);
}

static void wifimgr_autorun_notify_connect(union wifi_notifier_val val)
{
	int result = val.val_char;

	printf("%s result %d\n", __func__, result);
	wifimgr_timer_stop(sta_autorun_timerid);
	if (!result)
		sta_connected = true;
	sem_post(&sta_sem);
}

static void wifimgr_autorun_notify_scan_timeout(void)
{
	host_found = 0;
	sem_post(&sta_sem);
}

static void wifimgr_autorun_notify_connect_timeout(void)
{
	sta_connected = false;
	sem_post(&sta_sem);
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static
void wifimgr_autorun_get_ap_conf_cb(char *ssid, char *passphrase,
				    unsigned char band, unsigned char channel,
				    unsigned char ch_width,
				    enum wifimgr_security security, int autorun)
{
	ap_ssid = ssid;
	ap_autorun = autorun;
	sem_post(&ap_sem);
}

static
void wifimgr_autorun_get_ap_status_cb(char status, char *own_mac,
				      unsigned char sta_nr,
				      char sta_mac_addrs[][6],
				      unsigned char acl_nr,
				      char acl_mac_addrs[][6])
{
	ap_state = status;
	sem_post(&ap_sem);
}
static void wifimgr_autorun_notify_new_station(union wifi_notifier_val val)
{
	char *mac = val.val_ptr;

	printf("MAC:\t\t" MACSTR "\n", MAC2STR(mac));
}
#endif

static struct wifimgr_ctrl_cbs wifimgr_autorun_cbs = {
#ifdef CONFIG_WIFIMGR_STA
	.get_sta_conf_cb = wifimgr_autorun_get_sta_conf_cb,
	.get_sta_status_cb = wifimgr_autorun_get_sta_status_cb,
	.notify_scan_done = wifimgr_autorun_notify_scan_done,
	.notify_scan_timeout = wifimgr_autorun_notify_scan_timeout,
	.notify_connect_timeout = wifimgr_autorun_notify_connect_timeout,
#endif
#ifdef CONFIG_WIFIMGR_AP
	.get_ap_conf_cb = wifimgr_autorun_get_ap_conf_cb,
	.get_ap_status_cb = wifimgr_autorun_get_ap_status_cb,
#endif
};

#ifdef CONFIG_WIFIMGR_STA
static void wifimgr_autorun_sta(wifimgr_work *work)
{
	char *iface_name = WIFIMGR_IFACE_NAME_STA;
	int cnt;
	int ret;

	/* Check STA ops */
	if (!wifimgr_get_ctrl_ops()->get_conf
	    || !wifimgr_get_ctrl_ops()->get_status
	    || !wifimgr_get_ctrl_ops()->open
	    || !wifimgr_get_ctrl_ops()->scan
	    || !wifimgr_get_ctrl_ops()->connect) {
		wifimgr_err("incomplete ops!\n");
		return;
	}

	/* Get control of STA */
	ret = wifimgr_get_ctrl(iface_name);
	if (ret) {
		wifimgr_err("failed to get ctrl! %d\n", ret);
		goto exit;
	}

	sem_init(&sta_sem, 0, 0);
	/* Get STA config */
	ret =
	    wifimgr_get_ctrl_ops_cbs(&wifimgr_autorun_cbs)->
	    get_conf(iface_name);
	if (ret) {
		wifimgr_err("failed to get_conf! %d\n", ret);
		goto exit;
	}
	sem_wait(&sta_sem);
	if (sta_autorun <= 0) {
		sta_autorun = 0;
		if (sta_autorun < 0)
			printf("STA autorun disabled!\n");
		goto exit;
	}
	if (!host_ssid || !strlen(host_ssid)) {
		wifimgr_warn("no STA config!\n");
		goto exit;
	}

	/* Get STA status */
	ret =
	    wifimgr_get_ctrl_ops_cbs(&wifimgr_autorun_cbs)->
	    get_status(iface_name);
	if (ret) {
		wifimgr_err("failed to get_status! %d\n", ret);
		goto exit;
	}
	sem_wait(&sta_sem);

	switch (sta_state) {
	case WIFIMGR_SM_STA_NODEV:
		/* Open STA */
		ret = wifimgr_get_ctrl_ops()->open(iface_name);
		if (ret) {
			wifimgr_err("failed to open! %d\n", ret);
			goto exit;
		}
	case WIFIMGR_SM_STA_READY:
		/* Trigger STA scan */
		host_found = 0;
		for (cnt = 0; cnt < WIFIMGR_AUTORUN_STA_RETRY; cnt++) {
			ret =
			    wifimgr_get_ctrl_ops_cbs(&wifimgr_autorun_cbs)->
			    scan();
			if (ret)
				wifimgr_err("failed to scan! %d\n", ret);
			else
				sem_wait(&sta_sem);
			if (host_found)
				break;
		}
		if (cnt == WIFIMGR_AUTORUN_STA_RETRY) {
			wifimgr_err("maximum scan retry reached!\n");
			goto exit;
		}

		/* Connect the AP */
		ret = wifimgr_get_ctrl_ops_cbs(&wifimgr_autorun_cbs)->connect();
		if (ret) {
			wifimgr_err("failed to connect! %d\n", ret);
			goto exit;
		}
		sem_wait(&sta_sem);
	case WIFIMGR_SM_STA_CONNECTED:
		if (sta_connected == true)
			goto out;

		break;
	default:
		wifimgr_dbg("nothing else to do under %s!\n",
			    sta_sts2str(sta_state));
		break;
	}
exit:
	/* Set timer for the next run */
	wifimgr_timer_start(sta_autorun_timerid, sta_autorun);
out:
	/* Release control of STA */
	wifimgr_release_ctrl(iface_name);
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static void wifimgr_autorun_ap(wifimgr_work *work)
{
	char *iface_name = WIFIMGR_IFACE_NAME_AP;
	int ret;

	/* Check AP ops */
	if (!wifimgr_get_ctrl_ops()->get_conf
	    || !wifimgr_get_ctrl_ops()->get_status
	    || !wifimgr_get_ctrl_ops()->open
	    || !wifimgr_get_ctrl_ops()->start_ap) {
		wifimgr_err("incomplete ops!\n");
		return;
	}

	/* Get control of AP */
	ret = wifimgr_get_ctrl(iface_name);
	if (ret) {
		wifimgr_err("failed to get ctrl! %d\n", ret);
		goto exit;
	}

	sem_init(&ap_sem, 0, 0);
	/* Get AP config */
	ret =
	    wifimgr_get_ctrl_ops_cbs(&wifimgr_autorun_cbs)->
	    get_conf(iface_name);
	if (ret) {
		wifimgr_err("failed to get_conf! %d\n", ret);
		goto exit;
	}
	sem_wait(&ap_sem);
	if (ap_autorun <= 0) {
		ap_autorun = 0;
		if (ap_autorun < 0)
			printf("AP autorun disabled!\n");
		goto exit;
	}
	if (!ap_ssid || !strlen(ap_ssid)) {
		wifimgr_warn("no AP config!\n");
		goto exit;
	}

	/* Get AP status */
	ret =
	    wifimgr_get_ctrl_ops_cbs(&wifimgr_autorun_cbs)->
	    get_status(iface_name);
	if (ret) {
		wifimgr_err("failed to get_status! %d\n", ret);
		goto exit;
	}
	sem_wait(&ap_sem);

	switch (ap_state) {
	case WIFIMGR_SM_AP_NODEV:
		/* Open AP */
		ret = wifimgr_get_ctrl_ops()->open(iface_name);
		if (ret) {
			wifimgr_err("failed to open! %d\n", ret);
			goto exit;
		}
	case WIFIMGR_SM_AP_READY:
		/* Start AP */
		ret = wifimgr_get_ctrl_ops()->start_ap();
		if (ret) {
			wifimgr_err("failed to start AP! %d\n", ret);
			goto exit;
		}
		wifimgr_info("done!\n");

		break;
	default:
		wifimgr_dbg("nothing else to do under %s!\n",
			    ap_sts2str(ap_state));
		goto exit;
	}

exit:
	/* Set timer for the next run */
	wifimgr_timer_start(ap_autorun_timerid, ap_autorun);
	/* Release control of STA */
	wifimgr_release_ctrl(iface_name);
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

	wifimgr_register_connection_notifier(wifimgr_autorun_notify_connect);
	wifimgr_register_disconnection_notifier(wifimgr_autorun_notify_disconnect);

	/* Set timer for the first run */
	ret = wifimgr_timer_start(sta_autorun_timerid, 1);
	if (ret < 0) {
		wifimgr_err("failed to start STA autorun!\n");
		wifimgr_unregister_connection_notifier(wifimgr_autorun_notify_connect);
		wifimgr_unregister_disconnection_notifier(wifimgr_autorun_notify_disconnect);
	}
#endif
#ifdef CONFIG_WIFIMGR_AP
	wifimgr_init_work(&ap_work, (void *)wifimgr_autorun_ap);
	ret = wifimgr_timer_init(&ap_work, wifimgr_timeout,
				 &ap_autorun_timerid);
	if (ret < 0)
		wifimgr_err("failed to init AP autorun!\n");

	wifimgr_register_new_station_notifier(wifimgr_autorun_notify_new_station);
	/* Set timer for the first run */
	ret = wifimgr_timer_start(ap_autorun_timerid, 1);
	if (ret < 0)
		wifimgr_err("failed to start AP autorun!\n");
#endif
	return ret;
}

/*SYS_INIT(wifimgr_autorun_init, APPLICATION, WIFIMGR_AUTORUN_PRIORITY);*/
#endif
