/*
 * @file
 * @brief WiFi manager top layer implementation
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

	if (is_common_cmd(cmd_id) == true)
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

static void *wifi_manager_drv_iface_init(struct wifi_manager *mgr, char *devname)
{
	struct device *dev;
	struct net_if *iface;

	if (!devname)
		return NULL;

	dev = device_get_binding(devname);
	if (!dev) {
		syslog(LOG_ERR, "failed to get device %s!\n", devname);
		return NULL;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		syslog(LOG_ERR, "failed to get iface %s!\n", devname);
		return NULL;
	}

	return (void *)iface;
}

int wifi_manager_low_level_init(struct wifi_manager *mgr, unsigned int cmd_id)
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
		iface = wifi_manager_drv_iface_init(mgr, devname);
		if (!iface) {
			syslog(LOG_ERR, "failed to init %s interface!\n", devname);
			return -ENODEV;
		}
		syslog(LOG_INFO, "interface %s initialized!\n", devname);
	}

	if (!mgr->sta_iface && is_sta_cmd(cmd_id) == true)
		mgr->sta_iface = iface;
	else if (!mgr->ap_iface && is_ap_cmd(cmd_id) == true)
		mgr->ap_iface = iface;

	return ret;
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

static int wifi_manager_init(struct device *unused)
{
	struct wifi_manager *mgr = &wifimgr;
	int ret;

	ARG_UNUSED(unused);

	/*setlogmask(~(LOG_MASK(LOG_DEBUG))); */
	syslog(LOG_INFO, "WiFi manager start\n");
	memset(mgr, 0, sizeof(struct wifi_manager));

	ret = wifi_manager_event_listener_init(&mgr->lsnr);
	if (ret < 0)
		syslog(LOG_ERR, "failed to init WiFi event listener!\n");

	ret = wifi_manager_command_processor_init(&mgr->prcs);
	if (ret < 0)

		syslog(LOG_ERR, "failed to init WiFi command processor!\n");

	ret = wifi_manager_sm_init(mgr);
	if (ret < 0)
		syslog(LOG_ERR, "failed to init WiFi state machine!\n");

	return ret;
}

SYS_INIT(wifi_manager_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
