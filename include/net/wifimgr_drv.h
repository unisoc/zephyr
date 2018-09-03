/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_DRV_H_
#define _WIFIMGR_DRV_H_

#include <net/wifi_mgmt.h>

void wifi_drv_iface_scan_result_cb(struct net_if *iface, int status,
				   struct wifi_scan_result *entry);
void wifi_drv_iface_scan_done_cb(void *iface, int status);
void wifi_drv_iface_connect_cb(void *iface, int status);
void wifi_drv_iface_disconnect_cb(void *iface, int status);
#endif
