/*
 * @file
 * @brief autorun of WiFi manager, including both STA and AP.
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <net/wifimgr_api.h>

#include "os_adapter.h"
#include "sm.h"

#define WIFIMGR_AUTORUN_PRIORITY	91

#define WIFIMGR_AUTORUN_STACKSIZE	(4096)
#define WIFIMGR_AUTORUN_STA		"wifimgr_autorun_sta"
#define WIFIMGR_AUTORUN_STA_PRIORITY	(1)
#define WIFIMGR_AUTORUN_AP		"wifimgr_autorun_ap"
#define WIFIMGR_AUTORUN_AP_PRIORITY	(1)

#define WIFIMGR_AUTORUN_STA_RETRY	(3)

K_THREAD_STACK_ARRAY_DEFINE(sta_stacks, 1, WIFIMGR_AUTORUN_STACKSIZE);
K_THREAD_STACK_ARRAY_DEFINE(ap_stacks, 1, WIFIMGR_AUTORUN_STACKSIZE);

static sem_t sta_sem;
static char sta_autorun;
static char sta_state;
static bool host_found;
static char sta_connected;

static sem_t ap_sem;
static char ap_autorun;
static char ap_state;

static
void wifimgr_autorun_get_sta_conf_cb(char *ssid, char *bssid, char *passphrase,
				     unsigned char band, unsigned char channel,
				     enum wifimgr_security security,
				     char autorun)
{
	sta_autorun = autorun;
	sem_post(&sta_sem);
}

static
void wifimgr_autorun_get_ap_conf_cb(char *ssid, char *passphrase,
				    unsigned char band, unsigned char channel,
				    unsigned char ch_width,
				    enum wifimgr_security security,
				    char autorun)
{
	ap_autorun = autorun;
	sem_post(&ap_sem);
}

static
void wifimgr_autorun_get_sta_status_cb(char status, char *own_mac,
				       signed char host_rssi)
{
	sta_state = status;
	sem_post(&sta_sem);
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

static struct wifimgr_ctrl_cbs wifimgr_autorun_cbs = {
	.get_sta_conf_cb = wifimgr_autorun_get_sta_conf_cb,
	.get_ap_conf_cb = wifimgr_autorun_get_ap_conf_cb,
	.get_sta_status_cb = wifimgr_autorun_get_sta_status_cb,
	.get_ap_status_cb = wifimgr_autorun_get_ap_status_cb,
	.notify_scan_done = wifimgr_autorun_notify_scan_done,
	.notify_connect = wifimgr_autorun_notify_connect,
	.notify_scan_timeout = wifimgr_autorun_notify_scan_timeout,
	.notify_connect_timeout = wifimgr_autorun_notify_connect_timeout,
};

static void *wifimgr_autorun_sta(void *handle)
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
		goto exit;
	}

	/* Get STA config */
	ret = wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_conf(iface_name);
	if (ret) {
		printf("%s: failed to get_conf! %d\n", __func__, ret);
		goto exit;
	}
	sem_wait(&sta_sem);
	if (!sta_autorun) {
		printf("%s: autorun disabled!\n", __func__);
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
		    wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->
		    open(iface_name);
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
			printf("%s: maximum retry of scan reached!\n",
			       __func__);
			goto exit;
		}

		/* Connect the AP */
		for (cnt = 0; cnt < WIFIMGR_AUTORUN_STA_RETRY; cnt++) {
			ret =
			    wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->
			    connect();
			if (ret)
				printf("%s: failed to connect! %d\n", __func__,
				       ret);
			else
				sem_wait(&sta_sem);
			if (sta_connected)
				break;
		}
		if (cnt == WIFIMGR_AUTORUN_STA_RETRY) {
			printf("%s: maximum retry of connect reached!\n",
			       __func__);
			goto exit;
		}

		break;
	default:
		printf("%s: nothing else to do under %s!\n", __func__,
		       sta_sts2str(sta_state));
		goto exit;
	}

exit:
	pthread_exit(handle);
	return NULL;
}

static void *wifimgr_autorun_ap(void *handle)
{
	char *iface_name = WIFIMGR_IFACE_NAME_AP;
	int ret;

	/* Check AP ops */
	if (!wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_status
	    || !wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->open
	    || !wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_conf
	    || !wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->start_ap) {
		printf("%s: incomplete ops!\n", __func__);
		goto exit;
	}

	/* Get AP config */
	ret = wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->get_conf(iface_name);
	if (ret) {
		printf("%s: failed to get_conf! %d\n", __func__, ret);
		goto exit;
	}
	sem_wait(&ap_sem);
	if (!ap_autorun) {
		printf("%s: autorun disabled!\n", __func__);
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
		    wifimgr_get_ctrl_ops(&wifimgr_autorun_cbs)->
		    open(iface_name);
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
	pthread_exit(handle);
	return NULL;
}

static int wifimgr_autorun_init(struct device *unused)
{
	struct sched_param sparam;
	pthread_attr_t tattr;
	pthread_t pid;
	void *retval;
	int ret;

	sem_init(&sta_sem, 0, 0);
	pthread_attr_init(&tattr);
	sparam.priority = WIFIMGR_AUTORUN_STA_PRIORITY;
	pthread_attr_setschedparam(&tattr, &sparam);
	pthread_attr_setstack(&tattr, &sta_stacks[0][0],
			      WIFIMGR_AUTORUN_STACKSIZE);
	pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);

	ret = pthread_create(&pid, &tattr, wifimgr_autorun_sta, NULL);
	if (ret) {
		printf("failed to start %s!\n", WIFIMGR_AUTORUN_STA);
		return ret;
	}
	pthread_join(pid, &retval);

	sem_init(&ap_sem, 0, 0);
	pthread_attr_init(&tattr);
	sparam.priority = WIFIMGR_AUTORUN_AP_PRIORITY;
	pthread_attr_setschedparam(&tattr, &sparam);
	pthread_attr_setstack(&tattr, &ap_stacks[0][0],
			      WIFIMGR_AUTORUN_STACKSIZE);
	pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);

	ret = pthread_create(&pid, &tattr, wifimgr_autorun_ap, NULL);
	if (ret) {
		printf("failed to start %s!\n", WIFIMGR_AUTORUN_AP);
		return ret;
	}

	return 0;
}

SYS_INIT(wifimgr_autorun_init, APPLICATION, WIFIMGR_AUTORUN_PRIORITY);
