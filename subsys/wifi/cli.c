/*
 * @file
 * @brief A client to interact with WiFi manager
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_WIFIMGR_STA) || defined(CONFIG_WIFIMGR_AP)

#include <init.h>
#include <shell/shell.h>
#include <net/wifimgr_api.h>

#include "api.h"
#include "os_adapter.h"
#include "sm.h"

#if defined(CONFIG_WIFIMGR_STA) && defined(CONFIG_WIFIMGR_AP)
#define WIFIMGR_CMD_COMMON_HELP	"<iface: sta or ap>"
#elif defined(CONFIG_WIFIMGR_STA) && !defined(CONFIG_WIFIMGR_AP)
#define WIFIMGR_CMD_COMMON_HELP	"<iface: sta>"
#elif defined(CONFIG_WIFIMGR_STA) && !defined(CONFIG_WIFIMGR_AP)
#define WIFIMGR_CMD_COMMON_HELP	"<iface: ap>"
#else
#define WIFIMGR_CMD_COMMON_HELP	NULL
#endif

#ifdef CONFIG_WIFIMGR_STA
static
void wifimgr_cli_get_sta_conf_cb(char *ssid, char *bssid, char *passphrase,
				 unsigned char band, unsigned char channel,
				 enum wifimgr_security security, int autorun)
{
	printf("STA Config\n");
	if (ssid && strlen(ssid))
		printf("SSID:\t\t%s\n", ssid);
	if (bssid && !is_zero_ether_addr(bssid))
		printf("BSSID:\t\t" MACSTR "\n", MAC2STR(bssid));
	if (security)
		printf("Security:\t%s\n", security2str(security));
	if (passphrase && strlen(passphrase))
		printf("Passphrase:\t%s\n", passphrase);
	if (channel)
		printf("Channel:\t%u\n", channel);

	if (autorun)
		printf("----------------\n");
	if (autorun > 0)
		printf("Autorun:\t%ds\n", autorun);
	else if (autorun < 0)
		printf("Autorun:\toff\n");
}

static
void wifimgr_cli_get_sta_status_cb(char status, char *own_mac, char *host_bssid,
				   signed char host_rssi)
{
	printf("STA Status:\t%s\n", sta_sts2str(status));

	if (own_mac && !is_zero_ether_addr(own_mac))
		printf("own MAC:\t" MACSTR "\n", MAC2STR(own_mac));

	if (status == WIFIMGR_SM_STA_CONNECTED) {
		printf("----------------\n");
		if (host_bssid && !is_zero_ether_addr(host_bssid))
			wifimgr_info("Host BSSID:\t" MACSTR "\n",
				     MAC2STR(host_bssid));
		printf("Host RSSI:\t%d\n", host_rssi);
	}
}

static
void wifimgr_cli_notify_scan_res(char *ssid, char *bssid, unsigned char band,
				 unsigned char channel, signed char rssi,
				 enum wifimgr_security security)
{
	if (ssid && strlen(ssid))
		printf("\t%-32s", ssid);
	else
		printf("\t\t\t\t\t");

	if (bssid && !is_zero_ether_addr(bssid))
		printf("\t" MACSTR, MAC2STR(bssid));
	else
		printf("\t\t\t");

	printf("\t%s", security2str(security));
	printf("\t%u\t%d\n", channel, rssi);
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static
void wifimgr_cli_get_ap_conf_cb(char *ssid, char *passphrase,
				unsigned char band, unsigned char channel,
				unsigned char ch_width,
				enum wifimgr_security security, int autorun)
{
	printf("AP Config\n");
	if (ssid && strlen(ssid))
		printf("SSID:\t\t%s\n", ssid);
	if (security)
		printf("Security:\t%s\n", security2str(security));
	if (passphrase && strlen(passphrase))
		printf("Passphrase:\t%s\n", passphrase);
	if (channel)
		printf("Channel:\t%u\n", channel);
	if (ch_width)
		printf("Channel Width:\t%u\n", ch_width);

	if (autorun)
		printf("----------------\n");
	if (autorun > 0)
		printf("Autorun:\t%ds\n", autorun);
	else if (autorun < 0)
		printf("Autorun:\toff\n");
}

static
void wifimgr_cli_get_ap_capa_cb(unsigned char max_sta, unsigned char max_acl)
{
	printf("AP Capability\n");
	if (max_sta)
		printf("Max STA NR:\t%u\n", max_sta);
	if (max_acl)
		printf("Max ACL NR:\t%u\n", max_acl);
}

static
void wifimgr_cli_get_ap_status_cb(char status, char *own_mac,
				  unsigned char sta_nr,
				  char sta_mac_addrs[][6],
				  unsigned char acl_nr, char acl_mac_addrs[][6])
{
	printf("AP Status:\t%s\n", ap_sts2str(status));

	if (own_mac && !is_zero_ether_addr(own_mac))
		printf("BSSID:\t\t" MACSTR "\n", MAC2STR(own_mac));

	if (status == WIFIMGR_SM_AP_STARTED) {
		int i;
		char (*mac_addrs)[WIFIMGR_ETH_ALEN];

		printf("----------------\n");
		printf("STA NR:\t%u\n", sta_nr);
		if (sta_nr) {
			printf("STA:\n");
			mac_addrs = sta_mac_addrs;
			for (i = 0; i < sta_nr; i++)
				printf("\t\t" MACSTR "\n",
				       MAC2STR(mac_addrs[i]));
		}

		printf("----------------\n");
		printf("ACL NR:\t%u\n", acl_nr);
		if (acl_nr) {
			printf("ACL:\n");
			mac_addrs = acl_mac_addrs;
			for (i = 0; i < acl_nr; i++)
				printf("\t\t" MACSTR "\n",
				       MAC2STR(mac_addrs[i]));
		}
	}
}
#endif

static struct wifimgr_ctrl_cbs wifimgr_cli_cbs = {
#ifdef CONFIG_WIFIMGR_STA
	.get_sta_conf_cb = wifimgr_cli_get_sta_conf_cb,
	.get_sta_status_cb = wifimgr_cli_get_sta_status_cb,
	.notify_scan_res = wifimgr_cli_notify_scan_res,
#endif
#ifdef CONFIG_WIFIMGR_AP
	.get_ap_conf_cb = wifimgr_cli_get_ap_conf_cb,
	.get_ap_capa_cb = wifimgr_cli_get_ap_capa_cb,
	.get_ap_status_cb = wifimgr_cli_get_ap_status_cb,
#endif
};

static int strtomac(char *mac_str, char *mac_addr)
{
	char *mac;
	int i;

	mac = strtok(mac_str, ":");

	for (i = 0; i < WIFIMGR_ETH_ALEN; i++) {
		char *tail;

		mac_addr[i] = strtol(mac, &tail, 16);
		mac = strtok(NULL, ":");

		if (!mac)
			break;
	}

	if (i != (WIFIMGR_ETH_ALEN - 1))
		return -EINVAL;

	return 0;
}

static int wifimgr_cmd_set_config(const struct shell *shell, size_t argc,
				  char *argv[])
{
	char *iface_name = NULL;
	char *bssid = NULL;
	char *ssid = NULL;
	char *passphrase = NULL;
	char security = 0;
	int autorun = 0;
	unsigned char band = 0;
	unsigned char channel = 0;
	unsigned char ch_width = 0;
	int choice;

	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->set_conf)
		return -EOPNOTSUPP;

	if (!argv[1])
		return -EINVAL;
	iface_name = argv[1];

	optind = 0;
	while ((choice = getopt(argc, argv, "a:b:c:m:n:p:w:")) != -1) {
		switch (choice) {
		case 'a':
			autorun = atoi(optarg);
			break;
		case 'b':
			band = atoi(optarg);
			break;
		case 'c':
			channel = atoi(optarg);
			break;
		case 'm':
			if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
				char mac_addr[WIFIMGR_ETH_ALEN];
				char *mac;
				int ret = strtomac(optarg, mac_addr);

				if (!ret) {
					mac = mac_addr;
				} else {
					printf("invalid BSSID!\n");
					return ret;
				}
				bssid = mac_addr;
			} else {
				printf("invalid option '-%c' for '%s'\n",
				       choice, iface_name);
				return -EINVAL;
			}
			break;
		case 'n':
			ssid = optarg;
			break;
		case 'p':
			passphrase = optarg;
			if (strlen(passphrase))
				security = WIFIMGR_SECURITY_PSK;
			else
				security = WIFIMGR_SECURITY_OPEN;
			break;
		case 'w':
			if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
				ch_width = atoi(optarg);
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

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->set_conf(iface_name,
								ssid, bssid,
								security,
								passphrase,
								band, channel,
								ch_width,
								autorun);
}

static int wifimgr_cmd_clear_config(const struct shell *shell, size_t argc,
				    char *argv[])
{
	char *iface_name;

	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->clear_conf)
		return -EOPNOTSUPP;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->clear_conf(iface_name);
}

static int wifimgr_cmd_get_config(const struct shell *shell, size_t argc,
				  char *argv[])
{
	char *iface_name;

	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->get_conf)
		return -EOPNOTSUPP;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->get_conf(iface_name);
}

static int wifimgr_cmd_capa(const struct shell *shell, size_t argc,
			    char *argv[])
{
	char *iface_name;

	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->get_capa)
		return -EOPNOTSUPP;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->get_capa(iface_name);
}

static int wifimgr_cmd_status(const struct shell *shell, size_t argc,
			      char *argv[])
{
	char *iface_name;

	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->get_status)
		return -EOPNOTSUPP;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->get_status(iface_name);
}

static int wifimgr_cmd_open(const struct shell *shell, size_t argc,
			    char *argv[])
{
	char *iface_name;

	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->open)
		return -EOPNOTSUPP;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->open(iface_name);
}

static int wifimgr_cmd_close(const struct shell *shell, size_t argc,
			     char *argv[])
{
	char *iface_name;

	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->close)
		return -EOPNOTSUPP;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->close(iface_name);
}
#endif

#ifdef CONFIG_WIFIMGR_STA
static int wifimgr_cmd_scan(const struct shell *shell, size_t argc,
			    char *argv[])
{
	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->scan)
		return -EOPNOTSUPP;

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->scan();
}

static int wifimgr_cmd_connect(const struct shell *shell, size_t argc,
			       char *argv[])
{
	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->connect)
		return -EOPNOTSUPP;

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->connect();
}

static int wifimgr_cmd_disconnect(const struct shell *shell, size_t argc,
				  char *argv[])
{
	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->disconnect)
		return -EOPNOTSUPP;

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->disconnect();
}

#ifdef CONFIG_WIFIMGR_AP
static int wifimgr_cmd_start_ap(const struct shell *shell, size_t argc,
				char *argv[])
{
	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->start_ap)
		return -EOPNOTSUPP;

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->start_ap();
}

static int wifimgr_cmd_stop_ap(const struct shell *shell, size_t argc,
			       char *argv[])
{
	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->stop_ap)
		return -EOPNOTSUPP;

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->stop_ap();
}

static int wifimgr_cmd_set_mac_acl(const struct shell *shell, size_t argc,
				   char *argv[])
{
	int choice;
	char subcmd = 0;
	char *mac = NULL;

	if (!wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->set_mac_acl)
		return -EOPNOTSUPP;

	optind = 0;
	while ((choice = getopt(argc, argv, "ab:cu:")) != -1) {
		switch (choice) {
		case 'a':
		case 'b':
		case 'c':
		case 'u':
			switch (choice) {
			case 'a':
				subcmd = WIFIMGR_SUBCMD_ACL_BLOCK_ALL;
				break;
			case 'b':
				subcmd = WIFIMGR_SUBCMD_ACL_BLOCK;
				break;
			case 'c':
				subcmd = WIFIMGR_SUBCMD_ACL_UNBLOCK_ALL;
				break;
			case 'u':
				subcmd = WIFIMGR_SUBCMD_ACL_UNBLOCK;
				break;
			}
			if (!optarg) {
				mac = NULL;
			} else {
				char mac_addr[WIFIMGR_ETH_ALEN];
				int ret = strtomac(optarg, mac_addr);

				if (!ret) {
					mac = mac_addr;
				} else {
					printf("invalid MAC address!\n");
					return ret;
				}
			}
			break;
		default:
			return -EINVAL;
		}
	}

	return wifimgr_get_ctrl_ops(&wifimgr_cli_cbs)->set_mac_acl(subcmd, mac);
}
#endif

SHELL_CREATE_STATIC_SUBCMD_SET(wifimgr_commands) {
	SHELL_CMD(get_config, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cmd_get_config),
	SHELL_CMD(set_config, NULL,
#ifdef CONFIG_WIFIMGR_STA
	 "<sta> -n <SSID> -m <BSSID> -c <channel>"
	 "\n<sta> -p <passphrase (\"\" for OPEN)>"
	 "\n<sta> -a <autorun interval sec (<0: disable)>"
#endif
#ifdef CONFIG_WIFIMGR_AP
	 "\n<ap> -n <SSID> -c <channel> -w <channel_width>"
	 "\n<ap> -p <passphrase (\"\" for OPEN)>"
	 "\n<ap> -a <autorun interval sec (<0: disable)>"
#endif
	 ,
	 wifimgr_cmd_set_config),
	SHELL_CMD(clear_config, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cmd_clear_config),
	SHELL_CMD(capa, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cmd_capa),
	SHELL_CMD(status, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cmd_status),
	SHELL_CMD(open, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cmd_open),
	SHELL_CMD(close, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cmd_close),
#ifdef CONFIG_WIFIMGR_STA
	SHELL_CMD(scan, NULL,
	 "<band (optional)> <channel (optional)>",
	 wifimgr_cmd_scan),
	SHELL_CMD(connect, NULL,
	 NULL,
	 wifimgr_cmd_connect),
	SHELL_CMD(disconnect, NULL,
	 NULL,
	 wifimgr_cmd_disconnect),
#endif
#ifdef CONFIG_WIFIMGR_AP
	SHELL_CMD(start_ap, NULL,
	 NULL,
	 wifimgr_cmd_start_ap),
	SHELL_CMD(stop_ap, NULL,
	 NULL,
	 wifimgr_cmd_stop_ap),
	SHELL_CMD(mac_acl, NULL,
	 "-a (block all connected stations)"
	 "\n-b <MAC address (to be unblocked)>"
	 "\n-c (clear all blocked stations)"
	 "\n-u <MAC address (to be unblocked)>",
	 wifimgr_cmd_set_mac_acl),
#endif
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(wifimgr, &wifimgr_commands, "WiFi Manager commands", NULL);
#endif
