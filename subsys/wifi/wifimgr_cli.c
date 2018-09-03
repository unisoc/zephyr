/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <shell/shell.h>

#include "wifimgr.h"

#define WIFI_SHELL_MODULE "wifimgr"
#define WIFIMGR_CLI_PRIORITY	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT

static int wifimgr_cmd_set_config(int argc, char *argv[])
{
	char *iface_name = NULL;
	char *ssid = NULL;
	char *passphrase = NULL;
	unsigned char channel = 0;

	if (!argv[1])
		return -EINVAL;
	iface_name = argv[1];

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		if (argc < 3 || argc > 4)
			return -EINVAL;

		if (!argv[2])
			return -EINVAL;
		ssid = argv[2];

		if (argv[3])
			passphrase = argv[3];
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		if (argc < 4 || argc > 5)
			return -EINVAL;

		if (!argv[2])
			return -EINVAL;
		ssid = argv[2];

		if (!argv[3])
			return -EINVAL;
		channel = atoi(argv[3]);

		if (argv[4])
			passphrase = argv[4];
	}

	return wifimgr_ctrl_iface_set_conf(iface_name, ssid, NULL, passphrase,
					   0, channel);
}

static int wifimgr_cmd_get_config(int argc, char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_get_conf(iface_name);
}

static int wifimgr_cmd_status(int argc, char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_get_status(iface_name);
}

static int wifimgr_cmd_open(int argc, char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_open(iface_name);
}

static int wifimgr_cmd_close(int argc, char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_close(iface_name);
}

static int wifimgr_cmd_scan(int argc, char *argv[])
{
	return wifimgr_ctrl_iface_scan();
}

static int wifimgr_cmd_connect(int argc, char *argv[])
{
	return wifimgr_ctrl_iface_connect();
}

static int wifimgr_cmd_disconnect(int argc, char *argv[])
{
	return wifimgr_ctrl_iface_disconnect();
}

static int wifimgr_cmd_start_ap(int argc, char *argv[])
{
	return wifimgr_ctrl_iface_start_ap();
}

static int wifimgr_cmd_stop_ap(int argc, char *argv[])
{
	return wifimgr_ctrl_iface_stop_ap();
}

static int wifimgr_cmd_del_station(int argc, char *argv[])
{
	char *mac = NULL;

	if (argv[1])
		mac = argv[1];

	return wifimgr_ctrl_iface_del_station(mac);
}

static struct shell_cmd wifimgr_commands[] = {
	{"set_config", wifimgr_cmd_set_config,
	 "<sta> <SSID> <PSK (optional: valid only for secured SSIDs)>"
	 "\n\t\t\t     <ap> <SSID> <channel> <PSK (optional: valid only for secured SSIDs)>"},
	{"get_config", wifimgr_cmd_get_config,
	 "<iface, sta or ap>"},
	{"status", wifimgr_cmd_status,
	 "<iface, sta or ap>"},
	{"open", wifimgr_cmd_open,
	 "<iface, sta or ap>"},
	{"close", wifimgr_cmd_close,
	 "<iface, sta or ap>"},
	{"scan", wifimgr_cmd_scan,
	 NULL},
	{"connect", wifimgr_cmd_connect,
	 NULL},
	{"disconnect", wifimgr_cmd_disconnect,
	 NULL},
	{"start_ap", wifimgr_cmd_start_ap,
	 NULL},
	{"stop_ap", wifimgr_cmd_stop_ap,
	 NULL},
	{"del_station", wifimgr_cmd_del_station,
	 "<MAC address>"},
	{NULL, NULL, NULL},
};

static int wifimgr_shell_init(struct device *unused)
{
	ARG_UNUSED(unused);

	SHELL_REGISTER(WIFI_SHELL_MODULE, wifimgr_commands);

	return 0;
}

SYS_INIT(wifimgr_shell_init, APPLICATION, WIFIMGR_CLI_PRIORITY);
