/*
 * @file
 * @brief SoftAP mode handling
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#include "wifimgr.h"

static int wifimgr_ap_stop(void *handle);
static int wifimgr_ap_close(void *handle);

void wifimgr_ap_event_timeout(wifimgr_work *work)
{
	struct wifimgr_state_machine *ap_sm =
	    container_of(work, struct wifimgr_state_machine, work);
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	unsigned int expected_evt;

	/* Notify the external caller */
	switch (ap_sm->cur_cmd) {
	case WIFIMGR_CMD_SET_MAC_ACL:
		expected_evt = WIFIMGR_EVT_NEW_STATION;
		wifimgr_err("[%s] timeout!\n", wifimgr_evt2str(expected_evt));

		if (cbs && cbs->notify_set_mac_acl_timeout)
			cbs->notify_set_mac_acl_timeout();
		break;
	default:
		break;
	}
}

static int wifimgr_ap_get_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	char *ssid = NULL;
	char *passphrase = NULL;

	/* Load config form non-volatile memory */
	wifimgr_config_load(conf, WIFIMGR_SETTING_AP_PATH);

	if (!memiszero(conf, sizeof(struct wifimgr_config))) {
		wifimgr_info("No AP Config found!\n");
	} else {
		wifimgr_info("AP Config\n");
		if (conf->autorun)
			wifimgr_info("Autorun:\t%d\n", conf->autorun);

		if (strlen(conf->ssid)) {
			ssid = conf->ssid;
			wifimgr_info("SSID:\t\t%s\n", ssid);
		}

		if (strlen(conf->passphrase)) {
			passphrase = conf->passphrase;
			wifimgr_info("Passphrase:\t%s\n", passphrase);
		}

		if (conf->channel)
			wifimgr_info("Channel:\t%d\n", conf->channel);

		if (conf->ch_width)
			wifimgr_info("Channel Width:\t%d\n", conf->ch_width);
	}

	fflush(stdout);

	/* Notify the external caller */
	if (cbs && cbs->get_ap_conf_cb)
		cbs->get_ap_conf_cb(ssid, passphrase, conf->band, conf->channel,
				    conf->ch_width, conf->autorun);

	return 0;
}

static int wifimgr_ap_set_config(void *handle)
{
	struct wifimgr_config *conf = (struct wifimgr_config *)handle;

	if (!memiszero(conf, sizeof(struct wifimgr_config))) {
		wifimgr_info("Clearing STA Config ...\n");
		wifimgr_config_clear(conf, WIFIMGR_SETTING_AP_PATH);
		return 0;
	}

	wifimgr_info("Setting AP Config ...\n");

	if (conf->autorun)
		wifimgr_info("Autorun:\t%d\n", conf->autorun);

	if (strlen(conf->ssid))
		wifimgr_info("SSID:\t\t%s\n", conf->ssid);

	if (strlen(conf->passphrase))
		wifimgr_info("Passphrase:\t%s\n", conf->passphrase);

	if (conf->channel)
		wifimgr_info("Channel:\t%d\n", conf->channel);

	if (conf->ch_width)
		wifimgr_info("Channel Width:\t%d\n", conf->ch_width);

	fflush(stdout);

	/* Save config to non-volatile memory */
	wifimgr_config_save(conf, WIFIMGR_SETTING_AP_PATH);

	return 0;
}

static int wifimgr_ap_get_capa(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifi_drv_capa *capa = &mgr->ap_capa;
	int ret;

	/* Check driver capability */
	ret = wifi_drv_get_capa(mgr->ap_iface, capa);
	if (ret) {
		wifimgr_warn("failed to get driver capability!");
		return ret;
	}

	wifimgr_info("AP Capability\n");

	if (capa->max_ap_assoc_sta)
		wifimgr_info("Max STA NR:\t%d\n", capa->max_ap_assoc_sta);
	if (capa->max_acl_mac_addrs)
		wifimgr_info("Max ACL NR:\t%d\n", capa->max_acl_mac_addrs);

	return ret;
}

