/*
 * @file
 * @brief SoftAP mode handling
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
#include "led.h"

static int wifimgr_ap_stop(void *handle);
static int wifimgr_ap_close(void *handle);

void wifimgr_ap_event_timeout(wifimgr_work *work)
{
	struct wifimgr_state_machine *ap_sm =
	    container_of(work, struct wifimgr_state_machine, work);
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	unsigned int expected_evt;

	/* Notify the external caller */
	switch (ap_sm->cur_cmd) {
	case WIFIMGR_CMD_DEL_STATION:
		expected_evt = WIFIMGR_EVT_NEW_STATION;
		wifimgr_err("[%s] timeout!\n", wifimgr_evt2str(expected_evt));

		if (cbs && cbs->notify_del_station_timeout)
			cbs->notify_del_station_timeout();
		break;
	default:
		break;
	}
}

static int wifimgr_ap_get_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;
	struct wifi_manager *mgr =
	    container_of(conf, struct wifi_manager, ap_conf);
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	char *ssid = NULL;
	char *bssid = NULL;
	char *passphrase = NULL;

	/* Load config form non-volatile memory */
	wifimgr_load_config(conf, WIFIMGR_SETTING_AP_PATH);

	if (!memiszero(conf, sizeof(struct wifimgr_config))) {
		wifimgr_info("No AP Config found!\n");
	} else {
		wifimgr_info("AP Config\n");
		if (strlen(conf->ssid)) {
			ssid = conf->ssid;
			wifimgr_info("SSID:\t\t%s\n", ssid);
		}

		if (is_zero_ether_addr(conf->bssid))
			wifi_drv_iface_get_mac(mgr->ap_iface, conf->bssid);
		if (!is_zero_ether_addr(conf->bssid)) {
			bssid = conf->bssid;
			wifimgr_info
			    ("BSSID:\t\t%02x:%02x:%02x:%02x:%02x:%02x\n",
			     bssid[0], bssid[1], bssid[2], bssid[3], bssid[4],
			     bssid[5]);
		}

		if (strlen(conf->passphrase)) {
			passphrase = conf->passphrase;
			wifimgr_info("Passphrase:\t%s\n", passphrase);
		}

		if (conf->channel)
			wifimgr_info("Channel:\t%d\n", conf->channel);

		if (conf->channel_width)
			wifimgr_info("Channel Width:\t%d\n",
				     conf->channel_width);
	}

	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->get_conf_cb)
		cbs->get_conf_cb(WIFIMGR_IFACE_NAME_AP, ssid, bssid,
				 passphrase, conf->band, conf->channel,
				 conf->channel_width);

	return 0;
}

static int wifimgr_ap_set_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;

	/* Save config to non-volatile memory */
	wifimgr_save_config(conf, WIFIMGR_SETTING_AP_PATH);

	if (!memiszero(conf, sizeof(struct wifimgr_config))) {
		wifimgr_info("Clearing STA Config ...\n");
		return 0;
	}

	wifimgr_info("Setting AP Config ...\n");

	if (strlen(conf->ssid))
		wifimgr_info("SSID:\t\t%s\n", conf->ssid);

	if (strlen(conf->passphrase))
		wifimgr_info("Passphrase:\t%s\n", conf->passphrase);

	if (conf->channel)
		wifimgr_info("Channel:\t%d\n", conf->channel);

	if (conf->channel_width)
		wifimgr_info("Channel Width:\t%d\n", conf->channel_width);

	fflush(stdout);

	return 0;
}

static int wifimgr_ap_get_status(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sm = &mgr->ap_sm;
	struct wifimgr_status *sts = &mgr->ap_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	int ret = 0;

	wifimgr_info("AP Status:\t%s\n", ap_sts2str(sm_ap_query(sm)));

	if (is_zero_ether_addr(sts->own_mac))
		ret = wifi_drv_iface_get_mac(mgr->ap_iface, sts->own_mac);
	wifimgr_info("Own MAC:\t%02x:%02x:%02x:%02x:%02x:%02x\n",
		     sts->own_mac[0], sts->own_mac[1], sts->own_mac[2],
		     sts->own_mac[3], sts->own_mac[4], sts->own_mac[5]);

	if (sm_ap_started(sm) == true) {
		int i;

		wifimgr_info("----------------\n");
		wifimgr_info("Client Number:\t%d\n", sts->u.ap.client_nr);

		if (sts->u.ap.client_nr) {
			wifimgr_info("Client List:\n");
			for (i = 0; i < sts->u.ap.client_nr; i++)
				wifimgr_info("\t\t%02x:%02x:%02x:%02x:%02x:%02x\n",
					     sts->u.ap.client_mac[i][0],
					     sts->u.ap.client_mac[i][1],
					     sts->u.ap.client_mac[i][2],
					     sts->u.ap.client_mac[i][3],
					     sts->u.ap.client_mac[i][4],
					     sts->u.ap.client_mac[i][5]);
		}
	}

	/* Notify the external caller */
	if (cbs && cbs->get_ap_status_cb)
		cbs->get_ap_status_cb(sm_ap_query(sm),
				      sts->own_mac, sts->u.ap.client_nr,
				      sts->u.ap.client_mac);
	fflush(stdout);

	return 0;
}

