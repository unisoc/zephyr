/*
 * @file
 * @brief Station mode handling
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

static int wifimgr_sta_close(void *handle);

void wifimgr_sta_event_timeout(wifimgr_work *work)
{
	struct wifimgr_state_machine *sta_sm =
	    container_of(work, struct wifimgr_state_machine, work);
	struct wifi_manager *mgr =
	    container_of(sta_sm, struct wifi_manager, sta_sm);
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	unsigned int expected_evt;

	/* Remove event receiver, notify the external caller */
	switch (sta_sm->cur_cmd) {
	case WIFIMGR_CMD_SCAN:
		expected_evt = WIFIMGR_EVT_SCAN_DONE;
		wifimgr_err("[%s] timeout!\n",
		       wifimgr_evt2str(expected_evt));
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_SCAN_RESULT);
		event_listener_remove_receiver(&mgr->lsnr, expected_evt);

		if (cbs && cbs->notify_scan_timeout)
			cbs->notify_scan_timeout();
		break;
	case WIFIMGR_CMD_CONNECT:
		expected_evt = WIFIMGR_EVT_CONNECT;
		wifimgr_err("[%s] timeout!\n",
		       wifimgr_evt2str(expected_evt));
		event_listener_remove_receiver(&mgr->lsnr, expected_evt);

		if (cbs && cbs->notify_connect_timeout)
			cbs->notify_connect_timeout();
		break;
	case WIFIMGR_CMD_DISCONNECT:
		expected_evt = WIFIMGR_EVT_DISCONNECT;
		wifimgr_err("[%s] timeout!\n",
		       wifimgr_evt2str(expected_evt));

		if (cbs && cbs->notify_disconnect_timeout)
			cbs->notify_disconnect_timeout();
		break;
	default:
		break;
	}

	sm_sta_step_back(sta_sm);
}

static int wifimgr_sta_get_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();

	wifimgr_load_config(conf, WIFIMGR_SETTING_STA_PATH);

	wifimgr_info("STA Config\n");
	if (strlen(conf->ssid))
		wifimgr_info("SSID:\t\t%s\n", conf->ssid);

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
		cbs->get_conf_cb(WIFIMGR_IFACE_NAME_STA,
				 strlen(conf->ssid) ? conf->ssid : NULL,
				 is_zero_ether_addr(conf->bssid) ? NULL :
				 conf->bssid,
				 strlen(conf->passphrase) ? conf->
				 passphrase : NULL, conf->band, conf->channel);

	return 0;
}

static int wifimgr_sta_set_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;

	wifimgr_info("Setting STA Config ...\n");
	wifimgr_info("SSID:\t\t%s\n", conf->ssid);

	if (strlen(conf->passphrase))
		wifimgr_info("Passphrase:\t%s\n", conf->passphrase);
	fflush(stdout);

	wifimgr_save_config(conf, WIFIMGR_SETTING_STA_PATH);

	return 0;
}

static int wifimgr_sta_get_status(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sm = &mgr->sta_sm;
	struct wifimgr_status *sts = &mgr->sta_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	int ret = 0;

	wifimgr_info("STA Status:\t%s\n",
	       sta_sts2str(sm_sta_query(sm)));

	if (is_zero_ether_addr(sts->own_mac))
		ret = wifi_drv_iface_get_mac(mgr->sta_iface, sts->own_mac);
	wifimgr_info("Own MAC:\t%02x:%02x:%02x:%02x:%02x:%02x\n",
	       sts->own_mac[0], sts->own_mac[1], sts->own_mac[2],
	       sts->own_mac[3], sts->own_mac[4], sts->own_mac[5]);

	if (sm_sta_connected(sm) == true) {
		wifimgr_info("----------------\n");
		wifimgr_info("Host SSID:\t%s\n", sts->u.sta.host_ssid);
		if (!is_zero_ether_addr(sts->u.sta.host_bssid))
			wifimgr_info("Host BSSID:\t%02x:%02x:%02x:%02x:%02x:%02x\n",
			       sts->u.sta.host_bssid[0],
			       sts->u.sta.host_bssid[1],
			       sts->u.sta.host_bssid[2],
			       sts->u.sta.host_bssid[3],
			       sts->u.sta.host_bssid[4],
			       sts->u.sta.host_bssid[5]);
		ret = wifi_drv_iface_get_station(mgr->sta_iface, &sts->u.sta.host_rssi);
		wifimgr_info("Host Signal:\t%d\n", sts->u.sta.host_rssi);
	}
	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->get_sta_status_cb)
		cbs->get_sta_status_cb(sm_sta_query(sm),
				   sts->own_mac, sts->u.sta.host_rssi, sts->u.sta.host_ssid, sts->u.sta.host_bssid);

	return ret;
}

