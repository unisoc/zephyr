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

#include <init.h>
#include <net/wifimgr_api.h>

#include "os_adapter.h"
#include "sm.h"
#include "timer.h"

#define WIFIMGR_AUTORUN_PRIORITY       91

#ifdef CONFIG_WIFIMGR_STA
#define WIFIMGR_AUTORUN_STA_RETRY	(3)

static sem_t sta_sem;
wifimgr_work sta_work;
timer_t sta_autorun_timerid;
static int sta_autorun;
static char sta_state;
static char *host_ssid;
static bool host_found;
static char sta_connected;
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

static void wifimgr_autorun_notify_connect(char result)
{
	if (!result)
		sta_connected = 1;
	sem_post(&sta_sem);
}

static void wifimgr_autorun_notify_scan_timeout(void)
{
	host_found = 0;
	sem_post(&sta_sem);
}

static void wifimgr_autorun_notify_connect_timeout(void)
{
	sta_connected = 0;
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
#endif

static struct wifimgr_ctrl_cbs wifimgr_autorun_cbs = {
#ifdef CONFIG_WIFIMGR_STA
	.get_sta_conf_cb = wifimgr_autorun_get_sta_conf_cb,
	.get_sta_status_cb = wifimgr_autorun_get_sta_status_cb,
	.notify_scan_done = wifimgr_autorun_notify_scan_done,
	.notify_connect = wifimgr_autorun_notify_connect,
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
	if (!wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_status
	    || !wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->open
	    || !wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_conf
	    || !wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->scan
	    || !wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->connect) {
		printf("%s: incomplete ops!\n", __func__);
		return;
	}

	/* Get STA config */
	ret = wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_conf(iface_name);
	if (ret) {
		printf("%s: failed to get_conf! %d\n", __func__, ret);
		goto exit;
	}
	sem_wait(&sta_sem);
	if (sta_autorun <= 0) {
		printf("%s: autorun disabled!\n", __func__);
		sta_autorun = 0;
		goto exit;
	}
	if (!host_ssid || !strlen(host_ssid)) {
		printf("%s: no STA config!\n", __func__);
		goto exit;
	}

	/* Get STA status */
	ret =
	    wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_status(iface_name);
	if (ret) {
		printf("%s: failed to get_status! %d\n", __func__, ret);
		goto exit;
	}
	sem_wait(&sta_sem);

	switch (sta_state) {
	case WIFIMGR_SM_STA_NODEV:
		/* Open STA */
		ret =
		    wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->open
		    (iface_name);
		if (ret) {
			printf("%s: failed to open! %d\n", __func__, ret);
			goto exit;
		}
	case WIFIMGR_SM_STA_READY:
		/* Trigger STA scan */
		for (cnt = 0; cnt < WIFIMGR_AUTORUN_STA_RETRY; cnt++) {
			ret =
			    wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->scan();
			if (ret)
				printf("%s: failed to scan! %d\n", __func__,
				       ret);
			else
				sem_wait(&sta_sem);
			if (host_found)
				break;
		}
		if (cnt == WIFIMGR_AUTORUN_STA_RETRY) {
			printf("%s: maximum scan retry reached!\n", __func__);
			goto exit;
		}

		/* Connect the AP */
		ret = wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->connect();
		if (ret) {
			printf("%s: failed to connect! %d\n", __func__, ret);
			goto exit;
		}
		sem_wait(&sta_sem);
		if (sta_connected)
			printf("%s: done!\n", __func__);

		break;
	default:
		printf("%s: nothing else to do under %s!\n", __func__,
		       sta_sts2str(sta_state));
		goto exit;
	}
exit:
	wifimgr_timer_start(sta_autorun_timerid, sta_autorun);
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static void wifimgr_autorun_ap(wifimgr_work *work)
{
	char *iface_name = WIFIMGR_IFACE_NAME_AP;
	int ret;

	/* Check AP ops */
	if (!wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_status
	    || !wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->open
	    || !wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_conf
	    || !wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->start_ap) {
		printf("%s: incomplete ops!\n", __func__);
		return;
	}

	/* Get AP config */
	ret = wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_conf(iface_name);
	if (ret) {
		printf("%s: failed to get_conf! %d\n", __func__, ret);
		goto exit;
	}
	sem_wait(&ap_sem);
	if (ap_autorun <= 0) {
		printf("%s: autorun disabled!\n", __func__);
		ap_autorun = 0;
		goto exit;
	}
	if (!ap_ssid || !strlen(ap_ssid)) {
		printf("%s: no AP config!\n", __func__);
		goto exit;
	}

	/* Get AP status */
	ret =
	    wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_status(iface_name);
	if (ret) {
		printf("%s: failed to get_status! %d\n", __func__, ret);
		goto exit;
	}
	sem_wait(&ap_sem);

	switch (ap_state) {
	case WIFIMGR_SM_AP_NODEV:
		/* Open AP */
		ret =
		    wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->open
		    (iface_name);
		if (ret) {
			printf("%s: failed to open! %d\n", __func__, ret);
			goto exit;
		}
	case WIFIMGR_SM_AP_READY:
		/* Start AP */
		ret = wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->start_ap();
		if (ret) {
			printf("%s: failed to start AP! %d\n", __func__, ret);
			goto exit;
		}

		break;
	default:
		printf("%s: nothing else to do under %s!\n", __func__,
		       ap_sts2str(ap_state));
		goto exit;
	}

exit:
	wifimgr_timer_start(ap_autorun_timerid, ap_autorun);
}
#endif

static int wifimgr_autorun_init(struct device *unused)
{
	int ret;
#ifdef CONFIG_WIFIMGR_STA
	sem_init(&sta_sem, 0, 0);
	wifimgr_init_work(&sta_work, (void *)wifimgr_autorun_sta);
	ret =
	    wifimgr_timer_init(&sta_work, wifimgr_timeout,
			       &sta_autorun_timerid);
	if (ret < 0)
		printf("failed to init STA autorun!\n");
	ret = wifimgr_timer_start(sta_autorun_timerid, 1);
	if (ret < 0)
		printf("failed to start STA autorun!\n");
#endif
#ifdef CONFIG_WIFIMGR_AP
	sem_init(&ap_sem, 0, 0);
	wifimgr_init_work(&ap_work, (void *)wifimgr_autorun_ap);
	ret =
	    wifimgr_timer_init(&ap_work, wifimgr_timeout, &ap_autorun_timerid);
	if (ret < 0)
		printf("failed to init AP autorun!\n");
	ret = wifimgr_timer_start(ap_autorun_timerid, 1);
	if (ret < 0)
		printf("failed to start AP autorun!\n");
#endif
	return ret;
}

SYS_INIT(wifimgr_autorun_init, APPLICATION, WIFIMGR_AUTORUN_PRIORITY);
#endif
