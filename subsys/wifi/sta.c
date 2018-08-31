/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifimgr.h"

#define RECONNECT_TIMEOUT   10	/* s */

int wifi_manager_get_sta_config(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *sta_conf = &mgr->sta_conf;

	if (sta_conf->ssid[0] != '\0') {
		syslog(LOG_INFO, "STA Config\n");
		syslog(LOG_INFO, "SSID:\t\t%s\n", sta_conf->ssid);
		if (sta_conf->passphrase[0] != '\0')
			syslog(LOG_INFO, "Passphrase:\t%s\n",
			       sta_conf->passphrase);

	}

	return 0;
}

int wifi_manager_set_sta_config(void *handle)
{
	struct wifimgr_config *sta_conf = (struct wifimgr_config *)handle;

	syslog(LOG_INFO, "Setting STA SSID to %s\n", sta_conf->ssid);

	if (sta_conf->passphrase[0] != '\0')
		syslog(LOG_INFO, "Setting STA Passphrase to %s\n",
		       sta_conf->passphrase);

	return 0;
}

int wifi_manager_get_station(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_get_station(mgr->sta_iface, &mgr->sta_sts.rssi);

	return ret;
}

static int wifi_manager_disconnect_event(void *arg)
{
	struct wifimgr_evt_disconnect *evt_disc =
	    (struct wifimgr_evt_disconnect *)arg;
	struct wifi_manager *mgr =
	    container_of(evt_disc, struct wifi_manager, evt_disc);
	/*struct netif *netif; */
	int ret = 0;

	syslog(LOG_INFO, "disconnect, reason_code %d!\n",
	       evt_disc->reason_code);
	fflush(stdout);

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_DISCONNECT);
/*
	netif = netif_find(WIFIMGR_STA_DEVNAME);
	if (netif) {
		syslog(LOG_INFO, "stop DHCP\n");
		dhcp_stop(netif);
	}
*/

	/*Clear the info of specified AP */
	if (!strcmp(mgr->sta_sts.ssid, mgr->sta_conf.ssid))
		if (!strncmp
		    (mgr->sta_sts.bssid, mgr->sta_conf.bssid, WIFIMGR_ETH_ALEN)
		    || is_zero_ether_addr(mgr->sta_conf.bssid))
			memset(&mgr->sta_sts, 0, sizeof(struct wifimgr_status));

	/* Notify the external caller */
	if (wifimgr_get_ctrl_cbs() && wifimgr_get_ctrl_cbs()->notify_disconnect)
		wifimgr_get_ctrl_cbs()->notify_disconnect(&evt_disc->
							  reason_code);

	return ret;
}

int wifi_manager_disconnect(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_DISCONNECT,
				    true, wifi_manager_disconnect_event,
				    &mgr->evt_disc);

	ret = wifi_drv_iface_disconnect(mgr->sta_iface);
	if (ret)
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_DISCONNECT);

	return ret;
}

static int wifi_manager_connect_event(void *arg)
{
	struct wifimgr_evt_connect *evt_conn =
	    (struct wifimgr_evt_connect *)arg;
	struct wifi_manager *mgr =
	    container_of(evt_conn, struct wifi_manager, evt_conn);
	/*struct netif *netif; */
	int ret = evt_conn->status;

	if (!ret) {
		syslog(LOG_INFO, "connect successfully!\n");

		/* Register disconnect event here due to AP deauth */
		event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_DISCONNECT,
					    true,
					    wifi_manager_disconnect_event,
					    &mgr->evt_disc);

		command_processor_register_sender(&mgr->prcs,
						  WIFIMGR_CMD_DISCONNECT,
						  wifi_manager_disconnect, mgr);
/*
		netif = netif_find(WIFIMGR_STA_DEVNAME);
		if (netif) {
			syslog(LOG_INFO, "start DHCP client\n");
			dhcp_start(netif);
		}
*/

		/* Notify the external caller */
		if (wifimgr_get_ctrl_cbs()
		    && wifimgr_get_ctrl_cbs()->notify_connect)
			wifimgr_get_ctrl_cbs()->notify_connect(&evt_conn->
							       status);
	} else {
		syslog(LOG_ERR, "failed to connect!\n");
	}
	fflush(stdout);

	return ret;
}

