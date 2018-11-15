/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <logging/log.h>
#define LOG_MODULE_NAME wifi_uwp
#define LOG_LEVEL CONFIG_WIFI_LOG_LEVEL
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <net/net_pkt.h>
#include <net/net_if.h>
#include <net/ethernet.h>

#include "wifi_main.h"
#include "wifi_rf.h"
#include "wifi_cmdevt.h"
#include "wifi_txrx.h"
#include "sipc.h"
#include "uwp_hal.h"

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

static struct wifi_priv uwp_wifi_priv_data;


struct wifi_device *get_wifi_dev_by_dev(struct device *dev)
{
	struct wifi_device *wifi_dev = NULL;
	struct wifi_priv *priv = DEV_DATA(dev);

	for (int i = 0; i < MAX_WIFI_DEV_NUM; i++) {
		if (dev == (priv->wifi_dev[i]).dev) {
			wifi_dev = &(priv->wifi_dev[i]);
		}
	}

	return wifi_dev;
}

static int wifi_rf_init(void)
{
	int ret = 0;

	LOG_DBG("download the first section of config file");
	ret = wifi_cmd_load_ini(sec1_table, sizeof(sec1_table), SEC1);
	if (ret) {
		LOG_ERR("download first section ini fail,ret = %d", ret);
		return ret;
	}

	LOG_DBG("download the second section of config file");
	ret = wifi_cmd_load_ini(sec2_table, sizeof(sec2_table), SEC2);
	if (ret) {
		LOG_ERR("download second section ini fail,ret = %d", ret);
		return ret;
	}

	LOG_DBG("Load wifi ini success.");

	return 0;
}

static int uwp_mgmt_open(struct device *dev)
{
	int ret;
	struct wifi_priv *priv;
	struct wifi_device *wifi_dev;

	if (!dev) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	priv = DEV_DATA(dev);

	if (wifi_dev->opened) {
		return -EAGAIN;
	}

	ret = wifi_cmd_open(wifi_dev);
	if (ret) {
		return ret;
	}

	/* Open mode at first time */
	if (!priv->wifi_dev[WIFI_DEV_STA].opened
			&& !priv->wifi_dev[WIFI_DEV_AP].opened) {
		wifi_tx_empty_buf(TOTAL_RX_ADDR_NUM);
	}

	wifi_dev->opened = true;

	return 0;
}

static int uwp_mgmt_close(struct device *dev)
{
	int ret;
	struct wifi_priv *priv;
	struct wifi_device *wifi_dev;

	if (!dev) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	priv = DEV_DATA(dev);

	if (!wifi_dev->opened) {
		return -EAGAIN;
	}

	ret = wifi_cmd_close(wifi_dev);
	if (ret) {
		return ret;
	}

	wifi_dev->opened = false;

	/* Both are closed */
	if (!priv->wifi_dev[WIFI_DEV_STA].opened
			&& !priv->wifi_dev[WIFI_DEV_AP].opened) {
		wifi_release_rx_buf();
	}

	/* Flush all callbacks */
	if (wifi_dev->new_station_cb) {
		wifi_dev->new_station_cb = NULL;
	}

	if (wifi_dev->scan_result_cb) {
		wifi_dev->scan_result_cb = NULL;
	}

	if (wifi_dev->connect_cb) {
		wifi_dev->connect_cb = NULL;
	}

	if (wifi_dev->disconnect_cb) {
		wifi_dev->disconnect_cb = NULL;
	}

	return 0;
}

static int uwp_mgmt_start_ap(struct device *dev,
			     struct wifi_drv_start_ap_params *params,
				 new_station_t cb)
{
	struct wifi_device *wifi_dev;

	if (!dev || !params) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	if (wifi_dev->mode != WIFI_MODE_AP) {
		LOG_WRN("Improper mode %d to start_ap.",
				wifi_dev->mode);
		return -EINVAL;
	}

	if (wifi_dev->new_station_cb) {
		return -EAGAIN;
	}

	wifi_dev->new_station_cb = cb;

	return wifi_cmd_start_ap(wifi_dev, params);
}

static int uwp_mgmt_stop_ap(struct device *dev)
{
	struct wifi_device *wifi_dev;

	if (!dev) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	if (wifi_dev->mode != WIFI_MODE_AP) {
		LOG_WRN("Improper mode %d to stop_ap.",
				wifi_dev->mode);
		return -EINVAL;
	}

	if (wifi_dev->new_station_cb) {
		wifi_dev->new_station_cb = NULL;
	}

	return wifi_cmd_stop_ap(wifi_dev);
}