static int wifimgr_ap_get_status(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sm = &mgr->ap_sm;
	struct wifimgr_status *sts = &mgr->ap_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();

	wifimgr_info("AP Status:\t%s\n", ap_sts2str(sm_ap_query(sm)));

	if (is_zero_ether_addr(sts->own_mac))
		if (wifi_drv_get_mac(mgr->ap_iface, sts->own_mac))
			wifimgr_warn("failed to get Own MAC!\n");
	wifimgr_info("Own MAC:\t%02x:%02x:%02x:%02x:%02x:%02x\n",
		     sts->own_mac[0], sts->own_mac[1], sts->own_mac[2],
		     sts->own_mac[3], sts->own_mac[4], sts->own_mac[5]);

	if (sm_ap_started(sm) == true) {
		int i;

		wifimgr_info("----------------\n");
		wifimgr_info("STA NR:\t%d\n", sts->u.ap.sta_nr);

		if (sts->u.ap.sta_nr) {
			wifimgr_info("STA:\n");
			for (i = 0; i < sts->u.ap.sta_nr; i++)
				wifimgr_info
				    ("\t\t%02x:%02x:%02x:%02x:%02x:%02x\n",
				     sts->u.ap.sta_mac_addrs[i][0],
				     sts->u.ap.sta_mac_addrs[i][1],
				     sts->u.ap.sta_mac_addrs[i][2],
				     sts->u.ap.sta_mac_addrs[i][3],
				     sts->u.ap.sta_mac_addrs[i][4],
				     sts->u.ap.sta_mac_addrs[i][5]);
		}

		wifimgr_info("----------------\n");
		wifimgr_info("ACL NR:\t%d\n", sts->u.ap.acl_nr);
		if (sts->u.ap.acl_nr) {
			wifimgr_info("ACL:\n");
			for (i = 0; i < sts->u.ap.acl_nr; i++)
				wifimgr_info
				    ("\t\t%02x:%02x:%02x:%02x:%02x:%02x\n",
				     sts->u.ap.acl_mac_addrs[i][0],
				     sts->u.ap.acl_mac_addrs[i][1],
				     sts->u.ap.acl_mac_addrs[i][2],
				     sts->u.ap.acl_mac_addrs[i][3],
				     sts->u.ap.acl_mac_addrs[i][4],
				     sts->u.ap.acl_mac_addrs[i][5]);
		}
	}

	/* Notify the external caller */
	if (cbs && cbs->get_ap_status_cb)
		cbs->get_ap_status_cb(sm_ap_query(sm), sts->own_mac,
				      sts->u.ap.sta_nr,
				      sts->u.ap.sta_mac_addrs,
				      sts->u.ap.acl_nr,
				      sts->u.ap.acl_mac_addrs);
	fflush(stdout);

	return 0;
}

static int wifimgr_ap_del_station(void *handle)
{
	struct wifimgr_ap_acl *acl = (struct wifimgr_ap_acl *)handle;
	struct wifi_manager *mgr = container_of(acl, struct wifi_manager, acl);
	int ret;

	if (is_zero_ether_addr(acl->mac))
		return -EINVAL;

	if (is_broadcast_ether_addr(acl->mac))
		wifimgr_info("Deauth all stations!\n");
	else
		wifimgr_info("Deauth station (%02x:%02x:%02x:%02x:%02x:%02x)\n",
			     acl->mac[0], acl->mac[1], acl->mac[2],
			     acl->mac[3], acl->mac[4], acl->mac[5]);

	ret = wifi_drv_del_station(mgr->ap_iface, acl->mac);

	return ret;
}

static int search_mac_addr(char (*mac_addrs)[WIFIMGR_ETH_ALEN], char nr,
			   char *mac)
{
	int i;

	for (i = 0; i < nr; i++)
		if (!memcmp(mac_addrs + i, mac, WIFIMGR_ETH_ALEN))
			break;
	if (i == nr)
		return 0;

	return 1;
}

