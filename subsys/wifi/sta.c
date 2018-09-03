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

	/* Notify the external caller */
	if (wifimgr_get_ctrl_cbs() && wifimgr_get_ctrl_cbs()->get_conf_cb)
		wifimgr_get_ctrl_cbs()->get_conf_cb(WIFIMGR_IFACE_NAME_STA,
						    sta_conf->ssid,
						    sta_conf->bssid,
						    sta_conf->passphrase,
						    sta_conf->band,
						    sta_conf->channel);

	return 0;
}

int wifi_manager_set_sta_config(void *handle)
{
	struct wifimgr_config *sta_conf = (struct wifimgr_config *)handle;

	syslog(LOG_INFO, "Setting STA ...\n");
	syslog(LOG_INFO, "SSID:\t\t%s\n", sta_conf->ssid);

	if (sta_conf->passphrase[0] != '\0')
		syslog(LOG_INFO, "Passphrase:\t%s\n", sta_conf->passphrase);

	sta_conf->found = false;

	return 0;
}

int wifi_manager_get_sta_status(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;
	struct wifimgr_status *sta_sts = &mgr->sta_sts;
	int ret = 0;

	syslog(LOG_INFO, "STA Status:\t%s\n",
	       sta_sts2str(sm_sta_query(sta_sm)));

	if (is_zero_ether_addr(sta_sts->own_mac))
		ret = wifi_drv_iface_get_mac(mgr->sta_iface, sta_sts->own_mac);
	syslog(LOG_INFO, "Own MAC:\t%02x:%02x:%02x:%02x:%02x:%02x\n",
	       sta_sts->own_mac[0], sta_sts->own_mac[1],
	       sta_sts->own_mac[2], sta_sts->own_mac[3],
	       sta_sts->own_mac[4], sta_sts->own_mac[5]);

	if (sm_sta_connected(sta_sm) == true) {
		ret = wifi_drv_iface_get_station(mgr->sta_iface,
						 &sta_sts->rssi);
		syslog(LOG_INFO, "Signal:\t\t%d\n", sta_sts->rssi);
	}

	/* Notify the external caller */
	if (wifimgr_get_ctrl_cbs() && wifimgr_get_ctrl_cbs()->get_status_cb)
		wifimgr_get_ctrl_cbs()->get_status_cb(WIFIMGR_IFACE_NAME_STA,
						      sm_sta_query(sta_sm),
						      sta_sts->own_mac,
						      sta_sts->rssi);

	return ret;
}

static int wifi_manager_disconnect_event(void *arg)
{
	struct wifimgr_evt_disconnect *evt_disc =
	    (struct wifimgr_evt_disconnect *)arg;
	struct wifi_manager *mgr =
	    container_of(evt_disc, struct wifi_manager, evt_disc);
	int ret = 0;

	syslog(LOG_INFO, "disconnect, reason_code %d!\n",
	       evt_disc->reason_code);
	fflush(stdout);

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_DISCONNECT);
/*
	if (mgr->sta_iface) {
		syslog(LOG_INFO, "stop DHCP\n");
		dhcp_stop(mgr->sta_iface);
	}
*/

	/* Notify the external caller */
	if (wifimgr_get_ctrl_cbs() && wifimgr_get_ctrl_cbs()->notify_disconnect)
		wifimgr_get_ctrl_cbs()->notify_disconnect(evt_disc->
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
		if (mgr->sta_iface) {
			syslog(LOG_INFO, "start DHCP client\n");
			dhcp_start(mgr->sta_iface);
		}
*/

		/* Notify the external caller */
		if (wifimgr_get_ctrl_cbs()
		    && wifimgr_get_ctrl_cbs()->notify_connect)
			wifimgr_get_ctrl_cbs()->notify_connect(evt_conn->
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
	struct wifi_manager *mgr =
	    container_of(evt_scan_res, struct wifi_manager, evt_scan_res);

	syslog(LOG_INFO, "\t%-32s", evt_scan_res->ssid);
	syslog(LOG_INFO, "\t%02x:%02x:%02x:%02x:%02x:%02x\t%u\t%d\n",
	       evt_scan_res->bssid[0], evt_scan_res->bssid[1],
	       evt_scan_res->bssid[2], evt_scan_res->bssid[3],
	       evt_scan_res->bssid[4], evt_scan_res->bssid[5],
	       evt_scan_res->channel, evt_scan_res->rssi);
	fflush(stdout);

	/* Found specified AP */
	if (!strcmp(evt_scan_res->ssid, mgr->sta_conf.ssid))
		if (!strncmp
		    (evt_scan_res->bssid, mgr->sta_conf.bssid, WIFIMGR_ETH_ALEN)
		    || is_zero_ether_addr(mgr->sta_conf.bssid))
			mgr->sta_conf.found = true;

	/* Notify the external caller */
	if (wifimgr_get_ctrl_cbs() && wifimgr_get_ctrl_cbs()->notify_scan_res)
		wifimgr_get_ctrl_cbs()->notify_scan_res(evt_scan_res->ssid,
							evt_scan_res->bssid,
							evt_scan_res->band,
							evt_scan_res->channel,
							evt_scan_res->rssi);

	return 0;
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

	/* Notify the external caller */
	if (wifimgr_get_ctrl_cbs() && wifimgr_get_ctrl_cbs()->notify_scan_done)
		wifimgr_get_ctrl_cbs()->notify_scan_done(mgr->sta_conf.found);

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
