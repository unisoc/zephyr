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

static void wifimgr_cli_show_conf(char *iface_name, struct wifi_config *conf)
{
	if (!memiszero(conf, sizeof(struct wifi_config))) {
		printf("No config found!\n");
		return;
	}

	if (conf->ssid && strlen(conf->ssid))
		printf("SSID:\t\t%s\n", conf->ssid);
	if (conf->bssid && !is_zero_ether_addr(conf->bssid))
		printf("BSSID:\t\t" MACSTR "\n", MAC2STR(conf->bssid));

	if (conf->security)
		printf("Security:\t%s\n", security2str(conf->security));
	if (conf->passphrase && strlen(conf->passphrase))
		printf("Passphrase:\t%s\n", conf->passphrase);

	if (conf->band)
		printf("Band:\t%u\n", conf->band);
	if (conf->channel)
		printf("Channel:\t%u\n", conf->channel);
	if (conf->ch_width)
		printf("Channel Width:\t%u\n", conf->ch_width);

	if (conf->autorun)
		printf("----------------\n");
	if (conf->autorun > 0)
		printf("Autorun:\t%ds\n", conf->autorun);
	else if (conf->autorun < 0)
		printf("Autorun:\toff\n");
}

static void wifimgr_cli_show_capa(char *iface_name, union wifi_capa *capa)
{
	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		printf("STA Capability\n");
		if (capa->sta.max_rtt_peers)
			printf("Max RTT NR:\t%u\n", capa->sta.max_rtt_peers);
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		printf("AP Capability\n");
		if (capa->ap.max_ap_assoc_sta)
			printf("Max STA NR:\t%u\n", capa->ap.max_ap_assoc_sta);
		if (capa->ap.max_acl_mac_addrs)
			printf("Max ACL NR:\t%u\n", capa->ap.max_acl_mac_addrs);
	}
}

static void wifimgr_cli_show_status(char *iface_name,
				    struct wifi_status *status)
{
	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		printf("STA Status:\t%s\n", sta_sts2str(status->state));
		if (status->own_mac && !is_zero_ether_addr(status->own_mac))
			printf("own MAC:\t" MACSTR "\n",
			       MAC2STR(status->own_mac));

		if (status->state == WIFI_STATE_STA_CONNECTED) {
			printf("----------------\n");
			if (status->u.sta.host_bssid
			    && !is_zero_ether_addr(status->u.sta.host_bssid))
				printf("Host BSSID:\t" MACSTR "\n",
				       MAC2STR(status->u.sta.host_bssid));
			printf("Host RSSI:\t%d\n", status->u.sta.host_rssi);
		}
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		printf("AP Status:\t%s\n", ap_sts2str(status->state));
		if (status->own_mac && !is_zero_ether_addr(status->own_mac))
			printf("BSSID:\t\t" MACSTR "\n",
			       MAC2STR(status->own_mac));

		if (status->state == WIFI_STATE_AP_STARTED) {
			int i;
			char (*mac_addrs)[WIFIMGR_ETH_ALEN];

			printf("----------------\n");
			printf("STA NR:\t%u\n", status->u.ap.nr_sta);
			if (status->u.ap.nr_sta) {
				printf("STA:\n");
				mac_addrs = status->u.ap.sta_mac_addrs;
				for (i = 0; i < status->u.ap.nr_sta; i++)
					printf("\t\t" MACSTR "\n",
					       MAC2STR(mac_addrs[i]));
			}

			printf("----------------\n");
			printf("ACL NR:\t%u\n", status->u.ap.nr_acl);
			if (status->u.ap.nr_acl) {
				printf("ACL:\n");
				mac_addrs = status->u.ap.acl_mac_addrs;
				for (i = 0; i < status->u.ap.nr_acl; i++)
					printf("\t\t" MACSTR "\n",
					       MAC2STR(mac_addrs[i]));
			}
		}
	}
}

static void wifimgr_cli_show_scan_res(struct wifi_scan_result *scan_res)
{
	if (scan_res->ssid && strlen(scan_res->ssid))
		printf("\t%-32s", scan_res->ssid);
	else
		printf("\t\t\t\t\t");

	if (scan_res->bssid && !is_zero_ether_addr(scan_res->bssid))
		printf("\t" MACSTR, MAC2STR(scan_res->bssid));
	else
		printf("\t\t\t");

	printf("\t%s", security2str(scan_res->security));
	printf("\t%u\t%d\n", scan_res->channel, scan_res->rssi);
}

#ifdef CONFIG_WIFIMGR_STA
static void wifimgr_cli_show_rtt_resp(struct wifi_rtt_response *rtt_resp)
{
	if (rtt_resp->bssid && !is_zero_ether_addr(rtt_resp->bssid))
		printf("\t" MACSTR, MAC2STR(rtt_resp->bssid));
	else
		printf("\t\t\t");

	printf("\t%d\n", rtt_resp->range);
}
#endif

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

