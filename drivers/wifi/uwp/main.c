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
#include <flash.h>

#include "main.h"
#include "cmdevt.h"
#include "txrx.h"
#include "mem.h"
#include "sipc.h"
#include "uwp_hal.h"

#define MTU (1500)

#define SEC1 (1)
#define SEC2 (2)
#define WIFI_INI_OFFSET (DT_FLASH_AREA_HWPARAM_OFFSET + 0)
#define SEC1_LEN (328)
#define SEC2_LEN (1464)

#define L2_HEADER_LEN (sizeof(struct net_eth_hdr))

#define DEV_DATA(dev) \
	((struct wifi_priv *)(dev)->driver_data)

static struct wifi_priv uwp_wifi_priv_data;

static struct wifi_device *get_wifi_dev_by_dev(struct device *dev)
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
	u8_t *sec_buf;
	struct device *flash_dev;

	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	if (!flash_dev) {
		LOG_ERR("Could not find device %s", DT_FLASH_DEV_NAME);
		return -ENODEV;
	}

	sec_buf = k_malloc(SEC1_LEN + SEC2_LEN);
	if (!sec_buf) {
		LOG_ERR("No more mem");
		return -ENOMEM;
	}

	memset(sec_buf, 0, SEC1_LEN + SEC2_LEN);

	if (flash_read(flash_dev, WIFI_INI_OFFSET,
				sec_buf, SEC1_LEN + SEC2_LEN)) {
		LOG_ERR("Could not read wifi ini from flash");
		k_free(sec_buf);
		return -EIO;
	}

	LOG_HEXDUMP_DBG(sec_buf, SEC1_LEN, "SEC1");

	LOG_HEXDUMP_DBG(sec_buf + SEC1_LEN, SEC2_LEN, "SEC2");

	ret = wifi_cmd_load_ini(sec_buf, SEC1_LEN, SEC1);
	if (ret) {
		LOG_ERR("Download first section ini fail,ret = %d", ret);
		k_free(sec_buf);
		return ret;
	}

	ret = wifi_cmd_load_ini(sec_buf + SEC1_LEN, SEC2_LEN, SEC2);
	if (ret) {
		LOG_ERR("Download second section ini fail,ret = %d", ret);
		k_free(sec_buf);
		return ret;
	}

	k_free(sec_buf);

	LOG_DBG("Load wifi ini success.");

	return 0;
}

static int uwp_mgmt_get_capa(struct device *dev,
		union wifi_drv_capa *capa)
{
	struct wifi_device *wifi_dev;

	if (!dev || !capa) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	if (wifi_dev->mode == WIFI_MODE_AP) {
		capa->ap.max_ap_assoc_sta = wifi_dev->max_sta_num;
		capa->ap.max_acl_mac_addrs = wifi_dev->max_blacklist_num;
	} else if (wifi_dev->mode == WIFI_MODE_STA) {
		capa->sta.max_rtt_peers = wifi_dev->max_rtt_num;
	}

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
		wifi_alloc_rx_buf(CONFIG_WIFI_UWP_RX_BUF_COUNT);
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

	return 0;
}

static int uwp_mgmt_start_ap(struct device *dev,
			     struct wifi_drv_start_ap_params *params,
				 new_station_evt_t cb)
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

	return wifi_cmd_stop_ap(wifi_dev);
}

static int uwp_mgmt_del_station(struct device *dev, char *mac)
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
		LOG_WRN("Improper mode %d to del sta.",
				wifi_dev->mode);
		return -EINVAL;
	}

	/* Reason code not used by wifimgr. */
	return wifi_cmd_del_sta(wifi_dev, mac,
			0/* reason_code */);
}

static int uwp_mgmt_set_mac_acl(struct device *dev,
		char subcmd, unsigned char acl_nr,
		char acl_mac_addrs[][NET_LINK_ADDR_MAX_LENGTH])
{
	struct wifi_device *wifi_dev;
	u8_t sub_type;

	if (!dev) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	if (wifi_dev->mode != WIFI_MODE_AP) {
		LOG_WRN("Improper mode %d to add mac acl.",
				wifi_dev->mode);
		return -EINVAL;
	}

	switch (subcmd) {
	case WIFI_DRV_BLACKLIST_ADD:
		sub_type = ADD_MAC_ACL;
		break;
	case WIFI_DRV_BLACKLIST_DEL:
		sub_type = DEL_MAC_ACL;
		break;
	default:
		LOG_WRN("Unknown subcmd %d", subcmd);
		return -EINVAL;
	}

	return wifi_cmd_set_blacklist(wifi_dev, sub_type,
			acl_nr, (u8_t **)acl_mac_addrs);
}

static int uwp_mgmt_session_req(struct device *dev,
		struct wifi_drv_rtt_request *param, rtt_result_evt_t cb)
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
		LOG_WRN("Improper mode %d to do session req.",
				wifi_dev->mode);
		return -EINVAL;
	}

	wifi_dev->rtt_result_cb = cb;

	return wifi_cmd_session_request(wifi_dev, param);
}

static int uwp_mgmt_hw_test(struct device *dev,
		char *t_buf, unsigned int t_len,
		char *r_buf, unsigned int *r_len)
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

	return wifi_cmd_hw_test(wifi_dev, t_buf,
			t_len, r_buf, r_len);
}

