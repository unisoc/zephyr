/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_WIFI_LEVEL
#define SYS_LOG_DOMAIN "dev/wifi"
#if (SYS_LOG_LEVEL > SYS_LOG_LEVEL_OFF)
#define NET_LOG_ENABLED (1)
#endif

#include <logging/sys_log.h>

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/net_l2.h>
#include <net/net_context.h>
#include <net/net_offload.h>
#include <net/wifi_mgmt.h>
#include <net/ethernet.h>

#include "wifi_main.h"
#include "wifi_rf.h"
#include "sipc.h"

/*
 * We do not need <socket/include/socket.h>
 * It seems there is a bug in ASF side: if OS is already defining sockaddr
 * and all, ASF will not need to define it. Unfortunately its socket.h does
 * but also defines some NM API functions there (??), so we need to redefine
 * those here.
 */
#define __SOCKET_H__
#define HOSTNAME_MAX_SIZE (64)

#define MTU (1500)

#define SEC1 (1)
#define SEC2 (2)

#define DEV_DATA(dev) \
	((struct wifi_priv *)(dev)->driver_data)

#ifdef CONFIG_WIFI_UWP_USE_SRAM
#define put_reg32(val, reg) sys_write32(val, reg)
#define get_reg32(reg) sys_read32(reg)

#define DELAY(time) \
do { \
	unsigned int delay_time; \
	for (delay_time = 0; delay_time < (time * 208); delay_time++) { \
		*(volatile unsigned int *)0x100000; \
	} \
} while (0)
#endif /* CONFIG_WIFI_UWP_USE_SRAM */

static struct wifi_priv uwp_wifi_sta_priv;
static struct wifi_priv uwp_wifi_ap_priv;

int wifi_get_mac(u8_t *mac, int idx)
{
	/*
	 * 1 get mac address from flash.
	 * 2 copy to mac
	 * if idx == 0,get sta mac,
	 * if idx == 1,get ap mac
	 */
	mac[0] = 0x46;
	mac[1] = 0xd1;
	mac[2] = 0xf9;
	mac[3] = 0x44;
	mac[4] = 0x73;
	mac[5] = 0x2c;

	return 0;
}

static int wifi_rf_init(void)
{
	int ret = 0;

	SYS_LOG_DBG("download the first section of config file");
	ret = wifi_cmd_load_ini(sec1_table, sizeof(sec1_table), SEC1);
	if (ret) {
		SYS_LOG_ERR("download first section ini fail,ret = %d", ret);
		return ret;
	}

	SYS_LOG_DBG("download the second section of config file");
	ret = wifi_cmd_load_ini(sec2_table, sizeof(sec2_table), SEC2);
	if (ret) {
		SYS_LOG_ERR("download second section ini fail,ret = %d", ret);
		return ret;
	}

	SYS_LOG_DBG("Load wifi ini success.");

	return 0;
}

static int uwp_mgmt_open(struct device *dev)
{
	int ret;
	struct wifi_priv *priv = DEV_DATA(dev);

	if (priv->opened) {
		return -EAGAIN;
	}

	ret = wifi_cmd_start(priv);
	if (ret) {
		return ret;
	}

	if (priv->mode == WIFI_MODE_STA) {
		wifi_tx_empty_buf(TOTAL_RX_ADDR_NUM);
	}

	priv->opened = true;

	return 0;
}

static int uwp_mgmt_close(struct device *dev)
{
	int ret;
	struct wifi_priv *priv = DEV_DATA(dev);

	if (!priv->opened) {
		return -EAGAIN;
	}

	ret = wifi_cmd_stop(priv);
	if (ret) {
		return ret;
	}

	priv->opened = false;

	/* FIXME need to release all buffer which has been sent to CP. */

	return 0;
}

static int uwp_mgmt_start_ap(struct device *dev,
			     struct wifi_start_ap_req_params *params)
{
	struct wifi_priv *priv = DEV_DATA(dev);

	return wifi_cmd_start_ap(priv, params);
}

static int uwp_mgmt_stop_ap(struct device *dev)
{
	struct wifi_priv *priv = DEV_DATA(dev);

	return wifi_cmd_stop_ap(priv);
}

static int uwp_mgmt_del_station(struct device *dev,
				u8_t *mac)
{
	return 0;
}

static int uwp_mgmt_scan(struct device *dev, scan_result_cb_t cb)
{
	ARG_UNUSED(cb);

	struct wifi_priv *priv = DEV_DATA(dev);

	return wifi_cmd_scan(priv);
}

static int uwp_mgmt_get_station(struct device *dev,
		/* struct wifi_get_sta_req_params *params */
		u8_t *signal)
{
	return 0;
}

