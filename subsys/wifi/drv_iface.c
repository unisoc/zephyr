/*
 * @file
 * @brief Driver interface implementation for Zephyr
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/wifimgr_drv.h>

#include "os_adapter.h"
#include "drv_iface.h"

void *wifi_drv_init(char *devname)
{
	struct device *dev;
	struct net_if *iface;

	if (!devname)
		return NULL;

	dev = device_get_binding(devname);
	if (!dev) {
		wifimgr_err("failed to get device %s!\n", devname);
		return NULL;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		wifimgr_err("failed to get iface %s!\n", devname);
		return NULL;
	}

	return (void *)iface;
}

int wifi_drv_get_mac(void *iface, char *mac)
{
	if (!mac)
		return -EINVAL;

	memcpy(mac, net_if_get_link_addr(iface)->addr, WIFIMGR_ETH_ALEN);

	return 0;
}

int wifi_drv_get_capa(void *iface, struct wifi_drv_capa *capa)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->get_capa)
		return -EIO;

	return drv_api->get_capa(dev, capa);
}

int wifi_drv_open(void *iface)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->open)
		return -EIO;

	return drv_api->open(dev);
}

int wifi_drv_close(void *iface)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->close)
		return -EIO;

	return drv_api->close(dev);
}

static
void wifi_drv_event_iface_scan_result(void *iface, int status,
				      struct wifi_drv_scan_result_evt *entry)
{
	struct wifi_drv_scan_result_evt scan_res;
	struct wifi_drv_scan_done_evt scan_done;

	if (!entry) {
		scan_done.result = status;
		wifimgr_notify_event(WIFIMGR_EVT_SCAN_DONE, &scan_done,
				     sizeof(scan_done));
	} else {
		strcpy(scan_res.ssid, entry->ssid);
		strncpy(scan_res.bssid, entry->bssid, WIFIMGR_ETH_ALEN);
		scan_res.channel = entry->channel;
		scan_res.rssi = entry->rssi;
		scan_res.security = entry->security;

		wifimgr_notify_event(WIFIMGR_EVT_SCAN_RESULT, &scan_res,
				     sizeof(scan_res));
	}
}

int wifi_drv_scan(void *iface, unsigned char band, unsigned char channel)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;
	struct wifi_drv_scan_params params;

	if (!drv_api->scan)
		return -EIO;

	params.band = band;
	params.channel = channel;

	return drv_api->scan(dev, &params, wifi_drv_event_iface_scan_result);
}

static void wifi_drv_event_disconnect(void *iface, int status)
{
	struct wifi_drv_disconnect_evt disc;

	disc.reason_code = status;

	wifimgr_notify_event(WIFIMGR_EVT_DISCONNECT, &disc, sizeof(disc));
}

int wifi_drv_disconnect(void *iface)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->disconnect)
		return -EIO;

	return drv_api->disconnect(dev, wifi_drv_event_disconnect);
}

static void wifi_drv_event_connect(void *iface, int status, char *bssid,
				   unsigned char channel)
{
	struct wifi_drv_connect_evt conn;

	conn.status = status;
	if (bssid && !is_zero_ether_addr(bssid))
		memcpy(conn.bssid, bssid, WIFIMGR_ETH_ALEN);
	conn.channel = channel;

	wifimgr_notify_event(WIFIMGR_EVT_CONNECT, &conn, sizeof(conn));
}

int wifi_drv_connect(void *iface, char *ssid, char *bssid, char *passwd,
		     unsigned char channel)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;
	struct wifi_drv_connect_params params;

	if (!drv_api->connect)
		return -EIO;

	/* SSID is mandatory */
	params.ssid_length = strlen(ssid);
	if (!ssid || !params.ssid_length)
		return -EINVAL;
	params.ssid = ssid;

	/* BSSID is optional */
	if (bssid && is_zero_ether_addr(bssid))
		return -EINVAL;
	params.bssid = bssid;

	/* Passphrase only is valid for WPA/WPA2-PSK */
	params.psk_length = strlen(passwd);
	if (passwd && !params.psk_length)
		return -EINVAL;
	params.psk = passwd;

	return drv_api->connect(dev, &params, wifi_drv_event_connect,
				wifi_drv_event_disconnect);
}

int wifi_drv_get_station(void *iface, char *rssi)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->get_station)
		return -EIO;

	if (!rssi)
		return -EINVAL;

	return drv_api->get_station(dev, rssi);
}

int wifi_drv_notify_ip(void *iface, char *ipaddr, char len)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->notify_ip)
		return -EIO;

	if (!ipaddr)
		return -EINVAL;

	return drv_api->notify_ip(dev, ipaddr, len);
}

static void wifi_drv_event_new_station(void *iface, int status, char *mac)
{
	struct wifi_drv_new_station_evt new_sta;

	new_sta.is_connect = status;
	if (mac && !is_zero_ether_addr(mac))
		memcpy(new_sta.mac, mac, WIFIMGR_ETH_ALEN);

	wifimgr_notify_event(WIFIMGR_EVT_NEW_STATION, &new_sta,
			     sizeof(new_sta));
}

int wifi_drv_start_ap(void *iface, char *ssid, char *passwd,
		      unsigned char channel, unsigned char ch_width)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;
	struct wifi_drv_start_ap_params params;

	if (!drv_api->start_ap)
		return -EIO;

	/* SSID is mandatory */
	params.ssid_length = strlen(ssid);
	if (!ssid || !params.ssid_length)
		return -EINVAL;
	params.ssid = ssid;

	/* Passphrase only is valid for WPA/WPA2-PSK */
	params.psk_length = strlen(passwd);
	if (passwd && !params.psk_length)
		return -EINVAL;
	params.psk = passwd;

	/* Channel & channel width are optional */
	params.channel = channel;
	params.ch_width = ch_width;

	return drv_api->start_ap(dev, &params, wifi_drv_event_new_station);
}

int wifi_drv_stop_ap(void *iface)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->stop_ap)
		return -EIO;

	return drv_api->stop_ap(dev);
}

int wifi_drv_del_station(void *iface, char *mac)
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->del_station)
		return -EIO;

	if (!mac)
		return -EINVAL;

	return drv_api->del_station(dev, mac);
}

int wifi_drv_set_mac_acl(void *iface, char subcmd, unsigned char acl_nr,
			 char acl_mac_addrs[][NET_LINK_ADDR_MAX_LENGTH])
{
	struct device *dev = net_if_get_device((struct net_if *)iface);
	struct wifi_drv_api *drv_api = (struct wifi_drv_api *)dev->driver_api;

	if (!drv_api->set_mac_acl)
		return -EIO;

	if (!acl_mac_addrs)
		return -EINVAL;

	if (!acl_nr)
		return 0;

	return drv_api->set_mac_acl(dev, subcmd, acl_nr, acl_mac_addrs);
}
