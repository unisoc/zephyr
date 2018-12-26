/*
 * @file
 * @brief A client to interact with WiFi manager
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <shell/shell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/net_linkaddr.h>
#include <net/wifimgr_api.h>

#include "api.h"

static int strtomac(char *mac_string, char *mac_addr)
{
	char *mac;
	int ret = 0;
	int i;

	mac = strtok(mac_string, ":");

	for (i = 0; i < NET_LINK_ADDR_MAX_LENGTH; i++) {
		char *tail;

		mac_addr[i] = strtol(mac, &tail, 16);
		mac = strtok(NULL, ":");

		if (!mac)
			break;
	}

	if (i != (NET_LINK_ADDR_MAX_LENGTH - 1))
		ret = -EINVAL;

	return ret;
}

static int wifimgr_cmd_set_config(const struct shell *shell, size_t argc,
				  char *argv[])
{
	char *iface_name = NULL;
	char *bssid = NULL;
	char *ssid = NULL;
	char *passphrase = NULL;
	unsigned char band = 0;
	unsigned char channel = 0;
	unsigned char channel_width = 0;
	int choice;

	if (!argv[1])
		return -EINVAL;
	iface_name = argv[1];

	optind = 0;
	while ((choice = getopt(argc, argv, "b:c:m:n:p:w:")) != -1) {
		switch (choice) {
		case 'n':
			ssid = optarg;
			break;
		case 'm':
			if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
				char mac_addr[NET_LINK_ADDR_MAX_LENGTH];
				char *mac;
				int ret = strtomac(optarg, mac_addr);

				if (!ret) {
					mac = mac_addr;
				} else {
					printf("wrong host BSSID!\n");
					return ret;
				}
				bssid = mac_addr;
			} else {
				printf("invalid option '-%c' for '%s'\n",
				       choice, iface_name);
				return -EINVAL;
			}
			break;
		case 'p':
			passphrase = optarg;
			break;
		case 'b':
			band = atoi(optarg);
			break;
		case 'c':
			channel = atoi(optarg);
			break;
		case 'w':
			if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
				channel_width = atoi(optarg);
			} else {
				printf("invalid option '-%c' for '%s'\n",
				       choice, iface_name);
				return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
		}
	}

	return wifimgr_ctrl_iface_set_conf(iface_name, ssid, bssid, passphrase,
					   band, channel, channel_width);
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
	char *mac;

	if (argc == 1) {
		mac = NULL;
	} else if (argc == 2 && argv[1]) {
		char mac_addr[NET_LINK_ADDR_MAX_LENGTH];
		int ret = strtomac(argv[1], mac_addr);

		if (!ret) {
			mac = mac_addr;
		} else {
			printf("wrong client MAC!\n");
			return ret;
		}
	} else {
		return -EINVAL;
	}

	return wifimgr_ctrl_iface_del_station(mac);
}

SHELL_CREATE_STATIC_SUBCMD_SET(wifimgr_commands) {
	SHELL_CMD(set_config, NULL,
	 "<sta> -n <SSID> -m <BSSID> -c <channel> <PSK (WPA/WPA2)>"
	 "\n<sta> (clear STA config)"
	 "\n<ap> -n <SSID> -c <channel> -w <channel_width> <PSK (WPA/WPA2)>"
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