static int uwp_mgmt_connect(struct device *dev,
			    struct wifi_connect_req_params *params)
{
	struct wifi_priv *priv = DEV_DATA(dev);

	return wifi_cmd_connect(priv, params);
}

static int uwp_mgmt_disconnect(struct device *dev)
{
	struct wifi_priv *priv = DEV_DATA(dev);

	return wifi_cmd_disconnect(priv);
}

static int uwp_mgmt_set_ip(struct device *dev, u8_t *ip, u8_t len)
{
	struct wifi_priv *priv = DEV_DATA(dev);

	return wifi_cmd_set_ip(priv, ip, len);
}

static void uwp_iface_init(struct net_if *iface)
{
	struct wifi_priv *priv = DEV_DATA(iface->if_dev->dev);

	k_sleep(50);
	wifi_cmd_get_cp_info(priv);

	SYS_LOG_INF("mode: %d, cp version: 0x%x.",
		    priv->mode, priv->cp_version);

	net_if_set_link_addr(iface, priv->mac, sizeof(priv->mac),
			     NET_LINK_ETHERNET);

	priv->iface = iface;
}

static int wifi_tx_fill_msdu_dscr(struct wifi_priv *priv,
			   struct net_pkt *pkt, u8_t type, u8_t offset)
{
	u32_t addr = 0;
	u32_t reserve_len = net_pkt_ll_reserve(pkt);
	struct tx_msdu_dscr *dscr = NULL;

	net_pkt_set_ll_reserve(pkt,
			sizeof(struct tx_msdu_dscr) +
			net_pkt_ll_reserve(pkt));
	SYS_LOG_DBG("size msdu: %d", sizeof(struct tx_msdu_dscr));

	dscr = (struct tx_msdu_dscr *)net_pkt_ll(pkt);
	memset(dscr, 0x00, sizeof(struct tx_msdu_dscr));
	addr = (u32_t)dscr;
	SPRD_AP_TO_CP_ADDR(addr);
	dscr->next_buf_addr_low = addr;
	dscr->next_buf_addr_high = 0x0;

	dscr->tx_ctrl.checksum_offload = 0;
	dscr->common.type =
		(type == SPRDWL_TYPE_CMD ? SPRDWL_TYPE_CMD : SPRDWL_TYPE_DATA);
	dscr->common.direction_ind = TRANS_FOR_TX_PATH;
	dscr->common.next_buf = 0;

	dscr->common.interface = 0;

	dscr->pkt_len = reserve_len + net_pkt_get_len(pkt);
	dscr->offset = 11;
	/* TODO */
	dscr->tx_ctrl.sw_rate = (type == SPRDWL_TYPE_DATA_SPECIAL ? 1 : 0);
	dscr->tx_ctrl.wds = 0;
	/*TBD*/ dscr->tx_ctrl.swq_flag = 0;
	/*TBD*/ dscr->tx_ctrl.rsvd = 0;
	/*TBD*/ dscr->tx_ctrl.pcie_mh_readcomp = 1;
	dscr->buffer_info.msdu_tid = 0;
	dscr->buffer_info.mac_data_offset = 0;
	dscr->buffer_info.sta_lut_idx = 0;
	dscr->buffer_info.encrypt_bypass = 0;
	dscr->buffer_info.ap_buf_flag = 1;
	dscr->tx_ctrl.checksum_offload = 0;
	dscr->tx_ctrl.checksum_type = 0;
	dscr->tcp_udp_header_offset = 0;

	return 0;
}

static int uwp_iface_tx(struct net_if *iface, struct net_pkt *pkt)
{
	struct wifi_priv *priv = DEV_DATA(iface->if_dev->dev);
	struct net_buf *frag;
	bool first_frag = true;
	u8_t *data_ptr;
	u16_t data_len;
	u16_t total_len;
	u32_t addr;
	u16_t max_len;

	wifi_tx_fill_msdu_dscr(priv, pkt, SPRDWL_TYPE_DATA, 0);

	total_len = net_pkt_ll_reserve(pkt) + net_pkt_get_len(pkt);

	SYS_LOG_DBG("wifi tx data: %d bytes, reserve: %d bytes",
		    total_len, net_pkt_ll_reserve(pkt));

	for (frag = pkt->frags; frag; frag = frag->frags) {
		if (first_frag) {
			data_ptr = net_pkt_ll(pkt);
			data_len = net_pkt_ll_reserve(pkt) + frag->len;
			first_frag = false;
		} else {
			data_ptr = frag->data;
			data_len = frag->len;

		}

		addr = (u32_t)data_ptr;
		/* FIXME Save pkt addr before payload. */
		uwp_save_addr_before_payload(addr, (void *)pkt);

		SPRD_AP_TO_CP_ADDR(addr);

		max_len = MTU + sizeof(struct tx_msdu_dscr)
			+ 14 /* link layer header length */;
		if (data_len > max_len) {
			SYS_LOG_ERR("Exceed max length %d data_len %d",
					max_len, data_len);
			return -1;
		}

		wifi_tx_data((void *)addr, data_len);
	}

	return 0;
}

