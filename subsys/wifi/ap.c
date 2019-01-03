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
	case WIFIMGR_CMD_BLOCK_STATION:
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
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	char *ssid = NULL;
	char *passphrase = NULL;

	/* Load config form non-volatile memory */
	wifimgr_config_load(conf, WIFIMGR_SETTING_AP_PATH);

	if (!memiszero(conf, sizeof(struct wifimgr_config))) {
		wifimgr_info("No AP Config found!\n");
	} else {
		wifimgr_info("AP Config\n");
		if (conf->autorun)
			wifimgr_info("Autorun:\t%d\n", conf->autorun);

		if (strlen(conf->ssid)) {
			ssid = conf->ssid;
			wifimgr_info("SSID:\t\t%s\n", ssid);
		}

		if (strlen(conf->passphrase)) {
			passphrase = conf->passphrase;
			wifimgr_info("Passphrase:\t%s\n", passphrase);
		}

		if (conf->channel)
			wifimgr_info("Channel:\t%d\n", conf->channel);

		if (conf->ch_width)
			wifimgr_info("Channel Width:\t%d\n", conf->ch_width);
	}

	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->get_ap_conf_cb)
		cbs->get_ap_conf_cb(ssid, passphrase, conf->band, conf->channel,
				    conf->ch_width, conf->autorun);

	return 0;
}

static int wifimgr_ap_set_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;

	if (!memiszero(conf, sizeof(struct wifimgr_config))) {
		wifimgr_info("Clearing STA Config ...\n");
		wifimgr_config_clear(conf, WIFIMGR_SETTING_AP_PATH);
		return 0;
	}

	wifimgr_info("Setting AP Config ...\n");

	if (conf->autorun)
		wifimgr_info("Autorun:\t%d\n", conf->autorun);

	if (strlen(conf->ssid))
		wifimgr_info("SSID:\t\t%s\n", conf->ssid);

	if (strlen(conf->passphrase))
		wifimgr_info("Passphrase:\t%s\n", conf->passphrase);

	if (conf->channel)
		wifimgr_info("Channel:\t%d\n", conf->channel);

	if (conf->ch_width)
		wifimgr_info("Channel Width:\t%d\n", conf->ch_width);

	fflush(stdout);

	/* Save config to non-volatile memory */
	wifimgr_config_save(conf, WIFIMGR_SETTING_AP_PATH);

	return 0;
}

static int wifimgr_ap_get_status(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sm = &mgr->ap_sm;
	struct wifimgr_status *sts = &mgr->ap_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();

	wifimgr_info("AP Status:\t%s\n", ap_sts2str(sm_ap_query(sm)));

	if (is_zero_ether_addr(sts->own_mac))
		if (wifi_drv_get_mac(mgr->ap_iface, sts->own_mac))
			wifimgr_warn("failed to get Own MAC!");
	wifimgr_info("Own MAC:\t%02x:%02x:%02x:%02x:%02x:%02x\n",
		     sts->own_mac[0], sts->own_mac[1], sts->own_mac[2],
		     sts->own_mac[3], sts->own_mac[4], sts->own_mac[5]);

	if (sm_ap_started(sm) == true) {
		int i;

		wifimgr_info("----------------\n");
		wifimgr_info("Stations:\t%d\n", sts->u.ap.sta_nr);

		if (sts->u.ap.sta_nr) {
			wifimgr_info("Station List:\n");
			for (i = 0; i < sts->u.ap.sta_nr; i++)
				wifimgr_info
				    ("\t\t%02x:%02x:%02x:%02x:%02x:%02x\n",
				     sts->u.ap.sta_mac_addrs[i][0],
				     sts->u.ap.sta_mac_addrs[i][1],
				     sts->u.ap.sta_mac_addrs[i][2],
				     sts->u.ap.sta_mac_addrs[i][3],
				     sts->u.ap.sta_mac_addrs[i][4],
				     sts->u.ap.sta_mac_addrs[i][5]);
		}
	}

	/* Notify the external caller */
	if (cbs && cbs->get_ap_status_cb)
		cbs->get_ap_status_cb(sm_ap_query(sm), sts->own_mac,
				      sts->u.ap.sta_nr,
				      sts->u.ap.sta_mac_addrs,
				      sts->u.ap.acl_nr,
				      sts->u.ap.acl_mac_addrs);
	fflush(stdout);

	return 0;
}