/* WiFi Manager CLI client commands */
static int wifimgr_cli_cmd_set_config(const struct shell *shell, size_t argc,
				      char *argv[])
{
	struct wifi_config conf;
	char *iface_name;
	int choice;

	if (!argv[1])
		return -EINVAL;
	iface_name = argv[1];

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		printf("Setting STA Config ...\n");
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		printf("Setting AP Config ...\n");
	else
		return -EINVAL;

	memset(&conf, 0, sizeof(conf));

	optind = 0;
	while ((choice = getopt(argc, argv, "a:b:c:m:n:p:w:")) != -1) {
		switch (choice) {
		case 'a':
			conf.autorun = atoi(optarg);
			if (!conf.autorun) {
				printf("invalid autorun!\n");
				return -EINVAL;
			}
			break;
		case 'b':
			conf.band = atoi(optarg);
			if (!conf.band) {
				printf("invalid band!\n");
				return -EINVAL;
			}
			break;
		case 'c':
			conf.channel = atoi(optarg);
			if (!conf.channel) {
				printf("invalid channel!\n");
				return -EINVAL;
			}
			break;
		case 'm':
			if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
				int ret = strtomac(optarg, conf.bssid);

				if (ret) {
					printf("invalid BSSID!\n");
					return ret;
				}
			} else {
				printf("invalid option '-%c' for '%s'\n",
				       choice, iface_name);
				return -EINVAL;
			}
			break;
		case 'n':
			if (!optarg || !strlen(optarg)) {
				printf("invalid SSID!\n");
				return -EINVAL;
			}
			strcpy(conf.ssid, optarg);
			break;
		case 'p':
			if (!optarg || !strlen(optarg))
				conf.security = WIFIMGR_SECURITY_OPEN;
			else
				conf.security = WIFIMGR_SECURITY_PSK;
			strcpy(conf.passphrase, optarg);
			break;
		case 'w':
			if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
				conf.ch_width = atoi(optarg);
				if (!conf.ch_width) {
					printf("invalid channel width!\n");
					return -EINVAL;
				}
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

	return wifimgr_ctrl_iface_set_conf(iface_name, &conf);
}

static int wifimgr_cli_cmd_clear_config(const struct shell *shell, size_t argc,
					char *argv[])
{
	char *iface_name;
	struct wifi_config conf;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		printf("Clearing STA Config ...\n");
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		printf("Clearing AP Config ...\n");
	else
		return -EINVAL;

	memset(&conf, 0, sizeof(conf));

	return wifimgr_ctrl_iface_set_conf(iface_name, &conf);
}

static int wifimgr_cli_cmd_get_config(const struct shell *shell, size_t argc,
				      char *argv[])
{
	char *iface_name;
	struct wifi_config conf;
	int ret;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		printf("STA Config\n");
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		printf("AP Config\n");
	else
		return -EINVAL;

	memset(&conf, 0, sizeof(conf));
	ret = wifimgr_ctrl_iface_get_conf(iface_name, &conf);
	if (!ret)
		wifimgr_cli_show_conf(iface_name, &conf);

	return ret;
}

static int wifimgr_cli_cmd_capa(const struct shell *shell, size_t argc,
				char *argv[])
{
	char *iface_name;
	union wifi_capa capa;
	int ret;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	memset(&capa, 0, sizeof(capa));
	ret = wifimgr_ctrl_iface_get_capa(iface_name, &capa);
	if (!ret)
		wifimgr_cli_show_capa(iface_name, &capa);

	return ret;
}

static int wifimgr_cli_cmd_status(const struct shell *shell, size_t argc,
				  char *argv[])
{
	char *iface_name;
	struct wifi_status sts;
	int ret;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	memset(&sts, 0, sizeof(sts));
	ret = wifimgr_ctrl_iface_get_status(iface_name, &sts);
	if (!ret)
		wifimgr_cli_show_status(iface_name, &sts);

	return ret;
}

static int wifimgr_cli_cmd_open(const struct shell *shell, size_t argc,
				char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_open(iface_name);
}

static int wifimgr_cli_cmd_close(const struct shell *shell, size_t argc,
				 char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_close(iface_name);
}

static int wifimgr_cli_cmd_scan(const struct shell *shell, size_t argc,
				char *argv[])
{
	char *iface_name;

	if (argc != 2 || !argv[1])
		return -EINVAL;
	iface_name = argv[1];

	return wifimgr_ctrl_iface_scan(iface_name, wifimgr_cli_show_scan_res);
}

#ifdef CONFIG_WIFIMGR_STA
static int wifimgr_cli_cmd_rtt_req(const struct shell *shell, size_t argc,
				   char *argv[])
{
	struct wifi_rtt_request *rtt_req;
	struct wifi_rtt_peers *peer;
	int size;
	int choice;
	int ret;

	size = sizeof(struct wifi_rtt_request) + sizeof(struct wifi_rtt_peers);
	rtt_req = malloc(size);
	if (!rtt_req)
		return -ENOMEM;
	memset(rtt_req, 0, size);
	rtt_req->nr_peers = 1;
	rtt_req->peers =
	    (struct wifi_rtt_peers *)((char *)rtt_req +
				      rtt_req->nr_peers *
				      sizeof(struct wifi_rtt_request));
	peer = rtt_req->peers;

	optind = 0;
	while ((choice = getopt(argc, argv, "b:c:m:")) != -1) {
		switch (choice) {
		case 'b':
			peer->band = atoi(optarg);
			if (!peer->band) {
				printf("invalid band!\n");
				return -EINVAL;
			}
			break;
		case 'c':
			peer->channel = atoi(optarg);
			if (!peer->channel) {
				printf("invalid channel!\n");
				return -EINVAL;
			}
			break;
		case 'm':
			ret = strtomac(optarg, peer->bssid);
			if (ret) {
				printf("invalid BSSID!\n");
				return ret;
			}
			break;
		default:
			return -EINVAL;
		}
	}

	ret =
	    wifimgr_ctrl_iface_rtt_request(rtt_req, wifimgr_cli_show_rtt_resp);
	free(rtt_req);

	return ret;
}

static int wifimgr_cli_cmd_connect(const struct shell *shell, size_t argc,
				   char *argv[])
{
	return wifimgr_ctrl_iface_connect();
}

static int wifimgr_cli_cmd_disconnect(const struct shell *shell, size_t argc,
				      char *argv[])
{
	return wifimgr_ctrl_iface_disconnect();
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static int wifimgr_cli_cmd_start_ap(const struct shell *shell, size_t argc,
				    char *argv[])
{
	return wifimgr_ctrl_iface_start_ap();
}

static int wifimgr_cli_cmd_stop_ap(const struct shell *shell, size_t argc,
				   char *argv[])
{
	return wifimgr_ctrl_iface_stop_ap();
}

static int wifimgr_cli_cmd_del_sta(const struct shell *shell, size_t argc,
				   char *argv[])
{
	char mac_addr[WIFIMGR_ETH_ALEN];
	int ret;

	if (argc != 2 || !argv[1])
		return -EINVAL;

	ret = strtomac(argv[1], mac_addr);
	if (ret) {
		printf("invalid MAC address!\n");
		return ret;
	}

	if (is_broadcast_ether_addr(mac_addr))
		printf("Deauth all stations!\n");
	else
		printf("Deauth station (" MACSTR ")\n", MAC2STR(mac_addr));

	ret = wifimgr_ctrl_iface_del_station(mac_addr);

	return ret;
}

static int wifimgr_cli_cmd_set_mac_acl(const struct shell *shell, size_t argc,
				       char *argv[])
{
	int choice;
	char subcmd = 0;
	char *mac = NULL;
	int ret;

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

				ret = strtomac(optarg, mac_addr);
				if (ret) {
					printf("invalid MAC address!\n");
					return ret;
				}
				mac = mac_addr;
			}
			break;
		default:
			return -EINVAL;
		}
	}

	ret = wifimgr_ctrl_iface_set_mac_acl(subcmd, mac);

	return ret;
}
#endif

