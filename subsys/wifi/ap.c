/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifimgr.h"

int wifi_manager_get_ap_config(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *ap_conf = &mgr->ap_conf;

	if (ap_conf->ssid[0] != '\0') {
		syslog(LOG_INFO, "AP Config\n");
		syslog(LOG_INFO, "SSID:\t\t%s\n", ap_conf->ssid);
		if (ap_conf->passphrase[0] != '\0')
			syslog(LOG_INFO, "Passphrase:\t%s\n",
			       ap_conf->passphrase);
		syslog(LOG_INFO, "Channel:\t%d\n", ap_conf->channel);

	}

	return 0;
}

int wifi_manager_set_ap_config(void *handle)
{
	struct wifimgr_config *ap_conf = (struct wifimgr_config *)handle;

	syslog(LOG_INFO, "Setting AP SSID to %s\n", ap_conf->ssid);

	if (ap_conf->passphrase[0] != '\0')
		syslog(LOG_INFO, "Setting AP Passphrase to %s\n",
		       ap_conf->passphrase);

	syslog(LOG_INFO, "Channel:\t\t%d\n", ap_conf->channel);

	return 0;
}

static int wifi_manager_new_station_event_cb(void *arg)
{
	struct wifimgr_evt_new_station *evt_new_sta =
	    (struct wifimgr_evt_new_station *)arg;

	if (evt_new_sta->is_connect)
		syslog(LOG_INFO,
		       "new station (%02x:%02x:%02x:%02x:%02x:%02x) connected!\n",
		       evt_new_sta->mac[0], evt_new_sta->mac[1],
		       evt_new_sta->mac[2], evt_new_sta->mac[3],
		       evt_new_sta->mac[4], evt_new_sta->mac[5]);
	else
		syslog(LOG_ERR,
		       "station (%02x:%02x:%02x:%02x:%02x:%02x) disconnected!\n",
		       evt_new_sta->mac[0], evt_new_sta->mac[1],
		       evt_new_sta->mac[2], evt_new_sta->mac[3],
		       evt_new_sta->mac[4], evt_new_sta->mac[5]);

	fflush(stdout);

	/* Notify the external caller */
	if (wifimgr_get_ctrl_cbs()
	    && wifimgr_get_ctrl_cbs()->notify_new_station)
		wifimgr_get_ctrl_cbs()->notify_new_station(evt_new_sta->
							   is_connect,
							   evt_new_sta->mac);

	return 0;
}

int wifi_manager_start_softap(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *ap_conf = &mgr->ap_conf;
	/*struct netif *netif; */
	int ret;

	if (ap_conf->ssid[0] == '\0') {
		syslog(LOG_ERR, "no AP config!\n");
		return -EINVAL;
	}

	ret =
	    event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_NEW_STATION,
					false,
					wifi_manager_new_station_event_cb,
					&mgr->evt_new_sta);
	if (ret) {
		syslog(LOG_ERR, "failed to start AP!\n");
		return ret;
	}

	ret =
	    wifi_drv_iface_start_softap(mgr->ap_iface, ap_conf->ssid,
					ap_conf->passphrase, ap_conf->channel);

	if (ret) {
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_NEW_STATION);
		return ret;
	}

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP,
					  wifi_manager_stop_softap, mgr);
/*
	netif = netif_find(WIFIMGR_AP_DEVNAME);
	if (netif) {
		syslog(LOG_INFO, "start DHCP server\n");
		dhcpd_start();
	}
*/
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
