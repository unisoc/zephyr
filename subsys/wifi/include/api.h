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

#include "notifier.h"
#include "os_adapter.h"

struct wifimgr_set_mac_acl {
#define WIFIMGR_SUBCMD_ACL_BLOCK	(1)
#define WIFIMGR_SUBCMD_ACL_UNBLOCK	(2)
#define WIFIMGR_SUBCMD_ACL_BLOCK_ALL	(3)
#define WIFIMGR_SUBCMD_ACL_UNBLOCK_ALL	(4)
	char subcmd;
	char mac[WIFIMGR_ETH_ALEN];
};

enum wifimgr_cmd {
	/*STA Common command */
	WIFIMGR_CMD_SET_STA_CONFIG,
	WIFIMGR_CMD_GET_STA_CONFIG,
	WIFIMGR_CMD_GET_STA_CAPA,
	WIFIMGR_CMD_GET_STA_STATUS,
	/*STA command */
	WIFIMGR_CMD_OPEN_STA,
	WIFIMGR_CMD_CLOSE_STA,
	WIFIMGR_CMD_STA_SCAN,
	WIFIMGR_CMD_RTT_REQ,
	WIFIMGR_CMD_CONNECT,
	WIFIMGR_CMD_DISCONNECT,
	/*AP Common command */
	WIFIMGR_CMD_GET_AP_CONFIG,
	WIFIMGR_CMD_SET_AP_CONFIG,
	WIFIMGR_CMD_GET_AP_CAPA,
	WIFIMGR_CMD_GET_AP_STATUS,
	/*AP command */
	WIFIMGR_CMD_OPEN_AP,
	WIFIMGR_CMD_CLOSE_AP,
	WIFIMGR_CMD_AP_SCAN,
	WIFIMGR_CMD_START_AP,
	WIFIMGR_CMD_STOP_AP,
	WIFIMGR_CMD_DEL_STA,
	WIFIMGR_CMD_SET_MAC_ACL,

	WIFIMGR_CMD_MAX,
};

typedef void (*scan_done_cb_t)(char status);
typedef void (*rtt_done_cb_t)(char status);

static inline const char *security2str(int security)
{
	char *str = NULL;

	switch (security) {
	case WIFIMGR_SECURITY_UNKNOWN:
		str = "UNKNOWN\t";
		break;
	case WIFIMGR_SECURITY_OPEN:
		str = "OPEN\t";
		break;
	case WIFIMGR_SECURITY_PSK:
		str = "WPA/WPA2";
		break;
	case WIFIMGR_SECURITY_OTHERS:
		str = "OTHERS\t";
		break;
	default:
		break;
	}
	return str;
}

#endif