static int wifimgr_ap_block_station(void *handle)
{
	struct wifimgr_del_station *del_sta =
	    (struct wifimgr_del_station *)handle;
	struct wifi_manager *mgr =
	    container_of(del_sta, struct wifi_manager, del_sta);
	int ret;

	wifimgr_info("Deleting station (%02x:%02x:%02x:%02x:%02x:%02x)\n",
		     del_sta->mac[0], del_sta->mac[1], del_sta->mac[2],
		     del_sta->mac[3], del_sta->mac[4], del_sta->mac[5]);

	ret = wifi_drv_del_station(mgr->ap_iface, del_sta->mac);

	return ret;
}

static int wifimgr_ap_new_station_event_cb(void *arg)
{
	struct wifimgr_ap_event *ap_evt = (struct wifimgr_ap_event *)arg;
	struct wifi_drv_new_station_evt *new_sta = &ap_evt->u.new_sta;
	struct wifi_manager *mgr =
	    container_of(ap_evt, struct wifi_manager, ap_evt);
	struct wifimgr_status *sts = &mgr->ap_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	bool need_notify = false;

	wifimgr_info("station (%02x:%02x:%02x:%02x:%02x:%02x) %s!\n",
		     new_sta->mac[0], new_sta->mac[1], new_sta->mac[2],
		     new_sta->mac[3], new_sta->mac[4], new_sta->mac[5],
		     new_sta->is_connect ? "connected" : "disconnected");
	fflush(stdout);

	if (new_sta->is_connect) {
		if (!sts->u.ap.sta_nr)
			cmd_processor_add_sender(&mgr->prcs,
						 WIFIMGR_CMD_BLOCK_STATION,
						 wifimgr_ap_block_station,
						 &mgr->del_sta);

		if (sts->u.ap.sta_nr < WIFIMGR_MAX_STA_NR) {
			sts->u.ap.sta_nr++;
			memcpy(sts->u.ap.sta_mac_addrs + (sts->u.ap.sta_nr - 1),
			       new_sta->mac, WIFIMGR_ETH_ALEN);
			need_notify = true;
		}
	} else {
		if (sts->u.ap.sta_nr) {
			memset(sts->u.ap.sta_mac_addrs + (sts->u.ap.sta_nr - 1),
			       0x0, WIFIMGR_ETH_ALEN);
			sts->u.ap.sta_nr--;
			need_notify = true;
		}

		if (!sts->u.ap.sta_nr)
			cmd_processor_remove_sender(&mgr->prcs,
						    WIFIMGR_CMD_BLOCK_STATION);
	}

	/* Notify the external caller */
	if (cbs && cbs->notify_new_station && (need_notify == true))
		cbs->notify_new_station(new_sta->is_connect, new_sta->mac);

	return 0;
}

static int wifimgr_ap_start(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifi_drv_capa *capa = &mgr->ap_capa;
	struct wifimgr_config *conf = &mgr->ap_conf;
	struct wifimgr_status *sts = &mgr->ap_sts;
	char *ssid = NULL;
	char *passphrase = NULL;
	int size;
	int ret;

	if (!capa->max_ap_assoc_sta)
		capa->max_ap_assoc_sta = WIFIMGR_MAX_STA_NR;
	size = capa->max_ap_assoc_sta * WIFIMGR_ETH_ALEN;
	sts->u.ap.sta_mac_addrs = malloc(size);
	if (!sts->u.ap.sta_mac_addrs)
		return -ENOMEM;
	memset(sts->u.ap.sta_mac_addrs, 0, size);
	sts->u.ap.sta_nr = 0;

	if (!capa->max_acl_mac_addrs)
		capa->max_acl_mac_addrs = WIFIMGR_MAX_STA_NR;
	size = capa->max_acl_mac_addrs * WIFIMGR_ETH_ALEN;
	sts->u.ap.acl_mac_addrs = malloc(size);
	if (!sts->u.ap.acl_mac_addrs) {
		free(sts->u.ap.sta_mac_addrs);
		return -ENOMEM;
	}
	memset(sts->u.ap.acl_mac_addrs, 0, size);
	sts->u.ap.acl_nr = 0;

	ret = evt_listener_add_receiver(&mgr->lsnr,
					WIFIMGR_EVT_NEW_STATION, false,
					wifimgr_ap_new_station_event_cb,
					&mgr->ap_evt);
	if (ret)
		return ret;

	if (strlen(conf->ssid))
		ssid = conf->ssid;
	if (strlen(conf->passphrase))
		passphrase = conf->passphrase;
	ret = wifi_drv_start_ap(mgr->ap_iface, ssid, passphrase,
				conf->channel, conf->ch_width);

	if (ret) {
		wifimgr_err("failed to start AP! %d\n", ret);
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

	ret = wifi_drv_stop_ap(mgr->ap_iface);
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

	ret = wifi_drv_open(mgr->ap_iface);
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

	ret = wifi_drv_close(mgr->ap_iface);
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
