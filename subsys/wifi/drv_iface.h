/*
 * @file
 * @brief Driver interface header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DRV_IFACE_H_
#define _DRV_IFACE_H_

int wifi_drv_iface_get_mac(void *iface, char *mac);
int wifi_drv_iface_open_station(void *iface);
int wifi_drv_iface_close_station(void *iface);
int wifi_drv_iface_scan(void *iface, unsigned char band, unsigned char channel);
int wifi_drv_iface_connect(void *iface, char *ssid, char *bssid, char *passwd,
			   unsigned char channel);
int wifi_drv_iface_disconnect(void *iface);
int wifi_drv_iface_get_station(void *iface, char *signal);
int wifi_drv_iface_notify_ip(void *iface, char *ipaddr, char len);
int wifi_drv_iface_open_softap(void *iface);
int wifi_drv_iface_close_softap(void *iface);
int wifi_drv_iface_start_ap(void *iface, char *ssid, char *passwd,
			    unsigned char channel, unsigned char channel_width);
int wifi_drv_iface_stop_ap(void *iface);
int wifi_drv_iface_del_station(void *iface, char *mac);

#endif
