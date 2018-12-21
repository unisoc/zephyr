/*
 * @file
 * @brief A client to interact with WiFi manager
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#include <init.h>
#include <shell/shell.h>

#include "wifimgr.h"

static int wifimgr_cmd_set_config(const struct shell *shell, size_t argc,
				  char *argv[])
{
	char *iface_name = NULL;
	char *ssid = NULL;
	char *passphrase = NULL;
	unsigned char channel = 0;

	if (!argv[1])
		return -EINVAL;
	iface_name = argv[1];

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		if ((argc >= 3) && argv[2])
			ssid = argv[2];

		if ((argc == 4) && argv[3])
			passphrase = argv[3];
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		if ((argc >= 3) && argv[2])
			ssid = argv[2];

		if ((argc >= 4) && argv[3])
			channel = atoi(argv[3]);

		if ((argc == 5) && argv[4])
			passphrase = argv[4];
	}

	return wifimgr_ctrl_iface_set_conf(iface_name, ssid, NULL, passphrase,
					   0, channel, 0);
}

static int wifimgr_cmd_get_config(const struct shell *shell, size_t argc,
				  char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_get_conf(iface_name);
}

static int wifimgr_cmd_status(const struct shell *shell, size_t argc,
			      char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_get_status(iface_name);
}

static int wifimgr_cmd_open(const struct shell *shell, size_t argc,
			    char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_open(iface_name);
}

static int wifimgr_cmd_close(const struct shell *shell, size_t argc,
			     char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_close(iface_name);
}

static int wifimgr_cmd_scan(const struct shell *shell, size_t argc,
			    char *argv[])
{
	return wifimgr_ctrl_iface_scan();
}

static int wifimgr_cmd_connect(const struct shell *shell, size_t argc,
			       char *argv[])
{
	return wifimgr_ctrl_iface_connect();
}

static int wifimgr_cmd_disconnect(const struct shell *shell, size_t argc,
				  char *argv[])
{
	return wifimgr_ctrl_iface_disconnect();
}

static int wifimgr_cmd_start_ap(const struct shell *shell, size_t argc,
				char *argv[])
{
	return wifimgr_ctrl_iface_start_ap();
}

static int wifimgr_cmd_stop_ap(const struct shell *shell, size_t argc,
			       char *argv[])
{
	return wifimgr_ctrl_iface_stop_ap();
}

static int wifimgr_cmd_del_station(const struct shell *shell, size_t argc,
				   char *argv[])
{
	char mac_addr[WIFIMGR_ETH_ALEN];
	char *mac;

	if (argc == 1) {
		mac = NULL;
	} else if (argc == 2 && argv[1]) {
		char *mac_string = argv[1];
		int i;

		mac = strtok(mac_string, ":");

		for (i = 0; i < WIFIMGR_ETH_ALEN; i++) {
			char *tail;

			mac_addr[i] = strtol(mac, &tail, 16);
			mac = strtok(NULL, ":");

			if (!mac)
				break;
		}

		if (i != (WIFIMGR_ETH_ALEN - 1))
			return -EINVAL;
		mac = mac_addr;
	} else {
		return -EINVAL;
	}

	return wifimgr_ctrl_iface_del_station(mac);
}

SHELL_CREATE_STATIC_SUBCMD_SET(wifimgr_commands) {
	SHELL_CMD(set_config, NULL,
	 "<sta> <SSID> <PSK (optional: valid only for secured SSIDs)>"
	 "\n<sta> (clear STA config)"
	 "\n<ap> <SSID> <channel> <PSK (optional: valid only for secured SSIDs)>"
	 "\n<ap> (clear AP config)",
	 wifimgr_cmd_set_config),
	SHELL_CMD(get_config, NULL,
	 "<iface, sta or ap>",
	 wifimgr_cmd_get_config),
	SHELL_CMD(status, NULL,
	 "<iface, sta or ap>",
	 wifimgr_cmd_status),
	SHELL_CMD(open, NULL,
	 "<iface, sta or ap>",
	 wifimgr_cmd_open),
	SHELL_CMD(close, NULL,
	 "<iface, sta or ap>",
	 wifimgr_cmd_close),
	SHELL_CMD(scan, NULL,
	 "<band (optional)> <channel (optional)>",
	 wifimgr_cmd_scan),
	SHELL_CMD(connect, NULL,
	 NULL,
	 wifimgr_cmd_connect),
	SHELL_CMD(disconnect, NULL,
	 NULL,
	 wifimgr_cmd_disconnect),
	SHELL_CMD(start_ap, NULL,
	 NULL,
	 wifimgr_cmd_start_ap),
	SHELL_CMD(stop_ap, NULL,
	 NULL,
	 wifimgr_cmd_stop_ap),
	SHELL_CMD(del_station, NULL,
	 "<MAC address (empty for all stations)>",
	 wifimgr_cmd_del_station),
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(wifimgr, &wifimgr_commands, "WiFi Manager commands", NULL);
