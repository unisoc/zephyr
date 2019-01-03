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
		wifimgr_err("[%s] timeout!\n", wifimgr_evt2str(expected_evt));
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_SCAN_RESULT);
		evt_listener_remove_receiver(&mgr->lsnr, expected_evt);

		if (cbs && cbs->notify_scan_timeout)
			cbs->notify_scan_timeout();
		break;
	case WIFIMGR_CMD_CONNECT:
		expected_evt = WIFIMGR_EVT_CONNECT;
		wifimgr_err("[%s] timeout!\n", wifimgr_evt2str(expected_evt));
		evt_listener_remove_receiver(&mgr->lsnr, expected_evt);

		if (cbs && cbs->notify_connect_timeout)
			cbs->notify_connect_timeout();
		break;
	case WIFIMGR_CMD_DISCONNECT:
		expected_evt = WIFIMGR_EVT_DISCONNECT;
		wifimgr_err("[%s] timeout!\n", wifimgr_evt2str(expected_evt));

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
	char *ssid = NULL;
	char *bssid = NULL;
	char *passphrase = NULL;

	/* Load config form non-volatile memory */
	wifimgr_config_load(conf, WIFIMGR_SETTING_STA_PATH);

	if (!memiszero(conf, sizeof(struct wifimgr_config))) {
		wifimgr_info("No STA Config found!\n");
	} else {
		wifimgr_info("STA Config\n");
		if (conf->autorun)
			wifimgr_info("Autorun:\t%d\n", conf->autorun);

		if (strlen(conf->ssid)) {
			ssid = conf->ssid;
			wifimgr_info("SSID:\t\t%s\n", ssid);
		}

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
	}

	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->get_sta_conf_cb)
		cbs->get_sta_conf_cb(ssid, bssid, passphrase, conf->band,
				     conf->channel, conf->autorun);

	return 0;
}

static int wifimgr_sta_set_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;

	if (!memiszero(conf, sizeof(struct wifimgr_config))) {
		wifimgr_info("Clearing STA Config ...\n");
		wifimgr_config_clear(conf, WIFIMGR_SETTING_STA_PATH);
		return 0;
	}

	wifimgr_info("Setting STA Config ...\n");

	if (conf->autorun)
		wifimgr_info("Autorun:\t%d\n", conf->autorun);

	if (strlen(conf->ssid))
		wifimgr_info("SSID:\t\t%s\n", conf->ssid);

	if (!is_zero_ether_addr(conf->bssid))
		wifimgr_info("BSSID:\t\t%02x:%02x:%02x:%02x:%02x:%02x\n",
			     conf->bssid[0], conf->bssid[1], conf->bssid[2],
			     conf->bssid[3], conf->bssid[4], conf->bssid[5]);

	if (strlen(conf->passphrase))
		wifimgr_info("Passphrase:\t%s\n", conf->passphrase);

	if (conf->channel)
		wifimgr_info("Channel:\t%d\n", conf->channel);

	fflush(stdout);

	/* Save config to non-volatile memory */
	wifimgr_config_save(conf, WIFIMGR_SETTING_STA_PATH);

	return 0;
}

static int wifimgr_sta_get_capa(void *handle)
{
	/* TODO */

	return 0;
}

