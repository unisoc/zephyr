/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFI_DRV_IFACE_H_
#define _WIFI_DRV_IFACE_H_

#include <net/wifi_mgmt.h>

int wifi_drv_iface_load_fw(int firmware_type);
int wifi_drv_iface_get_mac(int ictx_id, char *buf);
int wifi_drv_iface_open_station(struct device *dev);
int wifi_drv_iface_close_station(struct device *dev);
int wifi_drv_iface_scan(struct device *dev);
int wifi_drv_iface_connect(struct device *dev, char *ssid, char *passwd);
int wifi_drv_iface_disconnect(struct device *dev);
int wifi_drv_iface_get_station(struct device *dev, char* signal);
int wifi_drv_iface_open_softap(struct device *dev);
int wifi_drv_iface_close_softap(struct device *dev);
int wifi_drv_iface_start_softap(struct device *dev, char *ssid, char *passwd, char channel);
int wifi_drv_iface_stop_softap(struct device *dev);
int wifi_drv_iface_del_station(struct device *dev, char *mac);

void wifi_drv_iface_scan_result_cb(struct net_if *iface, int status,
				   struct wifi_scan_result *entry);
void wifi_drv_iface_scan_done_cb(struct net_if *iface, int status);
void wifi_drv_iface_connect_cb(struct net_if *iface, int status);
void wifi_drv_iface_disconnect_cb(struct net_if *iface, int status);
#endif
