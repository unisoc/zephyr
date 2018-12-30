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

int wifi_drv_iface_get_mac(void *iface, char *mac);
int wifi_drv_iface_open_station(void *iface);
int wifi_drv_iface_close_station(void *iface);
int wifi_drv_iface_scan(void *iface, unsigned char band, unsigned char channel);
int wifi_drv_iface_connect(void *iface, char *ssid, char *bssid, char *passwd,
			   unsigned char channel);
int wifi_drv_iface_disconnect(void *iface);
int wifi_drv_iface_get_station(void *iface, char *signal);
int wifi_drv_iface_notify_ip(void *iface, char *ipaddr, char len);
int wifi_drv_iface_open_softap(void *iface);
int wifi_drv_iface_close_softap(void *iface);
int wifi_drv_iface_start_ap(void *iface, char *ssid, char *passwd,
			    unsigned char channel, unsigned char ch_width);
int wifi_drv_iface_stop_ap(void *iface);
int wifi_drv_iface_del_station(void *iface, char *mac);

int wifimgr_notify_event(unsigned int evt_id, void *buf, int buf_len);

#endif
