/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <shell/shell.h>

#include "wifimgr.h"

static struct wifi_manager wifimgr;

const char *wifimgr_sts2str(struct wifi_manager *mgr, unsigned int cmd_id)
{
	if (is_sta_cmd(cmd_id) == true)
		return sta_sts2str(mgr->sta_sm.state);

	if (is_ap_cmd(cmd_id) == true)
		return ap_sts2str(mgr->ap_sm.state);

	return NULL;
}

int wifi_manager_sm_start_timer(struct wifi_manager *mgr, unsigned int cmd_id)
{
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;
	struct wifimgr_state_machine *ap_sm = &mgr->ap_sm;
	int ret = 0;

	if (is_sta_cmd(cmd_id) == true)
		ret = sm_sta_timer_start(sta_sm, cmd_id);

	if (is_ap_cmd(cmd_id) == true)
		ret = sm_ap_timer_start(ap_sm, cmd_id);

	return ret;
}

int wifi_manager_sm_stop_timer(struct wifi_manager *mgr, unsigned int evt_id)
{
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;
	struct wifimgr_state_machine *ap_sm = &mgr->ap_sm;
	int ret = 0;

	if (is_sta_evt(evt_id) == true)
		ret = sm_sta_timer_stop(sta_sm, evt_id);

	if (is_ap_evt(evt_id) == true)
		ret = sm_ap_timer_stop(ap_sm, evt_id);

	return ret;
}

int wifi_manager_sm_query_cmd(struct wifi_manager *mgr, unsigned int cmd_id)
{
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;
	int ret = 0;

	if (is_comman_cmd(cmd_id) == true)
		return ret;

	if (is_sta_cmd(cmd_id) == true)
		ret = sm_sta_query_cmd(sta_sm, cmd_id);

	/*softAP does not need query cmd for now */

	return ret;
}

void wifi_manager_sm_step_cmd(struct wifi_manager *mgr, unsigned int cmd_id)
{
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;
	struct wifimgr_state_machine *ap_sm = &mgr->ap_sm;

	if (is_sta_cmd(cmd_id) == true)
		sm_sta_step_cmd(sta_sm, cmd_id);

	if (is_ap_cmd(cmd_id) == true)
		sm_ap_step_cmd(ap_sm, cmd_id);
}

void wifi_manager_sm_step_evt(struct wifi_manager *mgr, unsigned int evt_id)
{
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;

	if (is_sta_evt(evt_id) == true)
		sm_sta_step_evt(sta_sm, evt_id);

	/*softAP does not need step evt for now */
}

void wifi_manager_sm_step_back(struct wifi_manager *mgr, unsigned int evt_id)
{
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;

	if (is_sta_evt(evt_id) == true)
		sm_sta_step_back(sta_sm);

	/*softAP does not need step evt for now */
}

static int wifi_manager_sm_init(struct wifi_manager *mgr)
{
	int ret;

	ret = sm_sta_init(&mgr->sta_sm);
	if (ret < 0)
		syslog(LOG_ERR, "failed to init WiFi STA state machine!\n");

	ret = sm_ap_init(&mgr->ap_sm);
	if (ret < 0)
		syslog(LOG_ERR, "failed to init WiFi AP state machine!\n");

	return ret;
}

int wifi_manager_get_status(void *handle)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;
	struct wifimgr_sta_status *sta_sts = &mgr->sta_sts;
	struct wifimgr_state_machine *ap_sm = &mgr->ap_sm;

	printk("STA Status:\t%s\n", sta_sts2str(sta_sm->state));

	if (sm_sta_connected(sta_sm) == true) {
		printk("SSID:\t\t%s\n", sta_sts->ssid);
		printk("BSSID:\t\t%02x:%02x:%02x:%02x:%02x:%02x\n",
		       sta_sts->bssid[0],
		       sta_sts->bssid[1],
		       sta_sts->bssid[2],
		       sta_sts->bssid[3], sta_sts->bssid[4], sta_sts->bssid[5]);
		printk("Band:\t\t%u\n", sta_sts->band);
		printk("Channel:\t%u\n", sta_sts->channel);
		wifi_manager_get_station(mgr);
		printk("Signal:\t\t%d\n", sta_sts->rssi);
	}

	printk("\n");
	printk("AP Status:\t%s\n", ap_sts2str(ap_sm->state));
	fflush(stdout);

	return 0;
}

bool wifi_manager_first_run(struct wifi_manager * mgr)
{
	return mgr->fw_loaded == false;
}

static int wifi_drv_iface_init(struct wifi_manager *mgr, char *devname)
{
	struct device *dev;
	struct net_if *iface;

	dev = device_get_binding(devname);
	if (!dev) {
		syslog(LOG_ERR, "failed to get device %s!\n", devname);
		return -ENODEV;
	}
	mgr->sta_dev = dev;

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		syslog(LOG_ERR, "failed to get iface %s!\n", devname);
		return -EINVAL;
	}
	mgr->sta_iface = iface;

	return 0;
}

int wifi_manager_low_level_init(struct wifi_manager *mgr)
{
	int ret;

	syslog(LOG_DEBUG, "first time!\n");
/*
	ret = wifi_drv_iface_load_fw(UNSC_FIRMWARE_WIFI);
	if (ret) {
		syslog(LOG_ERR, "failed to load WiFi firmware!\n");
		return ret;
	}
*/
	ret = wifi_drv_iface_init(mgr, WIFIMGR_STA_DEVNAME);
	if (ret) {
		syslog(LOG_ERR, "failed to init WiFi driver interface!\n");
		return ret;
	}

/*
	[> Register driver callback for WiFi events <]
	wifi_drv_iface_register_event_cb(wifi_drv_iface_notify_event,
					 mgr->lsnr.mq);
*/
	mgr->fw_loaded = true;

	return ret;
}

static int wifi_manager_init(struct device *unused)
{
	struct wifi_manager *mgr = &wifimgr;
	int ret;

	ARG_UNUSED(unused);

	/*setlogmask(~(LOG_MASK(LOG_DEBUG)));*/
	syslog(LOG_INFO, "WiFi manager start\n");

	memset(mgr, 0, sizeof(struct wifi_manager));

	ret = wifi_manager_sm_init(mgr);
	if (ret < 0)
		syslog(LOG_ERR, "failed to init WiFi state machine!\n");

	ret = wifi_manager_event_listener_init(&mgr->lsnr);
	if (ret < 0)
		syslog(LOG_ERR, "failed to init WiFi listener!\n");

	ret = wifi_manager_command_processor_init(&mgr->prcs);
	if (ret < 0)
		syslog(LOG_ERR, "failed to init WiFi listener!\n");
/*
	[>get default config<]
	ret = wifimgr_get_connect_config(NULL);
*/
	return ret;
}

#define WIFIMGR_PRIORITY	39
SYS_INIT(wifi_manager_init, APPLICATION, WIFIMGR_PRIORITY);
