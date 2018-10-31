/*
 * @file
 * @brief WiFi manager header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_H_
#define _WIFIMGR_H_

#include <net/wifimgr_drv.h>
#include <net/wifimgr_api.h>

#include "os_adapter.h"
#include "cmd_prcs.h"
#include "evt_lsnr.h"
#include "sm.h"
#include "drv_iface.h"
#include "api.h"
#include "dhcpc.h"

#define WIFIMGR_DEV_NAME_STA	CONFIG_WIFI_STA_DRV_NAME
#define WIFIMGR_DEV_NAME_AP	CONFIG_WIFI_AP_DRV_NAME

#define WIFIMGR_MAX_SSID_LEN	32
#define WIFIMGR_MAX_PSPHR_LEN	32
#define WIFIMGR_ETH_ALEN	6

#define WIFIMGR_CMD_TIMEOUT	5
#define WIFIMGR_SCAN_TIMEOUT	10
#define WIFIMGR_EVENT_TIMEOUT	10

#define E2S(x) case x: return #x;

struct wifimgr_config {
	char ssid[WIFIMGR_MAX_SSID_LEN];
	char bssid[WIFIMGR_ETH_ALEN];
	char passphrase[WIFIMGR_MAX_PSPHR_LEN];
	unsigned char band;
	unsigned char channel;
	bool found;
};

struct wifimgr_status {
	char own_mac[WIFIMGR_ETH_ALEN];
	char rssi;
};

struct wifimgr_evt_connect {
	char status;
};

struct wifimgr_evt_disconnect {
	char reason_code;
};

struct wifimgr_evt_scan_done {
	char result;
};

struct wifimgr_evt_scan_result {
	char ssid[WIFIMGR_MAX_SSID_LEN];
	char bssid[WIFIMGR_ETH_ALEN];
	unsigned char band;
	unsigned char channel;
	int8_t rssi;
};

struct wifimgr_evt_new_station {
	char is_connect;
	char mac[WIFIMGR_ETH_ALEN];
};

struct wifi_manager {
	bool fw_loaded;

	struct wifimgr_config sta_conf;
	struct wifimgr_status sta_sts;
	struct wifimgr_state_machine sta_sm;

	struct wifimgr_config ap_conf;
	struct wifimgr_status ap_sts;
	struct wifimgr_state_machine ap_sm;

	struct cmd_processor prcs;
	struct evt_listener lsnr;

	void *sta_iface;
	void *ap_iface;

	struct wifimgr_evt_connect evt_conn;
	struct wifimgr_evt_disconnect evt_disc;
	struct wifimgr_evt_scan_done evt_scan_done;
	struct wifimgr_evt_scan_result evt_scan_res;

	struct wifimgr_evt_new_station evt_new_sta;
};

const char *wifimgr_cmd2str(int cmd);
const char *wifimgr_evt2str(int evt);
const char *wifimgr_sts2str_cmd(struct wifi_manager *mgr, unsigned int cmd_id);
const char *wifimgr_sts2str_evt(struct wifi_manager *mgr, unsigned int cmd_id);
int wifi_manager_sm_start_timer(struct wifi_manager *mgr, unsigned int cmd_id);
int wifi_manager_sm_stop_timer(struct wifi_manager *mgr, unsigned int cmd_id);
int wifi_manager_sm_query_cmd(struct wifi_manager *mgr, unsigned int cmd_id);
void wifi_manager_sm_step_cmd(struct wifi_manager *mgr, unsigned int cmd_id);
void wifi_manager_sm_step_evt(struct wifi_manager *mgr, unsigned int evt_id);
void wifi_manager_sm_step_back(struct wifi_manager *mgr, unsigned int evt_id);
int wifi_manager_low_level_init(struct wifi_manager *mgr, unsigned int cmd_id);

int wifi_manager_get_sta_config(void *handle);
int wifi_manager_set_sta_config(void *handle);
int wifi_manager_get_sta_status(void *handle);
int wifi_manager_open_station(void *handle);
int wifi_manager_close_station(void *handle);
int wifi_manager_scan(void *handle);
int wifi_manager_connect(void *handle);
int wifi_manager_disconnect(void *handle);

int wifi_manager_get_ap_config(void *handle);
int wifi_manager_set_ap_config(void *handle);
int wifi_manager_get_ap_status(void *handle);
int wifi_manager_open_softap(void *handle);
int wifi_manager_close_softap(void *handle);
int wifi_manager_start_softap(void *handle);
int wifi_manager_stop_softap(void *handle);

#endif
