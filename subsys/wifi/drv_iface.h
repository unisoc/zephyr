/*
 * @file
 * @brief Driver interface header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_DRV_IFACE_H_
#define _WIFIMGR_DRV_IFACE_H_

#include "os_adapter.h"

enum event_type {
	/*STA*/
	WIFIMGR_EVT_CONNECT,
	WIFIMGR_EVT_DISCONNECT,
	WIFIMGR_EVT_SCAN_RESULT,
	WIFIMGR_EVT_SCAN_DONE,
	/*AP*/
	WIFIMGR_EVT_NEW_STATION,

	WIFIMGR_EVT_MAX,
};

void *wifi_drv_init(char *devname);
int wifi_drv_get_mac(void *iface, char *mac);
int wifi_drv_get_capa(void *iface, struct wifi_drv_capa *capa);
int wifi_drv_open(void *iface);
int wifi_drv_close(void *iface);
int wifi_drv_scan(void *iface, unsigned char band, unsigned char channel);
int wifi_drv_connect(void *iface, char *ssid, char *bssid, char *passwd,
			   unsigned char channel);
int wifi_drv_disconnect(void *iface);
int wifi_drv_get_station(void *iface, char *rssi);
int wifi_drv_notify_ip(void *iface, char *ipaddr, char len);
int wifi_drv_start_ap(void *iface, char *ssid, char *passwd,
			    unsigned char channel, unsigned char ch_width);
int wifi_drv_stop_ap(void *iface);
int wifi_drv_del_station(void *iface, char *mac);
int wifi_drv_set_mac_acl(void *iface, char subcmd, unsigned char acl_nr,
			 char acl_mac_addrs[][WIFIMGR_ETH_ALEN]);

int wifimgr_notify_event(unsigned int evt_id, void *buf, int buf_len);

#endif
