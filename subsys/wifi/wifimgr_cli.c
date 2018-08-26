/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <shell/shell.h>

#include "wifimgr.h"

#define WIFI_SHELL_MODULE "wifimgr"

#define WIFI_SHELL_MGMT_EVENTS (NET_EVENT_WIFI_SCAN_RESULT |		\
				NET_EVENT_WIFI_SCAN_DONE |		\
				NET_EVENT_WIFI_CONNECT_RESULT |		\
				NET_EVENT_WIFI_DISCONNECT_RESULT)

static struct net_mgmt_event_callback wifimgr_event_cb;

static void wifimgr_event_handler(struct net_mgmt_event_callback *cb,
				    u32_t mgmt_event, struct net_if *iface)
{
	/*switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		handle_wifi_scan_result(cb);
		break;
	case NET_EVENT_WIFI_SCAN_DONE:
		handle_wifi_scan_done(cb);
		break;
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		handle_wifi_disconnect_result(cb);
		break;
	default:
		break;
	}*/
}

static int wifimgr_cmd_get_config(int argc, char *argv[])
{
	char *iface_name = &argv[1][1];
	int ret = -1;

	if(!strncmp(iface_name, "sta", strlen("sta")))
		ret = wifimgr_ctrl_iface_open_sta();
	else if(!strncmp(iface_name, "ap", strlen("ap")))
		ret = wifimgr_ctrl_iface_open_ap();

	return ret;
}

static int wifimgr_cmd_status(int argc, char *argv[])
{
	return wifimgr_ctrl_iface_get_status();
}

static int wifimgr_cmd_open(int argc, char *argv[])
{
	char *iface_name = &argv[1][1];
	int ret = -1;

	if(!strncmp(iface_name, "sta", strlen("sta")))
		ret = wifimgr_ctrl_iface_open_sta();
	else if(!strncmp(iface_name, "ap", strlen("ap")))
		ret = wifimgr_ctrl_iface_open_ap();

	return ret;
}

static int wifimgr_cmd_close(int argc, char *argv[])
{
	return wifimgr_ctrl_iface_close_sta();
}

static int wifimgr_cmd_scan(int argc, char *argv[])
{
	return wifimgr_ctrl_iface_scan();
}

static int shell_cmd_connect(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params;
	int idx = 3;

	if (argc < 3) {
		return -EINVAL;
	}

	cnx_params.ssid_length = atoi(argv[2]);
	if (cnx_params.ssid_length <= 2) {
		return -EINVAL;
	}

	cnx_params.ssid = &argv[1][1];

	argv[1][cnx_params.ssid_length + 1] = '\0';

	if ((idx < argc) && (strlen(argv[idx]) <= 2)) {
		cnx_params.channel = atoi(argv[3]);
		if (cnx_params.channel == 0) {
			cnx_params.channel = WIFI_CHANNEL_ANY;
		}

		idx++;
	} else {
		cnx_params.channel = WIFI_CHANNEL_ANY;
	}

	if (idx < argc) {
		cnx_params.psk = argv[idx];
		cnx_params.psk_length = strlen(argv[idx]);
		cnx_params.security = WIFI_SECURITY_TYPE_PSK;
	} else {
		cnx_params.security = WIFI_SECURITY_TYPE_NONE;
	}

	if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface,
		     &cnx_params, sizeof(struct wifi_connect_req_params))) {
		printk("Connection request failed\n");
	} else {
		printk("Connection requested\n");
	}

	return 0;
}

static int shell_cmd_disconnect(int argc, char *argv[])
{
	return wifimgr_ctrl_iface_scan();
}

static struct shell_cmd wifimgr_commands[] = {
	/*{ "get_config",		wifimgr_cmd_get_config,
	  NULL },
	{ "set_config",		wifimgr_cmd_set_config,
	  NULL },*/
	{ "status",		wifimgr_cmd_status,
	  NULL },
	{ "open",		wifimgr_cmd_open,
	  "\"<interface, sta or ap>\"" },
	{ "close",		wifimgr_cmd_close,
	  "\"<interface, sta or ap>\"" },
	{ "scan",		wifimgr_cmd_scan,
	  NULL },
	{ "connect",		shell_cmd_connect,
	  "\"<SSID>\" <SSID length> <channel number (optional), 0 means all> "
	  "<PSK (optional: valid only for secured SSIDs)>" },
	{ "disconnect",		shell_cmd_disconnect,
	  NULL },
	{ NULL, NULL, NULL },
};

static int wifimgr_shell_init(struct device *unused)
{
	ARG_UNUSED(unused);

	net_mgmt_init_event_callback(&wifimgr_event_cb,
				     wifimgr_event_handler,
				     WIFI_SHELL_MGMT_EVENTS);

	net_mgmt_add_event_callback(&wifimgr_event_cb);

	SHELL_REGISTER(WIFI_SHELL_MODULE, wifimgr_commands);

	return 0;
}

#define WIFIMGR_CLI_PRIORITY	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT
SYS_INIT(wifimgr_shell_init, APPLICATION, WIFIMGR_CLI_PRIORITY);
