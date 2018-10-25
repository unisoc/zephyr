/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WIFI_MAIN_H_
#define __WIFI_MAIN_H_

#include <zephyr.h>
#include <net/wifi_mgmt.h>
#include "wifi_cmdevt.h"
#include "wifi_txrx.h"
#include "wifi_ipc.h"

#define WIFI_MODE_NONE (0)
#define WIFI_MODE_STA (1)
#define WIFI_MODE_AP (2)
/* #define WIFI_MODE_APSTA (3) */
/* #define WIFI_MODE_MONITOR (4) */


struct wifi_priv {
	struct net_if *iface;
	u32_t cp_version;
	u8_t mode;
	u8_t mac[ETH_ALEN];
	u8_t ipv4_addr[IPV4_LEN];
	/* bool connecting; */
	/* bool connected; */
	bool opened;
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


int wifi_get_mac(u8_t *mac, int idx);
/* int wifi_ifnet_sta_init(struct adapter *pAd); */
/* int wifi_ifnet_ap_init(struct adapter *pAd); */
/* struct netif *wifi_ifnet_get_interface(struct adapter *pAd,int ctx_id); */

/* Import external interface. */
extern int cp_mcu_init(void);

#endif /* __WIFI_MAIN_H_ */
