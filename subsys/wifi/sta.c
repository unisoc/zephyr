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
	struct wifimgr_state_machine *sm =
	    container_of(work, struct wifimgr_state_machine, work);
	struct wifi_manager *mgr =
	    container_of(sm, struct wifi_manager, sta_sm);
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	unsigned int expected_evt;

	/* Remove event receiver, notify the external caller */
	switch (sm->cur_cmd) {
	case WIFIMGR_CMD_SCAN:
		expected_evt = WIFIMGR_EVT_SCAN_DONE;
		wifimgr_warn("[%s] timeout!\n", wifimgr_evt2str(expected_evt));
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_SCAN_RESULT);
		evt_listener_remove_receiver(&mgr->lsnr, expected_evt);

		if (cbs && cbs->notify_scan_timeout)
			cbs->notify_scan_timeout();
		break;
	case WIFIMGR_CMD_CONNECT:
		expected_evt = WIFIMGR_EVT_CONNECT;
		wifimgr_warn("[%s] timeout!\n", wifimgr_evt2str(expected_evt));
		evt_listener_remove_receiver(&mgr->lsnr, expected_evt);

		if (cbs && cbs->notify_connect_timeout)
			cbs->notify_connect_timeout();
		break;
	case WIFIMGR_CMD_DISCONNECT:
		expected_evt = WIFIMGR_EVT_DISCONNECT;
		wifimgr_warn("[%s] timeout!\n", wifimgr_evt2str(expected_evt));

		if (cbs && cbs->notify_disconnect_timeout)
			cbs->notify_disconnect_timeout();
		break;
	default:
		break;
	}

	sm_sta_step_back(sm);
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

	if (!memiszero(conf, sizeof(struct wifimgr_config)))
		wifimgr_info("No STA Config found!\n");

	if (strlen(conf->ssid))
		ssid = conf->ssid;

	if (!is_zero_ether_addr(conf->bssid))
		bssid = conf->bssid;

	if (strlen(conf->passphrase))
		passphrase = conf->passphrase;

	/* Notify the external caller */
	if (cbs && cbs->get_sta_conf_cb)
		cbs->get_sta_conf_cb(ssid, bssid, passphrase, conf->band,
				     conf->channel, conf->security,
				     conf->autorun);

	return 0;
}

static int wifimgr_sta_set_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;

	if (!memiszero(conf, sizeof(struct wifimgr_config))) {
		wifimgr_info("Clearing STA Config ...\n");
		wifimgr_config_clear(conf, WIFIMGR_SETTING_STA_PATH);
	} else {
		wifimgr_info("Setting STA Config ...\n");
		/* Save config to non-volatile memory */
		wifimgr_config_save(conf, WIFIMGR_SETTING_STA_PATH);
	}

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

	if (sm_sta_connected(sm) == true)
		if (wifi_drv_get_station(mgr->sta_iface, &sts->u.sta.host_rssi))
			wifimgr_warn("failed to get Host RSSI!\n");

	/* Notify the external caller */
	if (cbs && cbs->get_sta_status_cb)
		cbs->get_sta_status_cb(sm_sta_query(sm), sts->own_mac,
				       sts->u.sta.host_bssid,
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

	wifimgr_info("disconnect! reason: %d\n", disc->reason_code);

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_DISCONNECT);
	memset(sts->u.sta.host_bssid, 0, WIFIMGR_ETH_ALEN);
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

	ret = evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_DISCONNECT,
					true, wifimgr_sta_disconnect_event,
					&mgr->sta_evt);
	if (ret)
		return ret;

	ret = wifi_drv_disconnect(mgr->sta_iface);
	if (ret) {
		wifimgr_err("failed to disconnect! %d\n", ret);
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_DISCONNECT);
	}

	return ret;
}