static int uwp_mgmt_del_station(struct device *dev,
				u8_t *mac)
{
	return 0;
}

static int uwp_mgmt_scan(struct device *dev,
		struct wifi_drv_scan_params *params,
		scan_result_cb_t cb)
{
	struct wifi_device *wifi_dev;

	if (!dev || !params) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	if (wifi_dev->mode != WIFI_MODE_STA) {
		LOG_WRN("Improper mode %d to scan.",
				wifi_dev->mode);
		return -EINVAL;
	}

	if (wifi_dev->scan_result_cb) {
		return -EAGAIN;
	}

	wifi_dev->scan_result_cb = cb;

	return wifi_cmd_scan(wifi_dev, params);

}

static int uwp_mgmt_get_station(struct device *dev,
		/* struct wifi_get_sta_req_params *params */
		u8_t *signal)
{
	return 0;
}

static int uwp_mgmt_connect(struct device *dev,
			    struct wifi_drv_connect_params *params,
				connect_cb_t con_cb,
				disconnect_cb_t discon_cb)
{
	struct wifi_device *wifi_dev;

	if (!dev || !params) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	if (wifi_dev->mode != WIFI_MODE_STA) {
		LOG_WRN("Improper mode %d to connect.",
				wifi_dev->mode);
		return -EINVAL;
	}

	if (wifi_dev->connected) {
		LOG_WRN("Connect again in connected.");
	}

	wifi_dev->connect_cb = con_cb;
	wifi_dev->disconnect_cb = discon_cb;

	return wifi_cmd_connect(wifi_dev, params);
}

static int uwp_mgmt_disconnect(struct device *dev,
		disconnect_cb_t cb)
{
	struct wifi_device *wifi_dev;

	if (!dev) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	if (wifi_dev->mode != WIFI_MODE_STA) {
		LOG_WRN("Improper mode %d to disconnect.",
				wifi_dev->mode);
		return -EINVAL;
	}

	if (!wifi_dev->connected) {
		LOG_WRN("Disconnect again in disconnected.");
	}

	wifi_dev->disconnect_cb = cb;

	return wifi_cmd_disconnect(wifi_dev);
}

static int uwp_mgmt_set_ip(struct device *dev, u8_t *ip, u8_t len)
{
	struct wifi_device *wifi_dev;

	if (!dev || !ip) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	if (wifi_dev->mode != WIFI_MODE_STA) {
		LOG_WRN("Improper mode %d to connect.",
				wifi_dev->mode);
		return -EINVAL;
	}

	return wifi_cmd_set_ip(wifi_dev, ip, len);
}

static void uwp_iface_init(struct net_if *iface)
{
	struct wifi_device *wifi_dev;
	struct device *dev;

	if (!iface) {
		return;
	}

	dev = iface->if_dev->dev;
	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return;
	}

	/* Net stack has not been fit for softap as of now. */
	if (wifi_dev->mode == WIFI_MODE_STA) {
		ethernet_init(iface);
	}

	net_if_set_link_addr(iface, wifi_dev->mac,
			sizeof(wifi_dev->mac),
			NET_LINK_ETHERNET);

	/* Store iface in wifi device */
	wifi_dev->iface = iface;

}

static int wifi_tx_fill_msdu_dscr(struct wifi_device *wifi_dev,
			   struct net_pkt *pkt, u8_t type, u8_t offset)
{
	u32_t addr = 0;
	u32_t reserve_len = net_pkt_ll_reserve(pkt);
	struct tx_msdu_dscr *dscr = NULL;

	net_pkt_set_ll_reserve(pkt,
			sizeof(struct tx_msdu_dscr) +
			net_pkt_ll_reserve(pkt));
	LOG_DBG("size msdu: %d", sizeof(struct tx_msdu_dscr));

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
	dscr->common.buffer_type = 0;

	if (wifi_dev->mode == WIFI_MODE_STA) {
		dscr->common.interface = WIFI_DEV_STA;
	} else if (wifi_dev->mode == WIFI_MODE_AP) {
		dscr->common.interface = WIFI_DEV_AP;
	}

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
	struct device *dev;
	struct net_buf *frag;
	struct wifi_device *wifi_dev;
	bool first_frag = true;
	u8_t *data_ptr;
	u16_t data_len;
	u16_t total_len;
	u32_t addr;
	u16_t max_len;

