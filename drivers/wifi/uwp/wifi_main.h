/*
 * (C) Copyright 2017
 * Dong Xiang, <dong.xiang@spreadtrum.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __WIFI_MAIN_H_
#define __WIFI_MAIN_H_

#include <zephyr.h>
#include <net/wifi_mgmt.h>
#include "wifi_cmdevt.h"
#include "wifi_txrx.h"
//#include "wifi_msg.h"
#include "wifi_ipc.h"
#include "wifi_rf.h"

#define WIFI_MODE_STA       1
#define WIFI_MODE_AP        2
#define WIFI_MODE_APSTA     3
#define WIFI_MODE_MONITOR   4

struct wifi_priv {
	//	struct socket_data socket_data[
	//		CONFIG_WIFI_WINC1500_OFFLOAD_MAX_SOCKETS];
	struct net_if *iface;
	unsigned char mac[6];
	scan_result_cb_t scan_cb;
	u8_t scan_result;
	bool connecting;
	bool connected;
	bool opened;

	struct wifi_conf_t conf;
};

//extern struct adapter wifi_pAd;

//int wifi_ifnet_sta_init(struct adapter *pAd);
//int wifi_ifnet_ap_init(struct adapter *pAd);
//struct netif *wifi_ifnet_get_interface(struct adapter *pAd,int ctx_id);
extern int wifi_ipc_send(int ch,int prio,void *data,int len, int offset);
extern int wifi_get_mac(u8_t *mac,int idx);
extern int wifi_ipc_init(void);

#endif /* __WIFI_MAIN_H_ */
