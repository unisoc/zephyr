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
	size_t size;
	int (*set_conf) (char *iface_name, char *ssid, char *bssid,
			 char *passphrase, unsigned char band,
			 unsigned char channel);
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
	size_t size;
	void (*get_conf_cb) (char *iface_name, char *ssid, char *bssid,
			     char *passphrase, unsigned char band,
			     unsigned char channel);
	void (*get_status_cb) (char *iface_name, unsigned char status,
			       char *own_mac, signed char signal);
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

int wifimgr_ctrl_iface_set_conf(char *iface_name, char *ssid, char *bssid,
				char *passphrase, unsigned char band,
				unsigned char channel);
int wifimgr_ctrl_iface_get_conf(char *iface_name);
int wifimgr_ctrl_iface_get_status(char *iface_name);
int wifimgr_ctrl_iface_open(char *iface_name);
int wifimgr_ctrl_iface_close(char *iface_name);
int wifimgr_ctrl_iface_scan(void);
int wifimgr_ctrl_iface_connect(void);
int wifimgr_ctrl_iface_disconnect(void);
int wifimgr_ctrl_iface_start_ap(void);
int wifimgr_ctrl_iface_stop_ap(void);
int wifimgr_ctrl_iface_del_station(char *mac);

#endif