static int uwp_mgmt_scan(struct device *dev,
		struct wifi_drv_scan_params *params,
		scan_result_evt_t cb)
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

	wifi_dev->scan_result_cb = cb;

	return wifi_cmd_scan(wifi_dev, params);

}

static int uwp_mgmt_get_station(struct device *dev,
		signed char *rssi)
{
	struct wifi_device *wifi_dev;

	if (!dev || !rssi) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	if (wifi_dev->mode != WIFI_MODE_STA) {
		LOG_WRN("Improper mode %d to get sta.",
				wifi_dev->mode);
		return -EINVAL;
	}

	return wifi_cmd_get_sta(wifi_dev, rssi);
}

static int uwp_mgmt_connect(struct device *dev,
			    struct wifi_drv_connect_params *params,
				connect_evt_t con_cb,
				disconnect_evt_t discon_cb)
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
		disconnect_evt_t cb)
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

static int uwp_mgmt_notify_ip_acquired(struct device *dev,
		char *ip, unsigned char len)
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

	return wifi_cmd_notify_ip_acquired(wifi_dev, ip, len);
}

static void uwp_iface_init(struct net_if *iface)
{
	struct wifi_device *wifi_dev;
	struct device *dev;

	if (!iface) {
		return;
	}

	dev = net_if_get_device(iface);
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
			   void *data, u16_t total_len, u8_t type,
			   u8_t offset)
{
	u32_t addr = 0;
	struct tx_msdu_dscr *dscr = (struct tx_msdu_dscr *)data;

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

	dscr->pkt_len = total_len;
	dscr->offset = 11;
	/* TODO */
	dscr->tx_ctrl.sw_rate = (type == SPRDWL_TYPE_DATA_SPECIAL ? 1 : 0);
	dscr->tx_ctrl.wds = 0;
	/* TODO */ dscr->tx_ctrl.swq_flag = 0;
	/* TODO */ dscr->tx_ctrl.rsvd = 0;
	/* TODO */ dscr->tx_ctrl.pcie_mh_readcomp = 1;
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

static int uwp_tx(struct device *dev, struct net_pkt *pkt)
{
	struct wifi_device *wifi_dev;
	u8_t *data_ptr = NULL;
	u16_t total_len;
	u32_t addr;
	u16_t max_len;

	if (!dev || !pkt) {
		return -EINVAL;
	}

	wifi_dev = get_wifi_dev_by_dev(dev);
	if (!wifi_dev) {
		LOG_ERR("Unable to find wifi dev by dev %p", dev);
		return -EINVAL;
	}

	if (!wifi_dev->opened
			&& wifi_dev->mode == WIFI_MODE_AP) {
		LOG_WRN("AP mode %d not opened.", wifi_dev->mode);
		return -EWOULDBLOCK;
	} else if (!wifi_dev->connected
			&& wifi_dev->mode == WIFI_MODE_STA) {
		LOG_WRN("STA mode %d not connected.", wifi_dev->mode);
		return -EWOULDBLOCK;
	}

	data_ptr = get_data_buf(TX_BUF_LIST);
	if (!data_ptr) {
		LOG_ERR("Cannot alloc tx buf.");
		return -ENOMEM;
	}

	total_len = net_pkt_get_len(pkt);

	wifi_tx_fill_msdu_dscr(wifi_dev, data_ptr,
			total_len, SPRDWL_TYPE_DATA, 0);

	net_pkt_read(pkt, data_ptr + sizeof(struct tx_msdu_dscr),
			total_len);

	total_len += sizeof(struct tx_msdu_dscr);

	LOG_DBG("wifi tx data: %d bytes", total_len);

	LOG_HEXDUMP_DBG(data_ptr, total_len, "tx data");

	addr = (u32_t)data_ptr;

	SPRD_AP_TO_CP_ADDR(addr);

	max_len = MTU + sizeof(struct tx_msdu_dscr)
		+ L2_HEADER_LEN;
	if (total_len > max_len) {
		LOG_ERR("Exceed max length %d data_len %d",
				max_len, total_len);
		return -EINVAL;
	}

	if (wifi_tx_data((void *)addr, total_len)) {
		LOG_WRN("Send data failed.");
		return -EIO;
	}

	return 0;
}


static const struct wifi_drv_api uwp_api = {
	.eth_api.iface_api.init = uwp_iface_init,
	.eth_api.send           = uwp_tx,
	.get_capa               = uwp_mgmt_get_capa,
	.open                   = uwp_mgmt_open,
	.close                  = uwp_mgmt_close,
	.scan                   = uwp_mgmt_scan,
	.get_station            = uwp_mgmt_get_station,
	.connect                = uwp_mgmt_connect,
	.disconnect             = uwp_mgmt_disconnect,
	.notify_ip              = uwp_mgmt_notify_ip_acquired,
	.start_ap               = uwp_mgmt_start_ap,
	.stop_ap                = uwp_mgmt_stop_ap,
	.del_station            = uwp_mgmt_del_station,
	.set_mac_acl            = uwp_mgmt_set_mac_acl,
	.rtt_req                = uwp_mgmt_session_req,
	.hw_test                = uwp_mgmt_hw_test,
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

#ifdef CONFIG_SOC_UWP5661
		wifi_irq_init();
#endif

		k_sleep(400); /* FIXME: workaround */
		wifi_rf_init();

		wifi_cmd_get_cp_info(priv);

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