SHELL_CREATE_STATIC_SUBCMD_SET(wifimgr_commands) {
	SHELL_CMD(get_config, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_get_config),
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
	 wifimgr_cli_cmd_set_config),
	SHELL_CMD(clear_config, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_clear_config),
	SHELL_CMD(capa, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_capa),
	SHELL_CMD(status, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_status),
	SHELL_CMD(open, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_open),
	SHELL_CMD(close, NULL,
	 WIFIMGR_CMD_COMMON_HELP,
	 wifimgr_cli_cmd_close),
	SHELL_CMD(scan, NULL,
	 WIFIMGR_CMD_COMMON_HELP" <band (optional)> <channel (optional)>",
	 wifimgr_cli_cmd_scan),
#ifdef CONFIG_WIFIMGR_STA
	SHELL_CMD(rtt_req, NULL,
	 "-m <BSSID> -c <channel>",
	 wifimgr_cli_cmd_rtt_req),
	SHELL_CMD(connect, NULL,
	 NULL,
	 wifimgr_cli_cmd_connect),
	SHELL_CMD(disconnect, NULL,
	 NULL,
	 wifimgr_cli_cmd_disconnect),
#endif
#ifdef CONFIG_WIFIMGR_AP
	SHELL_CMD(start_ap, NULL,
	 NULL,
	 wifimgr_cli_cmd_start_ap),
	SHELL_CMD(stop_ap, NULL,
	 NULL,
	 wifimgr_cli_cmd_stop_ap),
	SHELL_CMD(del_sta, NULL,
	 "<MAC address (to be deleted)>",
	 wifimgr_cli_cmd_del_sta),
	SHELL_CMD(mac_acl, NULL,
	 "-a (block all connected stations)"
	 "\n-b <MAC address (to be unblocked)>"
	 "\n-c (clear all blocked stations)"
	 "\n-u <MAC address (to be unblocked)>",
	 wifimgr_cli_cmd_set_mac_acl),
#endif
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(wifimgr, &wifimgr_commands, "WiFi Manager commands", NULL);
#endif
