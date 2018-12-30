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
#include "api.h"
#include "config.h"
#include "drv_iface.h"
#include "cmd_prcs.h"
#include "evt_lsnr.h"
#include "sm.h"
#include "dhcpc.h"
#include "led.h"

#define WIFIMGR_DEV_NAME_STA	CONFIG_WIFI_STA_DRV_NAME
#define WIFIMGR_DEV_NAME_AP	CONFIG_WIFI_AP_DRV_NAME

#define WIFIMGR_MAX_STA_NR	16

#define WIFIMGR_CMD_TIMEOUT	5
#define WIFIMGR_SCAN_TIMEOUT	10
#define WIFIMGR_EVENT_TIMEOUT	10

#define E2S(x) case x: return #x;

struct wifimgr_capa {
	unsigned char max_ap_assoc_sta;
	unsigned char max_acl_mac_addrs;
};

struct wifimgr_status {
	char own_mac[WIFIMGR_ETH_ALEN];
	union {
		struct {
			char host_ssid[WIFIMGR_MAX_SSID_LEN + 1];
			char host_bssid[WIFIMGR_ETH_ALEN];
			unsigned char host_channel;
			char host_rssi;
		} sta;
		struct {
			unsigned char client_nr;
			char client_mac_addrs[WIFIMGR_MAX_STA_NR][WIFIMGR_ETH_ALEN];
		} ap;
	} u;
};

struct wifimgr_sta_event {
	union {
		struct wifi_drv_evt_connect conn;
		struct wifi_drv_evt_disconnect disc;
		struct wifi_drv_evt_scan_done scan_done;
		struct wifi_drv_evt_scan_result scan_res;
	} u;
};

struct wifimgr_ap_event {
	union {
		struct wifi_drv_evt_new_station new_sta;
	} u;
};

struct wifi_manager {
	struct wifimgr_capa capa;

	struct wifimgr_config sta_conf;
	struct wifimgr_status sta_sts;
	struct wifimgr_state_machine sta_sm;

	struct wifimgr_config ap_conf;
	struct wifimgr_status ap_sts;
	struct wifimgr_state_machine ap_sm;
	struct wifimgr_del_station del_sta;

	struct cmd_processor prcs;
	struct evt_listener lsnr;

	void *sta_iface;
	void *ap_iface;

	struct wifimgr_sta_event sta_evt;
	struct wifimgr_ap_event ap_evt;
};

const char *wifimgr_cmd2str(int cmd);
const char *wifimgr_evt2str(int evt);
const char *wifimgr_sts2str_cmd(struct wifi_manager *mgr, unsigned int cmd_id);
const char *wifimgr_sts2str_evt(struct wifi_manager *mgr, unsigned int cmd_id);
int wifimgr_sm_start_timer(struct wifi_manager *mgr, unsigned int cmd_id);
int wifimgr_sm_stop_timer(struct wifi_manager *mgr, unsigned int cmd_id);
int wifimgr_sm_query_cmd(struct wifi_manager *mgr, unsigned int cmd_id);
void wifimgr_sm_step_cmd(struct wifi_manager *mgr, unsigned int cmd_id);
void wifimgr_sm_step_evt(struct wifi_manager *mgr, unsigned int evt_id);
void wifimgr_sm_step_back(struct wifi_manager *mgr, unsigned int evt_id);
int wifimgr_low_level_init(struct wifi_manager *mgr, unsigned int cmd_id);

void wifimgr_sta_event_timeout(wifimgr_work *work);
void wifimgr_sta_init(void *handle);
void wifimgr_ap_event_timeout(wifimgr_work *work);
void wifimgr_ap_init(void *handle);

#endif
