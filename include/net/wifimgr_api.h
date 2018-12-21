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
	int (*set_conf) (char *iface_name, char *ssid, char *bssid,
			 char *passphrase, unsigned char band,
			 unsigned char channel, unsigned char channel_width);
	int (*get_conf) (char *iface_name);
	int (*get_status) (char *iface_name);
	int (*open) (char *iface_name);
	int (*close) (char *iface_name);
	int (*scan) (void);
	int (*connect) (void);
	int (*disconnect) (void);
	int (*start_ap) (void);
	int (*stop_ap) (void);
	int (*del_station) (char *mac);
};

struct wifimgr_ctrl_cbs {
	void (*get_conf_cb) (char *iface_name, char *ssid, char *bssid,
			     char *passphrase, unsigned char band,
			     unsigned char channel);
	void (*get_sta_status_cb) (unsigned char status, char *own_mac,
				   char *host_ssid, char *host_bssid,
				   char host_channel, char host_rssi);
	void (*get_ap_status_cb) (unsigned char status, char *own_mac,
				  char client_nr, char client_mac[][6]);
	void (*notify_scan_res) (char *ssid, char *bssid, unsigned char band,
				 unsigned char channel, char signal);
	void (*notify_scan_done) (unsigned char result);
	void (*notify_connect) (unsigned char result);
	void (*notify_disconnect) (unsigned char reason);
	void (*notify_scan_timeout) (void);
	void (*notify_connect_timeout) (void);
	void (*notify_disconnect_timeout) (void);
	void (*notify_new_station) (unsigned char status, unsigned char *mac);
	void (*notify_del_station_timeout) (void);
};

const
struct wifimgr_ctrl_ops *wifimgr_get_ctrl_ops(struct wifimgr_ctrl_cbs *cbs);

#endif