static int wifimgr_sta_connect_event(void *arg)
{
	struct wifimgr_sta_event *sta_evt = (struct wifimgr_sta_event *)arg;
	struct wifi_drv_connect_evt *conn = &sta_evt->u.conn;
	struct wifi_manager *mgr =
	    container_of(sta_evt, struct wifi_manager, sta_evt);
	struct wifimgr_status *sts = &mgr->sta_sts;
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

		if (!is_zero_ether_addr(conn->bssid))
			memcpy(sts->u.sta.host_bssid, conn->bssid,
			       WIFIMGR_ETH_ALEN);

		if (iface)
			wifimgr_dhcp_start(iface);

	} else {
		wifimgr_err("failed to connect!\n");
	}

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
	char *psk = NULL;
	char psk_len = 0;
	char wpa_psk[WIFIMGR_PSK_LEN];
	int ret;

	ret = evt_listener_add_receiver(&mgr->lsnr, WIFIMGR_EVT_CONNECT,
					true, wifimgr_sta_connect_event,
					&mgr->sta_evt);
	if (ret)
		return ret;

	if (strlen(conf->ssid))
		ssid = conf->ssid;
	if (!is_zero_ether_addr(conf->bssid))
		bssid = conf->bssid;
	if (strlen(conf->passphrase)) {
		ret = pbkdf2_sha1(conf->passphrase, ssid, WIFIMGR_PSK_ITER,
				  wpa_psk, WIFIMGR_PSK_LEN);
		if (ret) {
			wifimgr_err("failed to calculate PSK! %d\n", ret);
			return ret;
		}
		psk = wpa_psk;
		psk_len = WIFIMGR_PSK_LEN;
	}

	ret = wifi_drv_connect(mgr->sta_iface, ssid, bssid, psk, psk_len,
			       conf->channel);
	if (ret) {
		wifimgr_err("failed to connect! %d\n", ret);
		evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_CONNECT);
	}

	wifimgr_info("Connecting to %s\n", conf->ssid);
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

	if (strlen(scan_res->ssid))
		ssid = scan_res->ssid;

	if (!is_zero_ether_addr(scan_res->bssid))
		bssid = scan_res->bssid;

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
				     scan_res->channel, scan_res->rssi,
				     scan_res->security);

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

	wifimgr_info("trgger scan!\n");
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

	wifimgr_info("open STA!\n");
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

	wifimgr_info("close STA!\n");
	return ret;
}

static int wifimgr_sta_drv_init(struct wifi_manager *mgr)
{
	char *devname = WIFIMGR_DEV_NAME_STA;
	struct net_if *iface = NULL;
	int ret;

	/* Intialize driver interface */
	iface = wifi_drv_init(devname);
	if (!iface) {
		wifimgr_err("failed to init WiFi STA driver!\n");
		return -ENODEV;
	}
	mgr->sta_iface = iface;

	/* Get MAC address */
	ret = wifi_drv_get_mac(iface, mgr->sta_sts.own_mac);
	if (ret)
		wifimgr_warn("failed to get Own MAC!\n");

	wifimgr_info("interface %s(" MACSTR ") initialized!\n", devname,
		     MAC2STR(mgr->sta_sts.own_mac));
	return 0;
}

int wifimgr_sta_init(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct cmd_processor *prcs = &mgr->prcs;
	int ret;

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

	/* Initialize STA config */
	ret = wifimgr_config_init(&mgr->sta_conf, WIFIMGR_SETTING_STA_PATH);
	if (ret)
		wifimgr_warn("failed to init WiFi STA config!\n");

	/* Intialize STA driver */
	ret = wifimgr_sta_drv_init(mgr);
	if (ret) {
		wifimgr_err("failed to init WiFi STA driver!\n");
		return ret;
	}

	/* Initialize STA state machine */
	ret = wifimgr_sm_init(&mgr->sta_sm, wifimgr_sta_event_timeout);
	if (ret)
		wifimgr_err("failed to init WiFi STA state machine!\n");

	return ret;
}

void wifimgr_sta_exit(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;

	/* Deinitialize STA state machine */
	wifimgr_sm_exit(&mgr->sta_sm);

	/* Deinitialize STA config */
	wifimgr_config_exit(WIFIMGR_SETTING_STA_PATH);
}