static int wifimgr_ap_set_mac_acl(void *handle)
{
	struct wifimgr_ap_acl *acl = (struct wifimgr_ap_acl *)handle;
	struct wifi_manager *mgr = container_of(acl, struct wifi_manager, acl);
	struct wifimgr_status *sts = &mgr->ap_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	bool pending = false;
	char subcmd = 0;
	int result = 0;
	int ret = 0;

	if (is_zero_ether_addr(acl->mac))
		return -EINVAL;

	switch (acl->subcmd) {
	case WIFIMGR_SUBCMD_ACL_BLOCK:
		if (sts->u.ap.acl_nr >= mgr->ap_capa.max_acl_mac_addrs) {
			wifimgr_warn("Max number of ACL reached!");
			result = -ENOSPC;
			break;
		}

		ret =
		    search_mac_addr(sts->u.ap.acl_mac_addrs, sts->u.ap.acl_nr,
				    acl->mac);
		if (ret) {
			wifimgr_info("Duplicate item found!\n");
			result = -EEXIST;
			break;
		}

		sts->u.ap.acl_nr++;
		memcpy(sts->u.ap.acl_mac_addrs + (sts->u.ap.acl_nr - 1),
		       acl->mac, WIFIMGR_ETH_ALEN);
		subcmd = WIFI_DRV_BLACKLIST_ADD;
		pending = true;
		wifimgr_info("Block ");
		break;
	case WIFIMGR_SUBCMD_ACL_UNBLOCK:
		if (!sts->u.ap.acl_nr) {
			wifimgr_warn("No ACL to delete!");
			result = -ENOENT;
			break;
		}

		ret =
		    search_mac_addr(sts->u.ap.acl_mac_addrs, sts->u.ap.acl_nr,
				    acl->mac);
		if (!ret) {
			wifimgr_info("No matches found!\n");
			result = -ENOENT;
			break;
		}

		memset(sts->u.ap.acl_mac_addrs + (sts->u.ap.acl_nr - 1),
		       0, WIFIMGR_ETH_ALEN);
		sts->u.ap.acl_nr--;
		subcmd = WIFI_DRV_BLACKLIST_DEL;
		pending = true;
		wifimgr_info("Unblock ");
		break;
	case WIFIMGR_SUBCMD_ACL_BLOCK_ALL:
		wifimgr_info("Block ");
		subcmd = WIFI_DRV_BLACKLIST_ADD;
		pending = true;
		break;
	case WIFIMGR_SUBCMD_ACL_UNBLOCK_ALL:
		wifimgr_info("Unblock ");
		memset(sts->u.ap.acl_mac_addrs, 0,
		       mgr->ap_capa.max_acl_mac_addrs * WIFIMGR_ETH_ALEN);
		subcmd = WIFI_DRV_BLACKLIST_DEL;
		pending = true;
		break;
	default:
		return -EINVAL;
	}

	if (pending == true) {
		if (is_broadcast_ether_addr(acl->mac))
			wifimgr_info("all stations!\n");
		else
			wifimgr_info("(%02x:%02x:%02x:%02x:%02x:%02x)\n",
				     acl->mac[0], acl->mac[1], acl->mac[2],
				     acl->mac[3], acl->mac[4], acl->mac[5]);

		/* Set MAC ACL */
		ret =
		    wifi_drv_set_mac_acl(mgr->ap_iface, subcmd,
					 sts->u.ap.acl_nr,
					 sts->u.ap.acl_mac_addrs);
		if (ret) {
			wifimgr_err("failed to set MAC ACL! %d\n", ret);
			result = ret;
		}
	}

	pending = false;
	switch (acl->subcmd) {
	case WIFIMGR_SUBCMD_ACL_BLOCK:
		if (!sts->u.ap.sta_nr) {
			wifimgr_warn("No Station to deauth!");
			result = -ENOENT;
			break;
		}

		ret =
		    search_mac_addr(sts->u.ap.sta_mac_addrs, sts->u.ap.sta_nr,
				    acl->mac);
		if (!ret) {
			wifimgr_info("No matches found!\n");
			result = -ENOENT;
			break;
		}

		pending = true;
		wifimgr_info("Deauth ");
		break;
	case WIFIMGR_SUBCMD_ACL_BLOCK_ALL:
		wifimgr_info("Deauth ");
		pending = true;
		break;
	case WIFIMGR_SUBCMD_ACL_UNBLOCK:
	case WIFIMGR_SUBCMD_ACL_UNBLOCK_ALL:
		break;
	default:
		return -EINVAL;
	}

	if (pending == true) {
		if (is_broadcast_ether_addr(acl->mac))
			wifimgr_info("all stations!\n");
		else
			wifimgr_info("(%02x:%02x:%02x:%02x:%02x:%02x)\n",
				     acl->mac[0], acl->mac[1], acl->mac[2],
				     acl->mac[3], acl->mac[4], acl->mac[5]);

		/* Deauth station */
		ret = wifi_drv_del_station(mgr->ap_iface, acl->mac);
		if (ret) {
			wifimgr_err("failed to deauth STA! %d\n", ret);
			result = ret;
		}
	}

	if (cbs && cbs->notify_set_mac_acl)
		cbs->notify_set_mac_acl(result);

	return ret;
}

