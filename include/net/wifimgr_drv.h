/*
 * @file
 * @brief WiFi manager callbacks for the WiFi driver
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_DRV_H_
#define _WIFIMGR_DRV_H_

#include <device.h>
#include <net/net_if.h>
#include <net/wifi.h>

struct wifi_drv_connect_params {
	char *ssid;
	char ssid_length; /* Max 32 */
	char *psk;
	char psk_length; /* Min 8 - Max 64 */
	unsigned char channel;
	enum wifi_security_type security;
};

struct wifi_drv_start_ap_params {
	char *ssid;
	char ssid_length; /* Max 32 */
	char *psk;
	char psk_length; /* Min 8 - Max 64 */
	unsigned char channel;
	enum wifi_security_type security;
};

struct wifi_drv_scan_params {
#define WIFI_BAND_2.4G	(1)
#define WIFI_BAND_5G	(2)
	unsigned char band;
	unsigned char channel;
};

struct wifi_drv_scan_result {
	char bssid[NET_LINK_ADDR_MAX_LENGTH];
	char ssid[WIFI_SSID_MAX_LEN];
	char ssid_length;
	unsigned char channel;
	char rssi;
	enum wifi_security_type security;
};

typedef void (*scan_result_cb_t)(void *iface, int status,
				 struct wifi_drv_scan_result *entry);
typedef void (*connect_cb_t)(void *iface, int status);
typedef void (*disconnect_cb_t)(void *iface, int status);
typedef void (*new_station_t)(void *iface, int status, char *mac);

struct wifi_drv_api{
	/**
	 * Mandatory to get in first position.
	 * A network device should indeed provide a pointer on such
	 * net_if_api structure. So we make current structure pointer
	 * that can be casted to a net_if_api structure pointer.
	 */
	struct net_if_api iface_api;

	int (*open)(struct device *dev);
	int (*close)(struct device *dev);
	int (*scan)(struct device *dev, struct wifi_drv_scan_params *params,
		    scan_result_cb_t cb);
	int (*connect)(struct device *dev,
		       struct wifi_drv_connect_params *params,
		       connect_cb_t cb);
	int (*disconnect)(struct device *dev, disconnect_cb_t cb);
	int (*get_station)(struct device *dev, u8_t *signal);
	int (*notify_ip)(struct device *dev, u8_t *ipaddr, u8_t len);
	int (*start_ap)(struct device *dev,
			struct wifi_drv_start_ap_params *params,
			new_station_t cb);
	int (*stop_ap)(struct device *dev);
	int (*del_station)(struct device *dev, u8_t *mac);
};

#endif
