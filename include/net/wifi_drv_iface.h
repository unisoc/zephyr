/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFI_DRV_IFACE_H_
#define _WIFI_DRV_IFACE_H_

#include <net/wifi_mgmt.h>

int wifi_drv_iface_get_mac(void *iface, char *mac);
int wifi_drv_iface_open_station(void *iface);
int wifi_drv_iface_close_station(void *iface);
int wifi_drv_iface_scan(void *iface);
int wifi_drv_iface_connect(void *iface, char *ssid, char *passwd);
int wifi_drv_iface_disconnect(void *iface);
int wifi_drv_iface_get_station(void *iface, char *signal);
int wifi_drv_iface_open_softap(void *iface);
int wifi_drv_iface_close_softap(void *iface);
int wifi_drv_iface_start_softap(void *iface, char *ssid, char *passwd,
				char channel);
int wifi_drv_iface_stop_softap(void *iface);
int wifi_drv_iface_del_station(void *iface, char *mac);

void wifi_drv_iface_scan_result_cb(struct net_if *iface, int status,
				   struct wifi_scan_result *entry);
void wifi_drv_iface_scan_done_cb(void *iface, int status);
void wifi_drv_iface_connect_cb(void *iface, int status);
void wifi_drv_iface_disconnect_cb(void *iface, int status);
#endif
