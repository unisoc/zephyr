/*
 * @file
 * @brief WiFi manager APIs for the external caller
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_API_H_
#define _WIFIMGR_API_H_

#define WIFIMGR_IFACE_NAME_STA	"sta"
#define WIFIMGR_IFACE_NAME_AP	"ap"

struct wifimgr_ctrl_ops {
	int (*set_conf)(char *iface_name, char *ssid, char *bssid,
			char *passphrase, unsigned char band,
			unsigned char channel, unsigned char channel_width,
			char autorun);
	int (*get_conf)(char *iface_name);
	int (*get_capa)(char *iface_name);
	int (*get_status)(char *iface_name);
	int (*open)(char *iface_name);
	int (*close)(char *iface_name);
	int (*scan)(void);
	int (*connect)(void);
	int (*disconnect)(void);
	int (*start_ap)(void);
	int (*stop_ap)(void);
	int (*set_mac_acl)(char subcmd, char *mac);
};

struct wifimgr_ctrl_cbs {
	void (*get_sta_conf_cb)(char *ssid, char *bssid, char *passphrase,
				unsigned char band, unsigned char channel,
				char autorun);
	void (*get_ap_conf_cb)(char *ssid, char *passphrase,
			       unsigned char band, unsigned char channel,
			       unsigned char channel_width, char autorun);
	void (*get_ap_capa_cb)(unsigned char max_sta, unsigned char max_acl);
	void (*get_sta_status_cb)(unsigned char status, char *own_mac,
				  char *host_ssid, char *host_bssid,
				  char host_channel, signed char host_rssi);
	void (*get_ap_status_cb)(unsigned char status, char *own_mac,
				 unsigned char sta_nr, char sta_mac_addrs[][6],
				 unsigned char acl_nr, char acl_mac_addrs[][6]);
	void (*notify_scan_res)(char *ssid, char *bssid, unsigned char band,
				unsigned char channel, signed char rssi);
	void (*notify_scan_done)(char result);
	void (*notify_connect)(char result);
	void (*notify_disconnect)(char reason);
	void (*notify_scan_timeout)(void);
	void (*notify_connect_timeout)(void);
	void (*notify_disconnect_timeout)(void);
	void (*notify_new_station)(char status, char *mac);
	void (*notify_set_mac_acl)(char result);
	void (*notify_set_mac_acl_timeout)(void);
};

const
struct wifimgr_ctrl_ops *wifimgr_get_ctrl_ops(struct wifimgr_ctrl_cbs *cbs);

#endif
