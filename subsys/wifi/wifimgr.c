/*
 * @file
 * @brief WiFi manager top layer implementation
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(wifimgr);

#include <init.h>
#include <shell/shell.h>

#include "wifimgr.h"
#include "led.h"

static struct wifi_manager wifimgr;

const char *wifimgr_sts2str_cmd(struct wifi_manager *mgr, unsigned int cmd_id)
{
	if (is_sta_cmd(cmd_id) == true)
		return sta_sts2str(mgr->sta_sm.state);

	if (is_ap_cmd(cmd_id) == true)
		return ap_sts2str(mgr->ap_sm.state);

	return NULL;
}

const char *wifimgr_sts2str_evt(struct wifi_manager *mgr, unsigned int evt_id)
{
	if (is_sta_evt(evt_id) == true)
		return sta_sts2str(mgr->sta_sm.state);

	if (is_ap_evt(evt_id) == true)
		return ap_sts2str(mgr->ap_sm.state);

	return NULL;
}

int wifimgr_sm_start_timer(struct wifi_manager *mgr, unsigned int cmd_id)
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

int wifimgr_sm_stop_timer(struct wifi_manager *mgr, unsigned int evt_id)
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

int wifimgr_sm_query_cmd(struct wifi_manager *mgr, unsigned int cmd_id)
{
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;
	int ret = 0;

	if (is_common_cmd(cmd_id) == true)
		return ret;

	if (is_sta_cmd(cmd_id) == true)
		ret = sm_sta_query_cmd(sta_sm, cmd_id);

	/*softAP does not need query cmd for now */

	return ret;
}

void wifimgr_sm_step_cmd(struct wifi_manager *mgr, unsigned int cmd_id)
{
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;
	struct wifimgr_state_machine *ap_sm = &mgr->ap_sm;

	if (is_sta_cmd(cmd_id) == true)
		sm_sta_step_cmd(sta_sm, cmd_id);

	if (is_ap_cmd(cmd_id) == true)
		sm_ap_step_cmd(ap_sm, cmd_id);
}

void wifimgr_sm_step_evt(struct wifi_manager *mgr, unsigned int evt_id)
{
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;

	if (is_sta_evt(evt_id) == true)
		sm_sta_step_evt(sta_sm, evt_id);

	/*softAP does not need step evt for now */
}

void wifimgr_sm_step_back(struct wifi_manager *mgr, unsigned int evt_id)
{
	struct wifimgr_state_machine *sta_sm = &mgr->sta_sm;

	if (is_sta_evt(evt_id) == true)
		sm_sta_step_back(sta_sm);

	/*softAP does not need step evt for now */
}

static void *wifimgr_drv_iface_init(struct wifi_manager *mgr, char *devname)
{
	struct device *dev;
	struct net_if *iface;

	if (!devname)
		return NULL;

	dev = device_get_binding(devname);
	if (!dev) {
		wifimgr_err("failed to get device %s!\n", devname);
		return NULL;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		wifimgr_err("failed to get iface %s!\n", devname);
		return NULL;
	}

	return (void *)iface;
}

int wifimgr_low_level_init(struct wifi_manager *mgr, unsigned int cmd_id)
{
	char *devname = NULL;
	struct net_if *iface = NULL;
	int ret = 0;

	if (is_common_cmd(cmd_id) == true)
		return ret;

	if (!mgr->sta_iface && is_sta_cmd(cmd_id) == true)
		devname = WIFIMGR_DEV_NAME_STA;
	else if (!mgr->ap_iface && is_ap_cmd(cmd_id) == true)
		devname = WIFIMGR_DEV_NAME_AP;

	if (devname) {
		iface = wifimgr_drv_iface_init(mgr, devname);
		if (!iface) {
			wifimgr_err("failed to init %s interface!\n", devname);
			return -ENODEV;
		}
		wifimgr_info("interface %s initialized!\n", devname);
	}

	if (!mgr->sta_iface && is_sta_cmd(cmd_id) == true)
		mgr->sta_iface = iface;
	else if (!mgr->ap_iface && is_ap_cmd(cmd_id) == true)
		mgr->ap_iface = iface;

	return ret;
}

static int wifimgr_sm_init(struct wifi_manager *mgr)
{
	int ret;

	ret = sm_sta_init(&mgr->sta_sm);
	if (ret < 0)
		wifimgr_err("failed to init WiFi STA state machine!\n");

	ret = sm_ap_init(&mgr->ap_sm);
	if (ret < 0) {
		wifimgr_err("failed to init WiFi AP state machine!\n");
		sm_sta_exit(&mgr->sta_sm);
	}

	return ret;
}

static int wifimgr_init(struct device *unused)
{
	struct wifi_manager *mgr = &wifimgr;
	int ret;

	ARG_UNUSED(unused);

	/*setlogmask(~(LOG_MASK(LOG_DEBUG))); */
	memset(mgr, 0, sizeof(struct wifi_manager));

	ret = wifimgr_event_listener_init(&mgr->lsnr);
	if (ret < 0)
		wifimgr_err("failed to init WiFi event listener!\n");

	ret = wifimgr_command_processor_init(&mgr->prcs);
	if (ret < 0)

		wifimgr_err("failed to init WiFi command processor!\n");

	ret = wifimgr_sm_init(mgr);
	if (ret < 0)
		wifimgr_err("failed to init WiFi state machine!\n");

	wifimgr_sta_init(mgr);
	wifimgr_ap_init(mgr);

	wifimgr_info("WiFi manager started\n");

	return ret;
}

SYS_INIT(wifimgr_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
