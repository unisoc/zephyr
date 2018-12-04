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
		wifimgr_err("[%s] timeout!\n",
		       wifimgr_evt2str(expected_evt));

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
	int ret = 0;

	wifimgr_load_config(conf, WIFIMGR_SETTING_AP_PATH);

	wifimgr_info("AP Config\n");
	if (strlen(conf->ssid))
		wifimgr_info("SSID:\t\t%s\n", conf->ssid);

	if (is_zero_ether_addr(conf->bssid))
		ret = wifi_drv_iface_get_mac(mgr->ap_iface, conf->bssid);
	if (!is_zero_ether_addr(conf->bssid))
		wifimgr_info("BSSID:\t\t%02x:%02x:%02x:%02x:%02x:%02x\n",
		       conf->bssid[0], conf->bssid[1], conf->bssid[2],
		       conf->bssid[3], conf->bssid[4], conf->bssid[5]);

	if (conf->channel)
		wifimgr_info("Channel:\t%d\n", conf->channel);

	if (strlen(conf->passphrase))
		wifimgr_info("Passphrase:\t%s\n", conf->passphrase);
	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->get_conf_cb)
		cbs->get_conf_cb(WIFIMGR_IFACE_NAME_AP,
				 strlen(conf->ssid) ? conf->ssid : NULL,
				 is_zero_ether_addr(conf->bssid) ? NULL :
				 conf->bssid,
				 strlen(conf->passphrase) ? conf->
				 passphrase : NULL, conf->band, conf->channel);

	return ret;
}

static int wifimgr_ap_set_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;

	wifimgr_info("Setting AP Config ...\n");
	wifimgr_info("SSID:\t\t%s\n", conf->ssid);
	wifimgr_info("Channel:\t%d\n", conf->channel);

	if (strlen(conf->passphrase))
		wifimgr_info("Passphrase:\t%s\n", conf->passphrase);
	fflush(stdout);

	wifimgr_save_config(conf, WIFIMGR_SETTING_AP_PATH);

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

	/* Notify the external caller */
	if (cbs && cbs->get_status_cb)
		cbs->get_status_cb(WIFIMGR_IFACE_NAME_AP, sm_ap_query(sm),
				   sts->own_mac, 0);
	fflush(stdout);

	return 0;
}

static int wifimgr_ap_new_station_event_cb(void *arg)
{
	struct wifimgr_evt_new_station *new_sta =
	    (struct wifimgr_evt_new_station *)arg;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();

	wifimgr_info(
	       "station (%02x:%02x:%02x:%02x:%02x:%02x) %s!\n",
	       new_sta->mac[0], new_sta->mac[1], new_sta->mac[2],
	       new_sta->mac[3], new_sta->mac[4], new_sta->mac[5],
	       new_sta->is_connect ? "connected" : "disconnected");
	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->notify_new_station)
		cbs->notify_new_station(new_sta->is_connect, new_sta->mac);

	return 0;
}

static int wifimgr_ap_start(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *conf = &mgr->ap_conf;
	int ret;

	if (!strlen(conf->ssid)) {
		wifimgr_err("no AP config!\n");
		return -EINVAL;
	}

	ret = event_listener_add_receiver(&mgr->lsnr,
					  WIFIMGR_EVT_NEW_STATION, false,
					  wifimgr_ap_new_station_event_cb,
					  &mgr->evt_new_sta);
	if (ret) {
		wifimgr_err("failed to start AP!\n");
		return ret;
	}

	ret = wifi_drv_iface_start_ap(mgr->ap_iface, conf->ssid,
					  conf->passphrase, conf->channel);

	if (ret) {
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_NEW_STATION);
		return ret;
	}

	wifimgr_ap_led_on();

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP,
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

	event_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_NEW_STATION);

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP);
	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_START_AP,
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

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_AP);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_AP,
					  wifimgr_ap_close, mgr);
	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_START_AP,
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

	event_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_NEW_STATION);

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP);
	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);
	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_AP);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_AP,
					  wifimgr_ap_open, mgr);

	return ret;
}

void wifimgr_ap_init(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct cmd_processor *prcs = &mgr->prcs;

	/* Register default AP commands */
	command_processor_register_sender(prcs, WIFIMGR_CMD_SET_AP_CONFIG,
					  wifimgr_ap_set_config,
					  &mgr->ap_conf);
	command_processor_register_sender(prcs, WIFIMGR_CMD_GET_AP_CONFIG,
					  wifimgr_ap_get_config,
					  &mgr->ap_conf);
	command_processor_register_sender(prcs, WIFIMGR_CMD_GET_AP_STATUS,
					  wifimgr_ap_get_status, mgr);
	command_processor_register_sender(prcs, WIFIMGR_CMD_OPEN_AP,
					  wifimgr_ap_open, mgr);
}