static int wifimgr_ap_del_station(void *handle)
{
	struct wifimgr_del_station *del_sta =
	    (struct wifimgr_del_station *)handle;
	struct wifi_manager *mgr =
	    container_of(del_sta, struct wifi_manager, del_sta);
	int ret;

	wifimgr_info("Deleting station (%02x:%02x:%02x:%02x:%02x:%02x)\n",
		     del_sta->mac[0], del_sta->mac[1], del_sta->mac[2],
		     del_sta->mac[3], del_sta->mac[4], del_sta->mac[5]);

	ret = wifi_drv_iface_del_station(mgr->ap_iface, del_sta->mac);

	return ret;
}

static int wifimgr_ap_new_station_event_cb(void *arg)
{
	struct wifimgr_evt_new_station *new_sta =
	    (struct wifimgr_evt_new_station *)arg;
	struct wifi_manager *mgr =
	    container_of(new_sta, struct wifi_manager, evt_new_sta);
	struct wifimgr_status *sts = &mgr->ap_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	bool need_notify = false;

	wifimgr_info("station (%02x:%02x:%02x:%02x:%02x:%02x) %s!\n",
		     new_sta->mac[0], new_sta->mac[1], new_sta->mac[2],
		     new_sta->mac[3], new_sta->mac[4], new_sta->mac[5],
		     new_sta->is_connect ? "connected" : "disconnected");
	fflush(stdout);

	if (new_sta->is_connect) {
		if (!sts->u.ap.client_nr)
			cmd_processor_add_sender(&mgr->prcs,
						 WIFIMGR_CMD_DEL_STATION,
						 wifimgr_ap_del_station,
						 &mgr->del_sta);

		if (sts->u.ap.client_nr < WIFIMGR_MAX_STA_NR) {
			sts->u.ap.client_nr++;
			memcpy(sts->u.ap.client_mac + (sts->u.ap.client_nr - 1),
			       new_sta->mac, WIFIMGR_ETH_ALEN);
			need_notify = true;
		}
	} else {
		if (sts->u.ap.client_nr) {
			memset(sts->u.ap.client_mac + (sts->u.ap.client_nr - 1),
			       0x0, WIFIMGR_ETH_ALEN);
			sts->u.ap.client_nr--;
			need_notify = true;
		}

		if (!sts->u.ap.client_nr)
			cmd_processor_remove_sender(&mgr->prcs,
						    WIFIMGR_CMD_DEL_STATION);
	}

	/* Notify the external caller */
	if (cbs && cbs->notify_new_station && (need_notify == true))
		cbs->notify_new_station(new_sta->is_connect, new_sta->mac);

	return 0;
}

static int wifimgr_ap_start(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *conf = &mgr->ap_conf;
	struct wifimgr_status *sts = &mgr->ap_sts;
	int ret;

	if (!strlen(conf->ssid)) {
		wifimgr_err("no AP config!\n");
		return -EINVAL;
	}

	memset(sts->u.ap.client_mac, 0x0,
	       WIFIMGR_MAX_STA_NR * WIFIMGR_ETH_ALEN);
	sts->u.ap.client_nr = 0;

	ret = evt_listener_add_receiver(&mgr->lsnr,
					WIFIMGR_EVT_NEW_STATION, false,
					wifimgr_ap_new_station_event_cb,
					&mgr->evt_new_sta);
	if (ret) {
		wifimgr_err("failed to start AP!\n");
		return ret;
	}

	ret = wifi_drv_iface_start_ap(mgr->ap_iface, conf->ssid,
				      conf->passphrase, conf->channel,
				      conf->channel_width);

	if (ret) {
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_NEW_STATION);
		return ret;
	}

	wifimgr_ap_led_on();

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP,
				 wifimgr_ap_stop, mgr);

	/* TODO: Start DHCP server */

	return ret;
}

static int wifimgr_ap_stop(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_stop_ap(mgr->ap_iface);
	if (ret) {
		wifimgr_err("failed to stop AP!\n");
		return ret;
	}

	wifimgr_ap_led_off();

	evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_NEW_STATION);

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_START_AP,
				 wifimgr_ap_start, mgr);

	return ret;
}

static int wifimgr_ap_open(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_open_softap(mgr->ap_iface);
	if (ret) {
		wifimgr_err("failed to open AP!\n");
		return ret;
	}

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_AP);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_AP,
				 wifimgr_ap_close, mgr);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_START_AP,
				 wifimgr_ap_start, mgr);

	return ret;
}

static int wifimgr_ap_close(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_close_softap(mgr->ap_iface);
	if (ret) {
		wifimgr_err("failed to close AP!\n");
		return ret;
	}

	evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_NEW_STATION);

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_AP);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_AP,
				 wifimgr_ap_open, mgr);

	return ret;
}

void wifimgr_ap_init(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct cmd_processor *prcs = &mgr->prcs;

	/* Register default AP commands */
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_SET_AP_CONFIG,
				 wifimgr_ap_set_config, &mgr->ap_conf);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_AP_CONFIG,
				 wifimgr_ap_get_config, &mgr->ap_conf);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_AP_STATUS,
				 wifimgr_ap_get_status, mgr);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_OPEN_AP,
				 wifimgr_ap_open, mgr);
}
