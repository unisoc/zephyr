/*
 * @file
 * @brief SoftAP mode handling
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifimgr.h"
#include "led.h"

int wifi_manager_get_ap_config(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *conf = &mgr->ap_conf;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	int ret = 0;

	syslog(LOG_INFO, "AP Config\n");
	if (strlen(conf->ssid))
		syslog(LOG_INFO, "SSID:\t\t%s\n", conf->ssid);

	if (is_zero_ether_addr(conf->bssid))
		ret = wifi_drv_iface_get_mac(mgr->ap_iface, conf->bssid);
	syslog(LOG_INFO, "BSSID:\t\t%02x:%02x:%02x:%02x:%02x:%02x\n",
	       conf->bssid[0], conf->bssid[1], conf->bssid[2],
	       conf->bssid[3], conf->bssid[4], conf->bssid[5]);

	if (conf->channel)
		syslog(LOG_INFO, "Channel:\t%d\n", conf->channel);

	if (strlen(conf->passphrase))
		syslog(LOG_INFO, "Passphrase:\t%s\n", conf->passphrase);
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

int wifi_manager_set_ap_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;

	syslog(LOG_INFO, "Setting AP ...\n");
	syslog(LOG_INFO, "SSID:\t\t%s\n", conf->ssid);
	syslog(LOG_INFO, "Channel:\t%d\n", conf->channel);

	if (strlen(conf->passphrase))
		syslog(LOG_INFO, "Passphrase:\t%s\n", conf->passphrase);
	fflush(stdout);

	return 0;
}

int wifi_manager_get_ap_status(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sm = &mgr->ap_sm;
	struct wifimgr_status *sts = &mgr->ap_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	int ret = 0;

	syslog(LOG_INFO, "AP Status:\t%s\n", ap_sts2str(sm_ap_query(sm)));

	if (is_zero_ether_addr(sts->own_mac))
		ret = wifi_drv_iface_get_mac(mgr->ap_iface, sts->own_mac);
	syslog(LOG_INFO, "Own MAC:\t%02x:%02x:%02x:%02x:%02x:%02x\n",
	       sts->own_mac[0], sts->own_mac[1], sts->own_mac[2],
	       sts->own_mac[3], sts->own_mac[4], sts->own_mac[5]);

	/* Notify the external caller */
	if (cbs && cbs->get_status_cb)
		cbs->get_status_cb(WIFIMGR_IFACE_NAME_AP, sm_ap_query(sm),
				   sts->own_mac, 0);
	fflush(stdout);

	return 0;
}

static int wifi_manager_new_station_event_cb(void *arg)
{
	struct wifimgr_evt_new_station *new_sta =
	    (struct wifimgr_evt_new_station *)arg;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();

	syslog(LOG_INFO,
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

int wifi_manager_start_softap(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *conf = &mgr->ap_conf;
	int ret;

	if (!strlen(conf->ssid)) {
		syslog(LOG_ERR, "no AP config!\n");
		return -EINVAL;
	}

	ret = event_listener_add_receiver(&mgr->lsnr,
					  WIFIMGR_EVT_NEW_STATION, false,
					  wifi_manager_new_station_event_cb,
					  &mgr->evt_new_sta);
	if (ret) {
		syslog(LOG_ERR, "failed to start AP!\n");
		return ret;
	}

	ret = wifi_drv_iface_start_softap(mgr->ap_iface, conf->ssid,
					  conf->passphrase, conf->channel);

	if (ret) {
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_NEW_STATION);
		return ret;
	}

	light_turn_on(LED1_GPIO_PIN);

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP,
					  wifi_manager_stop_softap, mgr);

	/* TODO: Start DHCP server */

	return ret;
}

int wifi_manager_stop_softap(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_stop_softap(mgr->ap_iface);
	if (ret) {
		syslog(LOG_ERR, "failed to stop AP!\n");
		return ret;
	}

	light_turn_off(LED1_GPIO_PIN);

	event_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_NEW_STATION);

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP);
	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_START_AP,
					  wifi_manager_start_softap, mgr);

	return ret;
}

int wifi_manager_open_softap(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_open_softap(mgr->ap_iface);
	if (ret) {
		syslog(LOG_ERR, "failed to open AP!\n");
		return ret;
	}

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_AP);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_AP,
					  wifi_manager_close_softap, mgr);
	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_START_AP,
					  wifi_manager_start_softap, mgr);

	return ret;
}

int wifi_manager_close_softap(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_close_softap(mgr->ap_iface);
	if (ret) {
		syslog(LOG_ERR, "failed to close AP!\n");
		return ret;
	}

	event_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_NEW_STATION);

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP);
	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);
	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_AP);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_AP,
					  wifi_manager_open_softap, mgr);

	return ret;
}
