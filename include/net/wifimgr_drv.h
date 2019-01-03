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

#include <net/ethernet.h>
#include <net/net_linkaddr.h>
#include <net/wifi.h>

#define WIFI_DRV_BLACKLIST_ADD	(1)
#define WIFI_DRV_BLACKLIST_DEL	(2)

struct wifi_drv_capa {
	unsigned char max_ap_assoc_sta;
	unsigned char max_acl_mac_addrs;
};

struct wifi_drv_scan_params {
#define WIFI_BAND_2_4G	(1)
#define WIFI_BAND_5G	(2)
	unsigned char band;
	unsigned char channel;
};

struct wifi_drv_connect_params {
	char *ssid;
	char ssid_length;	/* Max 32 */
	char *bssid;
	char *psk;
	char psk_length;	/* Min 8 - Max 64 */
	unsigned char channel;
	enum wifi_security_type security;
};

struct wifi_drv_start_ap_params {
	char *ssid;
	char ssid_length;	/* Max 32 */
	char *psk;
	char psk_length;	/* Min 8 - Max 64 */
	unsigned char channel;
	unsigned char ch_width;
	enum wifi_security_type security;
};

struct wifi_drv_connect_evt {
	char status;
	char bssid[NET_LINK_ADDR_MAX_LENGTH];
	unsigned char channel;
};

struct wifi_drv_disconnect_evt {
	char reason_code;
};

struct wifi_drv_scan_done_evt {
	char result;
};

struct wifi_drv_scan_result_evt {
	char bssid[NET_LINK_ADDR_MAX_LENGTH];
	char ssid[WIFI_SSID_MAX_LEN];
	char ssid_length;
	unsigned char band;
	unsigned char channel;
	signed char rssi;
	enum wifi_security_type security;
};

struct wifi_drv_new_station_evt {
	char is_connect;
	char mac[NET_LINK_ADDR_MAX_LENGTH];
};

typedef void (*scan_result_cb_t)(void *iface, int status,
				 struct wifi_drv_scan_result_evt *entry);
typedef void (*connect_cb_t)(void *iface, int status, char *bssid,
			     unsigned char channel);
typedef void (*disconnect_cb_t)(void *iface, int status);
typedef void (*new_station_t)(void *iface, int status, char *mac);

struct wifi_drv_api {
	/**
	 * Mandatory to get in first position.
	 * A network device should indeed provide a pointer on such
	 * ethernet_api structure. So we make current structure pointer
	 * that can be casted to a net_if_api structure pointer.
	 */
	struct ethernet_api eth_api;

	int (*get_capa)(struct device *dev, struct wifi_drv_capa *capa);
	int (*open)(struct device *dev);
	int (*close)(struct device *dev);
	int (*scan)(struct device *dev, struct wifi_drv_scan_params *params,
		    scan_result_cb_t cb);
	int (*connect)(struct device *dev,
		       struct wifi_drv_connect_params *params,
		       connect_cb_t conn_cb, disconnect_cb_t disc_cb);
	int (*disconnect)(struct device *dev, disconnect_cb_t cb);
	int (*get_station)(struct device *dev, signed char *rssi);
	int (*notify_ip)(struct device *dev, char *ipaddr, unsigned char len);
	int (*start_ap)(struct device *dev,
			struct wifi_drv_start_ap_params *params,
			new_station_t cb);
	int (*stop_ap)(struct device *dev);
	int (*del_station)(struct device *dev, char *mac);
	int (*set_mac_acl)(struct device *dev, char subcmd,
			   unsigned char acl_nr,
			   char acl_mac_addrs[][NET_LINK_ADDR_MAX_LENGTH]);
	int (*hw_test)(struct device *dev, int ictx_id,
		       char *t_buf, unsigned int t_len, char *r_buf,
		       unsigned int *r_len);
};

#endif