	if (!iface) {
		return -EINVAL;
	}

	dev = iface->if_dev->dev;
	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	if (!wifi_dev->opened
			&& wifi_dev->mode == WIFI_MODE_AP) {
		LOG_WRN("AP mode %d not opened.", wifi_dev->mode);
		net_pkt_unref(pkt);
		return -EWOULDBLOCK;
	} else if (!wifi_dev->connected
			&& wifi_dev->mode == WIFI_MODE_STA) {
		LOG_WRN("STA mode %d not connected.", wifi_dev->mode);
		net_pkt_unref(pkt);
		return -EWOULDBLOCK;
	}

	wifi_tx_fill_msdu_dscr(wifi_dev, pkt, SPRDWL_TYPE_DATA, 0);

	total_len = net_pkt_ll_reserve(pkt) + net_pkt_get_len(pkt);

	LOG_DBG("wifi tx data: %d bytes, reserve: %d bytes",
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
			LOG_ERR("Exceed max length %d data_len %d",
					max_len, data_len);
			return -EINVAL;
		}

		wifi_tx_data((void *)addr, data_len);
	}

	return 0;
}


static const struct wifi_drv_api uwp_api = {
	.eth_api.iface_api.init		= uwp_iface_init,
	.eth_api.iface_api.send		= uwp_iface_tx,
	.open						= uwp_mgmt_open,
	.close						= uwp_mgmt_close,
	.scan						= uwp_mgmt_scan,
	.get_station				= uwp_mgmt_get_station,
	.connect					= uwp_mgmt_connect,
	.disconnect					= uwp_mgmt_disconnect,
	.notify_ip					= uwp_mgmt_set_ip,
	.start_ap					= uwp_mgmt_start_ap,
	.stop_ap					= uwp_mgmt_stop_ap,
	.del_station				= uwp_mgmt_del_station,
};

static int uwp_init(struct device *dev)
{
	int ret;
	struct wifi_priv *priv;
	struct wifi_device *wifi_dev;

	if (!dev) {
		return -EINVAL;
	}

	priv = DEV_DATA(dev);

	if (!strcmp(dev->config->name, CONFIG_WIFI_STA_DRV_NAME)) {
		wifi_dev = &(priv->wifi_dev[WIFI_DEV_STA]);
		wifi_dev->mode = WIFI_MODE_STA;
		/* Store device in wifi device */
		wifi_dev->dev = dev;
	} else if (!strcmp(dev->config->name, CONFIG_WIFI_AP_DRV_NAME)) {
		wifi_dev = &(priv->wifi_dev[WIFI_DEV_AP]);
		wifi_dev->mode = WIFI_MODE_AP;
		/* Store device in wifi device */
		wifi_dev->dev = dev;
	} else {
		LOG_ERR("Unknown WIFI DEV NAME: %s",
			    dev->config->name);
	}

	if (!priv->initialized) {
		ret = uwp_mcu_init();
		if (ret) {
			LOG_ERR("Firmware download failed %i.",
					ret);
			return ret;
		}

		wifi_cmdevt_init();
		wifi_txrx_init(priv);
		wifi_irq_init();

		k_sleep(400); /* FIXME: workaround */
		ret = wifi_rf_init();
		if (ret) {
			LOG_ERR("wifi rf init failed.");
			return ret;
		}

		ret = wifi_cmd_get_cp_info(priv);
		if (ret) {
			LOG_ERR("Get cp info failed.");
			return ret;
		}

		priv->initialized = true;
	}

	LOG_DBG("UWP WIFI driver Initialized");

	return 0;
}

NET_DEVICE_INIT(uwp_ap, CONFIG_WIFI_AP_DRV_NAME,
		uwp_init, &uwp_wifi_priv_data, NULL,
		CONFIG_WIFI_INIT_PRIORITY,
		&uwp_api,
		ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),
		MTU);

NET_DEVICE_INIT(uwp_sta, CONFIG_WIFI_STA_DRV_NAME,
		uwp_init, &uwp_wifi_priv_data, NULL,
		CONFIG_WIFI_INIT_PRIORITY,
		&uwp_api,
		ETHERNET_L2, NET_L2_GET_CTX_TYPE(ETHERNET_L2),
		MTU);