int wifi_manager_connect(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *sta_conf = &mgr->sta_conf;
	int ret;

	if (sta_conf->ssid[0] == '\0') {
		syslog(LOG_ERR, "no AP specified!\n");
		return -EINVAL;
	}

	syslog(LOG_INFO, "Connecting to %s\n", sta_conf->ssid);

	ret =
	    event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_CONNECT,
					true,
					wifi_manager_connect_event,
					&mgr->evt_conn);
	if (ret)
		return ret;

	ret =
	    wifi_drv_iface_connect(mgr->sta_iface, sta_conf->ssid,
				   sta_conf->passphrase);

	if (ret)
		event_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_CONNECT);

	return ret;
}

static int wifi_manager_scan_result(void *arg)
{
	struct wifimgr_evt_scan_result *evt_scan_res =
	    (struct wifimgr_evt_scan_result *)arg;
	int ret = 0;
	struct wifi_manager *mgr =
	    container_of(evt_scan_res, struct wifi_manager, evt_scan_res);

	syslog(LOG_INFO, "\t%-32s", evt_scan_res->ssid);
	syslog(LOG_INFO, "\t%02x:%02x:%02x:%02x:%02x:%02x\t%u\t%d\n",
	       evt_scan_res->bssid[0], evt_scan_res->bssid[1],
	       evt_scan_res->bssid[2], evt_scan_res->bssid[3],
	       evt_scan_res->bssid[4], evt_scan_res->bssid[5],
	       evt_scan_res->channel, evt_scan_res->rssi);
	fflush(stdout);

	/* Record the info of specified AP */
	if (!strcmp(evt_scan_res->ssid, mgr->sta_conf.ssid)) {
		if (!strncmp
		    (evt_scan_res->bssid, mgr->sta_conf.bssid, WIFIMGR_ETH_ALEN)
		    || is_zero_ether_addr(mgr->sta_conf.bssid)) {
			strcpy(mgr->sta_sts.ssid, evt_scan_res->ssid);
			strncpy(mgr->sta_sts.bssid, evt_scan_res->bssid,
				WIFIMGR_ETH_ALEN);
			mgr->sta_sts.band = evt_scan_res->band;
			mgr->sta_sts.channel = evt_scan_res->channel;
			mgr->sta_sts.rssi = evt_scan_res->rssi;
		}
	}

	/* Notify the external caller */
	if (wifimgr_get_ctrl_cbs() && wifimgr_get_ctrl_cbs()->notify_scan_res)
		wifimgr_get_ctrl_cbs()->notify_scan_res(evt_scan_res->ssid,
							evt_scan_res->bssid,
							evt_scan_res->band,
							evt_scan_res->channel,
							evt_scan_res->rssi);

	return ret;
}

static int wifi_manager_scan_done(void *arg)
{
	struct wifimgr_evt_scan_done *evt_scan_done =
	    (struct wifimgr_evt_scan_done *)arg;
	struct wifi_manager *mgr =
	    container_of(evt_scan_done, struct wifi_manager, evt_scan_done);
	int ret = evt_scan_done->result;

	event_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_RESULT);

	if (!ret)
		syslog(LOG_INFO, "scan done!\n");
	else
		syslog(LOG_ERR, "scan abort!\n");
	fflush(stdout);

	return ret;
}

int wifi_manager_scan(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret =
	    event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_RESULT,
					false, wifi_manager_scan_result,
					&mgr->evt_scan_res);
	if (ret)
		return ret;

	ret =
	    event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_DONE,
					true, wifi_manager_scan_done,
					&mgr->evt_scan_done);
	if (ret) {
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_SCAN_RESULT);
		return ret;
	}

	ret = wifi_drv_iface_scan(mgr->sta_iface);
	if (ret) {
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_SCAN_RESULT);
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_SCAN_DONE);
	}

	return ret;
}

int wifi_manager_open_station(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_open_station(mgr->sta_iface);
	if (ret) {
		syslog(LOG_ERR, "failed to open STA!\n");
		return ret;
	}

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_STA);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_STA,
					  wifi_manager_close_station, mgr);
	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_SCAN,
					  wifi_manager_scan, mgr);
	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_CONNECT,
					  wifi_manager_connect, mgr);

	return ret;
}

int wifi_manager_close_station(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_close_station(mgr->sta_iface);
	if (ret) {
		syslog(LOG_ERR, "failed to close STA!\n");
		return ret;
	}

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_STA);
	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_DISCONNECT);
	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_CONNECT);
	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_SCAN);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_STA,
					  wifi_manager_close_station, mgr);

	return ret;
}
