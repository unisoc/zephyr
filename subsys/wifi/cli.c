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

#include "wifimgr.h"

#define WIFI_SHELL_MODULE "wifimgr"
#define WIFIMGR_CLI_PRIORITY	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT

static int wifimgr_cmd_set_config(const struct shell *shell, size_t argc, char *argv[])
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

static int wifimgr_cmd_get_config(const struct shell *shell, size_t argc, char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_get_conf(iface_name);
}

static int wifimgr_cmd_status(const struct shell *shell, size_t argc, char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_get_status(iface_name);
}

static int wifimgr_cmd_open(const struct shell *shell, size_t argc, char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_open(iface_name);
}

static int wifimgr_cmd_close(const struct shell *shell, size_t argc, char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_close(iface_name);
}

static int wifimgr_cmd_scan(const struct shell *shell, size_t argc, char *argv[])
{
	return wifimgr_ctrl_iface_scan();
}

static int wifimgr_cmd_connect(const struct shell *shell, size_t argc, char *argv[])
{
	return wifimgr_ctrl_iface_connect();
}

static int wifimgr_cmd_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	return wifimgr_ctrl_iface_disconnect();
}

static int wifimgr_cmd_start_ap(const struct shell *shell, size_t argc, char *argv[])
{
	return wifimgr_ctrl_iface_start_ap();
}

static int wifimgr_cmd_stop_ap(const struct shell *shell, size_t argc, char *argv[])
{
	return wifimgr_ctrl_iface_stop_ap();
}

static int wifimgr_cmd_del_station(const struct shell *shell, size_t argc, char *argv[])
{
	char *mac = NULL;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	mac = argv[1];

	return wifimgr_ctrl_iface_del_station(mac);
}

SHELL_CREATE_STATIC_SUBCMD_SET(wifimgr_commands) {
	SHELL_CMD(set_config, NULL,
	 "<sta> <SSID> <PSK (optional: valid only for secured SSIDs)>"
	 "\n\t\t\t     <ap> <SSID> <channel> <PSK (optional: valid only for secured SSIDs)>",
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
	 "<MAC address>",
	 wifimgr_cmd_del_station),
	SHELL_SUBCMD_SET_END
};

static int cmd_wifimgr(const struct shell *shell, size_t argc, char **argv)
{
	int err;

	if (argc == 1) {
		shell_help_print(shell, NULL, 0);
		/* shell_cmd_precheck returns 1 when help is printed */
		return 1;
	}

	err = shell_cmd_precheck(shell, (argc == 2), NULL, 0);
	if (err) {
		return err;
	}

	syslog(LOG_ERR, "%s: unknown parameter: %s\n", argv[0] , argv[1]);
	return -EINVAL;
}

SHELL_CMD_REGISTER(wifimgr, &wifimgr_commands, "WiFi Manager commands", cmd_wifimgr);

/*static const struct shell_cmd wifimgr_commands[] = {
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
};*/

/*static int wifimgr_shell_init(struct device *unused)
{
	ARG_UNUSED(unused);

	SHELL_CMD_REGISTER(WIFI_SHELL_MODULE, wifimgr_commands);

	return 0;
}

SYS_INIT(wifimgr_shell_init, APPLICATION, WIFIMGR_CLI_PRIORITY);*/