static int wifimgr_sta_get_status(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sm = &mgr->sta_sm;
	struct wifimgr_status *sts = &mgr->sta_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	char *ssid = NULL;
	char *bssid = NULL;

	wifimgr_info("STA Status:\t%s\n", sta_sts2str(sm_sta_query(sm)));

	if (is_zero_ether_addr(sts->own_mac))
		if (wifi_drv_get_mac(mgr->sta_iface, sts->own_mac))
			wifimgr_warn("failed to get Own MAC!\n");
	wifimgr_info("Own MAC:\t%02x:%02x:%02x:%02x:%02x:%02x\n",
		     sts->own_mac[0], sts->own_mac[1], sts->own_mac[2],
		     sts->own_mac[3], sts->own_mac[4], sts->own_mac[5]);

	if (sm_sta_connected(sm) == true) {
		wifimgr_info("----------------\n");

		if (strlen(sts->u.sta.host_ssid)) {
			ssid = sts->u.sta.host_ssid;
			wifimgr_info("Host SSID:\t%s\n", ssid);
		}

		if (!is_zero_ether_addr(sts->u.sta.host_bssid)) {
			bssid = sts->u.sta.host_bssid;
			wifimgr_info
			    ("Host BSSID:\t%02x:%02x:%02x:%02x:%02x:%02x\n",
			     bssid[0], bssid[1], bssid[2],
			     bssid[3], bssid[4], bssid[5]);
		}

		if (sts->u.sta.host_channel)
			wifimgr_info("Host Channel:\t%d\n",
				     sts->u.sta.host_channel);

		if (wifi_drv_get_station(mgr->sta_iface, &sts->u.sta.host_rssi))
			wifimgr_warn("failed to get Host RSSI!\n");
		wifimgr_info("Host RSSI:\t%d\n", sts->u.sta.host_rssi);
	}
	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->get_sta_status_cb)
		cbs->get_sta_status_cb(sm_sta_query(sm), sts->own_mac,
				       ssid, bssid, sts->u.sta.host_channel,
				       sts->u.sta.host_rssi);

	return 0;
}

static int wifimgr_sta_disconnect_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	struct wifi_drv_disconnect_evt *disc = &sta_evt->u.disc;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);
	struct wifimgr_status *sts = &mgr->sta_sts;
	struct net_if *iface = (struct net_if *)mgr->sta_iface;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();

	wifimgr_info("disconnect, reason_code %d!\n", disc->reason_code);
	fflush(stdout);

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_DISCONNECT);
	memset(sts->u.sta.host_ssid, 0, WIFIMGR_MAX_SSID_LEN + 1);
	memset(sts->u.sta.host_bssid, 0, WIFIMGR_ETH_ALEN);
	sts->u.sta.host_channel = 0;
	sts->u.sta.host_rssi = 0;

	if (iface)
		wifimgr_dhcp_stop(iface);

	/* Notify the external caller */
	if (cbs && cbs->notify_disconnect)
		cbs->notify_disconnect(disc->reason_code);

	return 0;
}

static int wifimgr_sta_disconnect(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_DISCONNECT,
				  true, wifimgr_sta_disconnect_event,
				  &mgr->sta_evt);

	ret = wifi_drv_disconnect(mgr->sta_iface);
	if (ret)
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_DISCONNECT);

	return ret;
}

static int wifimgr_sta_connect_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	struct wifi_drv_connect_evt *conn = &sta_evt->u.conn;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);
	struct wifimgr_status *sts = &mgr->sta_sts;
	struct wifimgr_config *conf = &mgr->sta_conf;
	struct net_if *iface = (struct net_if *)mgr->sta_iface;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	int ret = conn->status;

	if (!ret) {
		wifimgr_info("connect successfully!\n");

		/* Register disconnect event here due to AP deauth */
		evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_DISCONNECT,
					  true, wifimgr_sta_disconnect_event,
					  &mgr->sta_evt);

		cmd_processor_add_sender(&mgr->prcs,
					 WIFIMGR_CMD_DISCONNECT,
					 wifimgr_sta_disconnect, mgr);
		if (strlen(conf->ssid))
			strcpy(sts->u.sta.host_ssid, conf->ssid);
		if (!is_zero_ether_addr(conn->bssid))
			memcpy(sts->u.sta.host_bssid, conn->bssid,
			       WIFIMGR_ETH_ALEN);
		if (conn->channel)
			sts->u.sta.host_channel = conn->channel;

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
	char *ssid = NULL;
	char *bssid = NULL;
	char *passphrase = NULL;
	int ret;

	wifimgr_info("Connecting to %s\n", conf->ssid);

	ret = evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_CONNECT,
					true, wifimgr_sta_connect_event,
					&mgr->sta_evt);
	if (ret)
		return ret;

	if (strlen(conf->ssid))
		ssid = conf->ssid;
	if (!is_zero_ether_addr(conf->bssid))
		bssid = conf->bssid;
	if (strlen(conf->passphrase))
		passphrase = conf->passphrase;
	ret = wifi_drv_connect(mgr->sta_iface, ssid, bssid, passphrase,
			       conf->channel);

	if (ret) {
		wifimgr_err("failed to connect! %d\n", ret);
		evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_CONNECT);
	}

	return ret;
}

