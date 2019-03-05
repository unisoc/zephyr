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
#include "timer.h"
#include "sm.h"
#include "psk.h"
#include "dhcpc.h"
#include "led.h"

#ifdef CONFIG_WIFI_STA_DRV_NAME
#define WIFIMGR_DEV_NAME_STA	CONFIG_WIFI_STA_DRV_NAME
#else
#define WIFIMGR_DEV_NAME_STA	"WIFI_STA"
#endif

#ifdef CONFIG_WIFI_AP_DRV_NAME
#define WIFIMGR_DEV_NAME_AP	CONFIG_WIFI_AP_DRV_NAME
#else
#define WIFIMGR_DEV_NAME_AP	"WIFI_AP"
#endif

#define WIFIMGR_MAX_STA_NR	16

#define WIFIMGR_CMD_TIMEOUT	5
#define WIFIMGR_SCAN_TIMEOUT	10
#define WIFIMGR_EVENT_TIMEOUT	10

#define C2S(x) case x: return #x;

struct wifimgr_status {
	char own_mac[WIFIMGR_ETH_ALEN];
	union {
		struct {
			bool host_found;
			char host_bssid[WIFIMGR_ETH_ALEN];
			signed char host_rssi;
		} sta;
		struct {
			unsigned char sta_nr;
			char (*sta_mac_addrs)[WIFIMGR_ETH_ALEN];
			unsigned char acl_nr;
			char (*acl_mac_addrs)[WIFIMGR_ETH_ALEN];
		} ap;
	} u;
};

struct wifimgr_mac_node {
	wifimgr_snode_t node;
	char mac[WIFIMGR_ETH_ALEN];
};

struct wifimgr_mac_list {
	unsigned char nr;
	wifimgr_slist_t list;
};

struct wifimgr_sta_event {
	union {
		struct wifi_drv_connect_evt conn;
		struct wifi_drv_disconnect_evt disc;
		struct wifi_drv_scan_done_evt scan_done;
		struct wifi_drv_scan_result_evt scan_res;
	} u;
};

struct wifimgr_ap_event {
	union {
		struct wifi_drv_new_station_evt new_sta;
	} u;
};

struct wifi_manager {
	sem_t sta_ctrl;			/* STA global control */
	struct wifimgr_config sta_conf;
	struct wifimgr_status sta_sts;
	struct wifimgr_state_machine sta_sm;

	sem_t ap_ctrl;			/* AP global control */
	struct wifi_drv_capa ap_capa;
	struct wifimgr_config ap_conf;
	struct wifimgr_status ap_sts;
	struct wifimgr_state_machine ap_sm;
	struct wifimgr_mac_list assoc_list;
	struct wifimgr_mac_list mac_acl;
	struct wifimgr_set_mac_acl set_acl;

	struct cmd_processor prcs;
	struct evt_listener lsnr;

	void *sta_iface;
	void *ap_iface;

	struct wifimgr_sta_event sta_evt;
	struct wifimgr_ap_event ap_evt;
};

int wifimgr_sta_init(void *handle);
void wifimgr_sta_exit(void *handle);
int wifimgr_ap_init(void *handle);
void wifimgr_ap_exit(void *handle);

#endif