static int wifimgr_ap_new_station_event(void *arg)
{
	struct wifimgr_ap_event *ap_evt = (struct wifimgr_ap_event *)arg;
	struct wifi_drv_new_station_evt *new_sta = &ap_evt->u.new_sta;
	struct wifi_manager *mgr =
	    container_of(ap_evt, struct wifi_manager, ap_evt);
	struct wifimgr_status *sts = &mgr->ap_sts;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	bool pending = false;

	wifimgr_info("Station (%02x:%02x:%02x:%02x:%02x:%02x) %s!\n",
		     new_sta->mac[0], new_sta->mac[1], new_sta->mac[2],
		     new_sta->mac[3], new_sta->mac[4], new_sta->mac[5],
		     new_sta->is_connect ? "connected" : "disconnected");
	fflush(stdout);

	if (new_sta->is_connect) {
		if (!sts->u.ap.sta_nr)
			cmd_processor_add_sender(&mgr->prcs,
						 WIFIMGR_CMD_DEL_STA,
						 wifimgr_ap_del_station,
						 &mgr->acl);

		if (sts->u.ap.sta_nr >= mgr->ap_capa.max_ap_assoc_sta) {
			wifimgr_warn("Max number of stations reached!");
			return 0;
		}

		sts->u.ap.sta_nr++;
		memcpy(sts->u.ap.sta_mac_addrs + (sts->u.ap.sta_nr - 1),
		       new_sta->mac, WIFIMGR_ETH_ALEN);
		pending = true;
	} else {
		if (!sts->u.ap.sta_nr) {
			wifimgr_warn("No connected stations!");
			return 0;
		}

		memset(sts->u.ap.sta_mac_addrs + (sts->u.ap.sta_nr - 1),
		       0, WIFIMGR_ETH_ALEN);
		sts->u.ap.sta_nr--;
		pending = true;

		if (!sts->u.ap.sta_nr)
			cmd_processor_remove_sender(&mgr->prcs,
						    WIFIMGR_CMD_DEL_STA);
	}

	/* Notify the external caller */
	if (cbs && cbs->notify_new_station && (pending == true))
		cbs->notify_new_station(new_sta->is_connect, new_sta->mac);

	return 0;
}

