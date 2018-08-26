/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_API_H_
#define _WIFIMGR_API_H_

struct wifimgr_ctrl_ops {
	size_t size;
	int (*set_conf) (char *ssid, char *bssid, char *passphrase,
			 unsigned char band, unsigned char channel);
	int (*get_conf) (void);
	int (*get_status) (void);
	int (*open_sta) (void);
	int (*close_sta) (void);
	int (*scan) (void);
	int (*connect) (void);
	int (*disconnect) (void);
};

struct wifimgr_ctrl_cbs {
	size_t size;
	void (*get_conf_cb) (char *ssid, char *bssid, char *passphrase,
			     unsigned char band, unsigned char channel);
	void (*get_status_cb) (int status, char *ssid, char *bssid,
			       unsigned char band, unsigned char channel,
			       signed char signal, unsigned int ip_addr);
	void (*notify_scan_res) (char *ssid, char *bssid, unsigned char band,
				 unsigned char channel, signed char signal);
	void (*notify_connect) (unsigned char *result);
	void (*notify_disconnect) (unsigned char *reason);
	void (*notify_scan_timeout) (void);
	void (*notify_connect_timeout) (void);
	void (*notify_disconnect_timeout) (void);
	void (*notify_new_station) (unsigned char status, unsigned char *mac);
	void (*notify_del_station_timeout) (void);
};

const struct wifimgr_ctrl_ops *wifimgr_get_ctrl_ops(void);
const struct wifimgr_ctrl_cbs *wifimgr_get_ctrl_cbs(void);

int wifimgr_ctrl_iface_set_conf(char *ssid, char *bssid,
				       char *passphrase, unsigned char band,
				       unsigned char channel);
int wifimgr_ctrl_iface_get_conf(void);
int wifimgr_ctrl_iface_get_status(void);
int wifimgr_ctrl_iface_open_sta(void);
int wifimgr_ctrl_iface_close_sta(void);
int wifimgr_ctrl_iface_scan(void);
int wifimgr_ctrl_iface_connect(void);
int wifimgr_ctrl_iface_disconnect(void);
int wifimgr_ctrl_iface_open_ap(void);
int wifimgr_ctrl_iface_close_ap(void);

#endif