static int wifimgr_sta_disconnect_event(void *arg)
{
	struct wifimgr_evt_disconnect *disc =
	    (struct wifimgr_evt_disconnect *)arg;
	struct wifi_manager *mgr =
	    container_of(disc, struct wifi_manager, evt_disc);
	struct wifimgr_status *sts = &mgr->sta_sts;
	struct net_if *iface = (struct net_if *)mgr->sta_iface;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	int ret = 0;

	wifimgr_info("disconnect, reason_code %d!\n", disc->reason_code);
	fflush(stdout);

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_DISCONNECT);
	sts->u.sta.host_rssi = 0;
	memset(sts->u.sta.host_ssid, 0x0, WIFIMGR_MAX_SSID_LEN + 1);
	memset(sts->u.sta.host_bssid, 0x0, WIFIMGR_ETH_ALEN);

	if (iface)
		wifimgr_dhcp_stop(iface);

	/* Notify the external caller */
	if (cbs && cbs->notify_disconnect)
		cbs->notify_disconnect(disc->reason_code);

	return ret;
}

static int wifimgr_sta_disconnect(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_DISCONNECT,
				    true, wifimgr_sta_disconnect_event,
				    &mgr->evt_disc);

	ret = wifi_drv_iface_disconnect(mgr->sta_iface);
	if (ret)
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_DISCONNECT);

	return ret;
}

static int wifimgr_sta_connect_event(void *arg)
{
	struct wifimgr_evt_connect *conn = (struct wifimgr_evt_connect *)arg;
	struct wifi_manager *mgr =
	    container_of(conn, struct wifi_manager, evt_conn);
	struct wifimgr_status *sts = &mgr->sta_sts;
	struct wifimgr_config *conf = &mgr->sta_conf;
	struct net_if *iface = (struct net_if *)mgr->sta_iface;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	int ret = conn->status;

	if (!ret) {
		wifimgr_info("connect successfully!\n");

		/* Register disconnect event here due to AP deauth */
		event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_DISCONNECT,
					    true, wifimgr_sta_disconnect_event,
					    &mgr->evt_disc);

		command_processor_register_sender(&mgr->prcs,
						  WIFIMGR_CMD_DISCONNECT,
						  wifimgr_sta_disconnect, mgr);
		if (conf->ssid && strlen(conf->ssid))
			strcpy(sts->u.sta.host_ssid, conf->ssid);
		if (conf->bssid && !is_zero_ether_addr(conf->bssid))
			memcpy(sts->u.sta.host_bssid, conf->bssid, WIFIMGR_ETH_ALEN);

		if (iface)
			wifimgr_dhcp_start(iface);

	} else {
		wifimgr_err("failed to connect!\n");
	}
	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->notify_connect)
		cbs->notify_connect(conn->status);

	return ret;
}

static int wifimgr_sta_connect(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *conf = &mgr->sta_conf;
	int ret;

	if (!strlen(conf->ssid)) {
		wifimgr_err("no AP specified!\n");
		return -EINVAL;
	}

	wifimgr_info("Connecting to %s\n", conf->ssid);

	ret = event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_CONNECT,
					  true, wifimgr_sta_connect_event,
					  &mgr->evt_conn);
	if (ret)
		return ret;

	ret = wifi_drv_iface_connect(mgr->sta_iface, conf->ssid,
				     conf->passphrase);

	if (ret)
		event_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_CONNECT);

	return ret;
}

