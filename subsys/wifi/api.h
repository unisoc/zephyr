/*
 * @file
 * @brief Internal API definitions
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_API_INTERNAL_H_
#define _WIFIMGR_API_INTERNAL_H_

int wifimgr_ctrl_iface_set_conf(char *iface_name, char *ssid, char *bssid,
				char *passphrase, unsigned char band,
				unsigned char channel, unsigned char ch_width,
				char autorun);
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

struct wifimgr_ctrl_cbs *wifimgr_get_ctrl_cbs(void);

#endif