static enum ethernet_hw_caps uwp_iface_get_capabilities(struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T | ETHERNET_LINK_100BASE_T;
}

static const struct net_wifi_mgmt_api uwp_api = {
	.iface_api.init			= uwp_iface_init,
	.iface_api.send			= uwp_iface_tx,
	.get_capabilities		= uwp_iface_get_capabilities,
	.open					= uwp_mgmt_open,
	.close					= uwp_mgmt_close,
	.scan                   = uwp_mgmt_scan,
	.get_station			= uwp_mgmt_get_station,
	.connect                = uwp_mgmt_connect,
	.disconnect             = uwp_mgmt_disconnect,
	.notify_ip              = uwp_mgmt_set_ip,
	.start_ap               = uwp_mgmt_start_ap,
	.stop_ap                = uwp_mgmt_stop_ap,
	.del_station			= uwp_mgmt_del_station,
};

#ifdef CONFIG_WIFI_UWP_USE_SRAM
static void wifi_mem_init(void)
{
	SYS_LOG_INF("power on sram start");

	unsigned int val;

	val = get_reg32(0x40130004); /* enable */
	val |= 0x220;
	put_reg32(val, 0x40130004);
	DELAY(1000);

	val = get_reg32(0x4083c088); /* power on WRAP */
	val &= ~(0x2);
	put_reg32(val, 0x4083c088);
	while (!(get_reg32(0x4083c00c) & (0x1 << 14))) {
	}

	val = get_reg32(0x4083c0a8);
	val &= ~(0x4);
	put_reg32(val, 0x4083c0a8);
	while (!(get_reg32(0x4083c00c) & (0x1 << 16))) {
	}

	val = get_reg32(0x4083c134); /* close MEM PD */
	val &= 0xffffff;
	put_reg32(val, 0x4083c134);

	val = get_reg32(0x4083c130);
	val &= 0xfffffff0;
	put_reg32(val, 0x4083c130);

	SYS_LOG_INF("power on sram done");
}
#else
#define wifi_mem_init(...)
#endif /* CONFIG_WIFI_UWP_USE_SRAM */

static int uwp_init(struct device *dev)
{
	int ret;
	struct wifi_priv *priv = DEV_DATA(dev);

	if (!strcmp(dev->config->name, CONFIG_WIFI_UWP_STA_NAME)) {
		priv->mode = WIFI_MODE_STA;
	} else if (!strcmp(dev->config->name, CONFIG_WIFI_UWP_AP_NAME)) {
		priv->mode = WIFI_MODE_AP;
	} else {
		SYS_LOG_ERR("Unknown WIFI DEV NAME: %s",
			    dev->config->name);
	}

	if (!priv->initialized) {
		if (priv->mode == WIFI_MODE_STA) {
			/* priv->connecting = false; */
			/* priv->connected = false; */

			wifi_mem_init();

			ret = cp_mcu_init();
			if (ret) {
				SYS_LOG_ERR("firmware download failed %i.",
						ret);
				return ret;
			}

			wifi_cmdevt_init();
			wifi_txrx_init(priv);
			wifi_irq_init();

			k_sleep(400); /* FIXME: workaround */
			ret = wifi_rf_init();
			if (ret) {
				SYS_LOG_ERR("wifi rf init failed.");
				return ret;
			}
		}

		/* CP does not need to do initialization twice
		 * because softap and sta are initialized at one time.
		 */
		priv->opened = false;
		priv->initialized = true;
	}

	SYS_LOG_DBG("UWP WIFI driver Initialized");

	return 0;
}

NET_DEVICE_INIT(uwp_sta, CONFIG_WIFI_UWP_STA_NAME,
		uwp_init, &uwp_wifi_sta_priv, NULL,
		CONFIG_WIFI_INIT_PRIORITY,
		&uwp_api,
		ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),
		MTU);

NET_DEVICE_INIT(uwp_ap, CONFIG_WIFI_UWP_AP_NAME,
		uwp_init, &uwp_wifi_ap_priv, NULL,
		CONFIG_WIFI_INIT_PRIORITY,
		&uwp_api,
		ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),
		MTU);