static int wifimgr_sta_scan_result(void *arg)
{
	struct wifimgr_evt_scan_result *scan_res =
	    (struct wifimgr_evt_scan_result *)arg;
	struct wifi_manager *mgr =
	    container_of(scan_res, struct wifi_manager, evt_scan_res);
	struct wifimgr_config *conf = &mgr->sta_conf;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();


	wifimgr_info("\t%-32s", scan_res->ssid);
	wifimgr_info("\t%02x:%02x:%02x:%02x:%02x:%02x\t%u\t%d\n",
	       scan_res->bssid[0], scan_res->bssid[1], scan_res->bssid[2],
	       scan_res->bssid[3], scan_res->bssid[4], scan_res->bssid[5],
	       scan_res->channel, scan_res->rssi);
	fflush(stdout);

	/* Found specified AP */
	if (!strcmp(scan_res->ssid, conf->ssid)) {
		if (!strncmp (scan_res->bssid, conf->bssid, WIFIMGR_ETH_ALEN)) {
			mgr->evt_scan_done.found = true;
		/* Choose the first match when BSSID is not specified */
		} else if (is_zero_ether_addr(conf->bssid)) {
			mgr->evt_scan_done.found = true;
			memcpy(conf->bssid, scan_res->bssid, WIFIMGR_ETH_ALEN);
		}
	}

	/* Notify the external caller */
	if (cbs && cbs->notify_scan_res)
		cbs->notify_scan_res(strlen(scan_res->ssid) ? scan_res->
				     ssid : NULL,
				     is_zero_ether_addr(scan_res->
							bssid) ? NULL :
				     scan_res->bssid, scan_res->band,
				     scan_res->channel, scan_res->rssi);

	return 0;
}

static int wifimgr_sta_scan_done(void *arg)
{
	struct wifimgr_evt_scan_done *scan_done =
	    (struct wifimgr_evt_scan_done *)arg;
	struct wifi_manager *mgr =
	    container_of(scan_done, struct wifi_manager, evt_scan_done);
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	int ret = scan_done->result;

	event_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_RESULT);

	if (!ret)
		wifimgr_info("scan done!\n");
	else
		wifimgr_err("scan abort!\n");
	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->notify_scan_done)
		cbs->notify_scan_done(scan_done->found);

	return ret;
}

static int wifimgr_sta_scan(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *conf = &mgr->sta_conf;
	int ret;

	ret = event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_RESULT,
					  false, wifimgr_sta_scan_result,
					  &mgr->evt_scan_res);
	if (ret)
		return ret;

	ret = event_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_DONE,
					  true, wifimgr_sta_scan_done,
					  &mgr->evt_scan_done);
	if (ret) {
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_SCAN_RESULT);
		return ret;
	}

	mgr->evt_scan_done.found = false;

	ret = wifi_drv_iface_scan(mgr->sta_iface, conf->band, conf->channel);
	if (ret) {
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_SCAN_RESULT);
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_SCAN_DONE);
	}

	return ret;
}

static int wifimgr_sta_open(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_open_station(mgr->sta_iface);
	if (ret) {
		wifimgr_err("failed to open STA!\n");
		return ret;
	}

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_STA);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_STA,
					  wifimgr_sta_close, mgr);
	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_SCAN,
					  wifimgr_sta_scan, mgr);
	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_CONNECT,
					  wifimgr_sta_connect, mgr);

	return ret;
}

static int wifimgr_sta_close(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_iface_close_station(mgr->sta_iface);
	if (ret) {
		wifimgr_err("failed to close STA!\n");
		return ret;
	}

	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_STA);
	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_DISCONNECT);
	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_CONNECT);
	command_processor_unregister_sender(&mgr->prcs, WIFIMGR_CMD_SCAN);

	command_processor_register_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_STA,
					  wifimgr_sta_open, mgr);

	return ret;
}

void wifimgr_sta_init(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct cmd_processor *prcs = &mgr->prcs;

	/* Register default STA commands */
	command_processor_register_sender(prcs, WIFIMGR_CMD_SET_STA_CONFIG,
					  wifimgr_sta_set_config,
					  &mgr->sta_conf);
	command_processor_register_sender(prcs, WIFIMGR_CMD_GET_STA_CONFIG,
					  wifimgr_sta_get_config,
					  &mgr->sta_conf);
	command_processor_register_sender(prcs, WIFIMGR_CMD_GET_STA_STATUS,
					  wifimgr_sta_get_status, mgr);
	command_processor_register_sender(prcs, WIFIMGR_CMD_OPEN_STA,
					  wifimgr_sta_open, mgr);
}
