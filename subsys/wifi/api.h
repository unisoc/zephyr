/*
 * @file
 * @brief Internal API definitions
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_INTERNAL_API_H_
#define _WIFIMGR_INTERNAL_API_H_

#include "os_adapter.h"

#define WIFIMGR_MAX_SSID_LEN	32
#define WIFIMGR_MAX_PSPHR_LEN	63

struct wifimgr_config {
	char ssid[WIFIMGR_MAX_SSID_LEN + 1];
	char bssid[WIFIMGR_ETH_ALEN];
	char security;
	char passphrase[WIFIMGR_MAX_PSPHR_LEN + 1];
	unsigned char band;
	unsigned char channel;
	unsigned char ch_width;
	char autorun;
};

struct wifimgr_set_mac_acl {
#define WIFIMGR_SUBCMD_ACL_BLOCK	(1)
#define WIFIMGR_SUBCMD_ACL_UNBLOCK	(2)
#define WIFIMGR_SUBCMD_ACL_BLOCK_ALL	(3)
#define WIFIMGR_SUBCMD_ACL_UNBLOCK_ALL	(4)
	char subcmd;
	char mac[WIFIMGR_ETH_ALEN];
};

enum wifimgr_cmd {
	/*Common command */
	WIFIMGR_CMD_GET_STA_CONFIG,
	WIFIMGR_CMD_SET_STA_CONFIG,
	WIFIMGR_CMD_GET_STA_STATUS,
	WIFIMGR_CMD_GET_STA_CAPA,
	WIFIMGR_CMD_GET_AP_CONFIG,
	WIFIMGR_CMD_SET_AP_CONFIG,
	WIFIMGR_CMD_GET_AP_STATUS,
	WIFIMGR_CMD_GET_AP_CAPA,
	/*STA command */
	WIFIMGR_CMD_OPEN_STA,
	WIFIMGR_CMD_CLOSE_STA,
	WIFIMGR_CMD_SCAN,
	WIFIMGR_CMD_CONNECT,
	WIFIMGR_CMD_DISCONNECT,
	/*AP command */
	WIFIMGR_CMD_OPEN_AP,
	WIFIMGR_CMD_CLOSE_AP,
	WIFIMGR_CMD_START_AP,
	WIFIMGR_CMD_STOP_AP,
	WIFIMGR_CMD_DEL_STA,
	WIFIMGR_CMD_SET_MAC_ACL,

	WIFIMGR_CMD_MAX,
};

int wifimgr_ctrl_iface_send_cmd(unsigned int cmd_id, void *buf, int buf_len);

int wifimgr_ctrl_iface_set_conf(char *iface_name, char *ssid, char *bssid,
				char *passphrase, unsigned char band,
				unsigned char channel, unsigned char ch_width,
				char autorun);
int wifimgr_ctrl_iface_get_conf(char *iface_name);
int wifimgr_ctrl_iface_get_capa(char *iface_name);
int wifimgr_ctrl_iface_get_status(char *iface_name);
int wifimgr_ctrl_iface_open(char *iface_name);
int wifimgr_ctrl_iface_close(char *iface_name);
int wifimgr_ctrl_iface_scan(void);
int wifimgr_ctrl_iface_connect(void);
int wifimgr_ctrl_iface_disconnect(void);
int wifimgr_ctrl_iface_start_ap(void);
int wifimgr_ctrl_iface_stop_ap(void);
int wifimgr_ctrl_iface_set_mac_acl(char subcmd, char *mac);

struct wifimgr_ctrl_cbs *wifimgr_get_ctrl_cbs(void);

#endif