static int wifimgr_ap_start(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifi_drv_capa *capa = &mgr->ap_capa;
	struct wifimgr_config *conf = &mgr->ap_conf;
	struct wifimgr_status *sts = &mgr->ap_sts;
	char *ssid = NULL;
	char *passphrase = NULL;
	int size;
	int ret;

	ret = evt_listener_add_receiver(&mgr->lsnr,
					WIFIMGR_EVT_NEW_STATION, false,
					wifimgr_ap_new_station_event,
					&mgr->ap_evt);
	if (ret)
		return ret;

	if (!capa->max_ap_assoc_sta)
		capa->max_ap_assoc_sta = WIFIMGR_MAX_STA_NR;
	size = capa->max_ap_assoc_sta * WIFIMGR_ETH_ALEN;
	sts->u.ap.sta_mac_addrs = malloc(size);
	if (!sts->u.ap.sta_mac_addrs)
		return -ENOMEM;
	memset(sts->u.ap.sta_mac_addrs, 0, size);
	sts->u.ap.sta_nr = 0;

	if (!capa->max_acl_mac_addrs)
		capa->max_acl_mac_addrs = WIFIMGR_MAX_STA_NR;
	size = capa->max_acl_mac_addrs * WIFIMGR_ETH_ALEN;
	sts->u.ap.acl_mac_addrs = malloc(size);
	if (!sts->u.ap.acl_mac_addrs) {
		free(sts->u.ap.sta_mac_addrs);
		return -ENOMEM;
	}
	memset(sts->u.ap.acl_mac_addrs, 0, size);
	sts->u.ap.acl_nr = 0;

	if (strlen(conf->ssid))
		ssid = conf->ssid;
	if (strlen(conf->passphrase))
		passphrase = conf->passphrase;
	ret = wifi_drv_start_ap(mgr->ap_iface, ssid, passphrase,
				conf->channel, conf->ch_width);

	if (ret) {
		wifimgr_err("failed to start AP! %d\n", ret);
		evt_listener_remove_receiver(&mgr->lsnr,
					     WIFIMGR_EVT_NEW_STATION);
		free(sts->u.ap.sta_mac_addrs);
		free(sts->u.ap.acl_mac_addrs);
		return ret;
	}

	wifimgr_ap_led_on();

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP,
				 wifimgr_ap_stop, mgr);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_SET_MAC_ACL,
				 wifimgr_ap_set_mac_acl, &mgr->acl);

	/* TODO: Start DHCP server */

	return ret;
}

static int wifimgr_ap_stop(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_status *sts = &mgr->ap_sts;
	int ret;

	evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_NEW_STATION);
	free(sts->u.ap.sta_mac_addrs);
	free(sts->u.ap.acl_mac_addrs);

	/* Deauth all stations before leaving */
	memset(mgr->acl.mac, 0xff, sizeof(mgr->acl.mac));
	ret = wifi_drv_del_station(mgr->ap_iface, mgr->acl.mac);
	if (ret)
		wifimgr_warn("failed to deauth all stations!\n");

	ret = wifi_drv_stop_ap(mgr->ap_iface);
	if (ret) {
		wifimgr_err("failed to stop AP!\n");
		return ret;
	}

	wifimgr_ap_led_off();

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_SET_MAC_ACL);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_START_AP,
				 wifimgr_ap_start, mgr);

	return ret;
}

static int wifimgr_ap_open(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_open(mgr->ap_iface);
	if (ret) {
		wifimgr_err("failed to open AP!\n");
		return ret;
	}

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_AP);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_AP,
				 wifimgr_ap_close, mgr);
	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_START_AP,
				 wifimgr_ap_start, mgr);

	return ret;
}

static int wifimgr_ap_close(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret;

	ret = wifi_drv_close(mgr->ap_iface);
	if (ret) {
		wifimgr_err("failed to close AP!\n");
		return ret;
	}

	evt_listener_remove_receiver(&mgr->lsnr, WIFIMGR_EVT_NEW_STATION);

	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_STOP_AP);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_START_AP);
	cmd_processor_remove_sender(&mgr->prcs, WIFIMGR_CMD_CLOSE_AP);

	cmd_processor_add_sender(&mgr->prcs, WIFIMGR_CMD_OPEN_AP,
				 wifimgr_ap_open, mgr);

	return ret;
}

void wifimgr_ap_init(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct cmd_processor *prcs = &mgr->prcs;

	/* Register default AP commands */
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_SET_AP_CONFIG,
				 wifimgr_ap_set_config, &mgr->ap_conf);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_AP_CONFIG,
				 wifimgr_ap_get_config, &mgr->ap_conf);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_AP_CAPA,
				 wifimgr_ap_get_capa, mgr);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_GET_AP_STATUS,
				 wifimgr_ap_get_status, mgr);
	cmd_processor_add_sender(prcs, WIFIMGR_CMD_OPEN_AP,
				 wifimgr_ap_open, mgr);
}
