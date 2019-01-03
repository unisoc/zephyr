/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WIFI_MAIN_H__
#define __WIFI_MAIN_H__

#include <zephyr.h>
#include <net/wifimgr_drv.h>


#define WIFI_MODE_NONE (0)
#define WIFI_MODE_STA (1)
#define WIFI_MODE_AP (2)
/* #define WIFI_MODE_APSTA (3) */
/* #define WIFI_MODE_MONITOR (4) */

#define MAX_WIFI_DEV_NUM (2)

#define WIFI_DEV_STA (0)
#define WIFI_DEV_AP (1)

#define ETH_ALEN (6)
#define IPV4_LEN (4)

struct wifi_device {
	/* bool connecting; */
	bool connected;
	bool opened;
	u8_t mode;
	u8_t mac[ETH_ALEN];
	u8_t ipv4_addr[IPV4_LEN];
	/* Maximum stations on softap */
	u8_t max_sta_num;
	/* Maximum stations in blacklist on softap */
	u8_t max_blacklist_num;
	scan_result_cb_t scan_result_cb;
	connect_cb_t connect_cb;
	disconnect_cb_t disconnect_cb;
	new_station_t new_station_cb;
	struct net_if *iface;
	struct device *dev;
};

struct wifi_priv {
	struct wifi_device wifi_dev[MAX_WIFI_DEV_NUM];
	u32_t cp_version;
	bool initialized;
};

static inline void uwp_save_addr_before_payload(u32_t payload, void *addr)
{
	u32_t *pkt_ptr;

	pkt_ptr = (u32_t *)(payload - 4);
	*pkt_ptr = (u32_t)addr;
}

static inline u32_t uwp_get_addr_from_payload(u32_t payload)
{
	u32_t *ptr;

	ptr = (u32_t *)(payload - 4);

	return *ptr;
}
/* extern struct adapter wifi_pAd; */


struct wifi_device *get_wifi_dev_by_dev(struct device *dev);
/* int wifi_ifnet_sta_init(struct adapter *pAd); */
/* int wifi_ifnet_ap_init(struct adapter *pAd); */
/* struct netif *wifi_ifnet_get_interface(struct adapter *pAd,int ctx_id); */


#endif /* __WIFI_MAIN_H__ */