static int wifimgr_sta_scan_result_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	struct wifi_drv_scan_result_evt *scan_res = &sta_evt->u.scan_res;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);
	struct wifimgr_config *conf = &mgr->sta_conf;
	struct wifimgr_status *sts = &mgr->sta_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	char *ssid = NULL;
	char *bssid = NULL;

	if (strlen(scan_res->ssid)) {
		ssid = scan_res->ssid;
		wifimgr_info("\t%-32s", ssid);
	}

	if (!is_zero_ether_addr(scan_res->bssid)) {
		bssid = scan_res->bssid;
		wifimgr_info("\t%02x:%02x:%02x:%02x:%02x:%02x\t%u\t%d\n",
			     bssid[0], bssid[1], bssid[2],
			     bssid[3], bssid[4], bssid[5],
			     scan_res->channel, scan_res->rssi);
	}

	fflush(stdout);

	/* Found specified AP */
	if (!strcmp(scan_res->ssid, conf->ssid)) {
		if (!strncmp(scan_res->bssid, conf->bssid, WIFIMGR_ETH_ALEN))
			sts->u.sta.host_found = true;
		/* Choose the first match when BSSID is not specified */
		else if (is_zero_ether_addr(conf->bssid))
			sts->u.sta.host_found = true;
	}

	/* Notify the external caller */
	if (cbs && cbs->notify_scan_res)
		cbs->notify_scan_res(ssid, bssid, scan_res->band,
				     scan_res->channel, scan_res->rssi);

	return 0;
}

static int wifimgr_sta_scan_done_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	struct wifi_drv_scan_done_evt *scan_done = &sta_evt->u.scan_done;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	int ret = scan_done->result;

	evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_RESULT);

	if (!ret)
		wifimgr_info("scan done!\n");
	else
		wifimgr_err("scan abort!\n");
	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->notify_scan_done)
		cbs->notify_scan_done(mgr->sta_sts.u.sta.host_found);

	return ret;
}

static int wifimgr_sta_scan(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_config *conf = &mgr->sta_conf;
	int ret;

	ret = evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_RESULT,
					false, wifimgr_sta_scan_result_event,
					&mgr->sta_evt);
	if (ret)
		return ret;

	ret = evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_DONE,
					true, wifimgr_sta_scan_done_event,
					&mgr->sta_evt);
	if (ret) {
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_SCAN_RESULT);
		return ret;
	}

	mgr->sta_sts.u.sta.host_found = false;

	ret = wifi_drv_scan(mgr->sta_iface, conf->band, conf->channel);
	if (ret) {
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_SCAN_RESULT);
		evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_SCAN_DONE);
	}

	return ret;
}

static int wifimgr_sta_open(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_open(mgr->sta_iface);
	if (ret) {
		wifimgr_err("failed to open STA!\n");
		return ret;
	}

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_STA);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_STA,
				 wifimgr_sta_close, mgr);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_SCAN,
				 wifimgr_sta_scan, mgr);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_CONNECT,
				 wifimgr_sta_connect, mgr);

	return ret;
}

static int wifimgr_sta_close(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_close(mgr->sta_iface);
	if (ret) {
		wifimgr_err("failed to close STA!\n");
		return ret;
	}

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_STA);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_DISCONNECT);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_CONNECT);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_SCAN);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_STA,
				 wifimgr_sta_open, mgr);

	return ret;
}

void wifimgr_sta_init(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct cmd_processor *prcs = &mgr->prcs;

	/* Register default STA commands */
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_SET_STA_CONFIG,
				 wifimgr_sta_set_config, &mgr->sta_conf);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_STA_CONFIG,
				 wifimgr_sta_get_config, &mgr->sta_conf);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_STA_CAPA,
				 wifimgr_sta_get_capa, mgr);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_STA_STATUS,
				 wifimgr_sta_get_status, mgr);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_OPEN_STA,
				 wifimgr_sta_open, mgr);
}
