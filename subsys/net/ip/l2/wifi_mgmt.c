/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_WIFI_MGMT)
#define SYS_LOG_DOMAIN "net/wifi_mgmt"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>

#include <net/net_core.h>
#include <net/net_if.h>
#include <net/wifi_mgmt.h>

static int wifi_open(u32_t mgmt_request, struct net_if *iface,
		     void *data, size_t len)
{
	struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_api *mgmt_api =
		(struct net_wifi_mgmt_api *) dev->driver_api;

	if (mgmt_api->open == NULL)
		return -EIO;

	return mgmt_api->open(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_OPEN, wifi_open);

static int wifi_close(u32_t mgmt_request, struct net_if *iface,
			void *data, size_t len)
{
	struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_api *mgmt_api =
		(struct net_wifi_mgmt_api *) dev->driver_api;

	if (mgmt_api->close == NULL)
		return -EIO;

	return mgmt_api->close(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CLOSE, wifi_close);

static int wifi_connect(u32_t mgmt_request, struct net_if *iface,
			void *data, size_t len)
{
	struct wifi_connect_req_params *params =
		(struct wifi_connect_req_params *)data;
	struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_api *mgmt_api =
		(struct net_wifi_mgmt_api *) dev->driver_api;

	SYS_LOG_DBG("%s %u %u %u %s %u",
		    params->ssid, params->ssid_length,
		    params->channel, params->security,
		    params->psk, params->psk_length);

	if ((params->security > WIFI_SECURITY_TYPE_PSK) ||
	    (params->ssid_length > WIFI_SSID_MAX_LEN) ||
	    (params->ssid_length == 0) ||
	    ((params->security == WIFI_SECURITY_TYPE_PSK) &&
	     ((params->psk_length < 8) || (params->psk_length > 64) ||
	      (params->psk_length == 0) || !params->psk)) ||
	    ((params->channel != WIFI_CHANNEL_ANY) &&
	     (params->channel > WIFI_CHANNEL_MAX)) ||
	    !params->ssid) {
		return -EINVAL;
	}

	if (mgmt_api->connect == NULL)
		return -EIO;

	return mgmt_api->connect(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT, wifi_connect);

static void _scan_result_cb(struct net_if *iface, int status,
			    struct wifi_scan_result *entry)
{
	if (!iface) {
		return;
	}

	if (!entry) {
		struct wifi_status scan_status = {
			.status = status,
		};

		net_mgmt_event_notify_with_info(NET_EVENT_WIFI_SCAN_DONE,
						iface, &scan_status,
						sizeof(struct wifi_status));
		return;
	}

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_SCAN_RESULT, iface,
					entry, sizeof(struct wifi_scan_result));
}

static int wifi_scan(u32_t mgmt_request, struct net_if *iface,
		     void *data, size_t len)
{
	struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_api *mgmt_api =
		(struct net_wifi_mgmt_api *) dev->driver_api;

	if (mgmt_api->scan == NULL)
		return -EIO;

	return mgmt_api->scan(dev, _scan_result_cb);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_SCAN, wifi_scan);

static int wifi_get_station(u32_t mgmt_request, struct net_if *iface,
			    u8_t *data, size_t len)
{
	struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_api *mgmt_api =
		(struct net_wifi_mgmt_api *) dev->driver_api;

	if (data == NULL)
		return -EINVAL;

	if (mgmt_api->get_station == NULL)
		return -EIO;

	return mgmt_api->get_station(dev, data);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_GET_STATION,
		wifi_get_station);

static int wifi_disconnect(u32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_api *mgmt_api =
		(struct net_wifi_mgmt_api *) dev->driver_api;

	if (mgmt_api->disconnect == NULL)
		return -EIO;

	return mgmt_api->disconnect(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_DISCONNECT, wifi_disconnect);

static int wifi_start_ap(u32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_api *mgmt_api =
		(struct net_wifi_mgmt_api *) dev->driver_api;
	struct wifi_start_ap_req_params *params =
		(struct wifi_start_ap_req_params *)data;

	if ((params->security > WIFI_SECURITY_TYPE_PSK) ||
	    (params->ssid_length > WIFI_SSID_MAX_LEN) ||
	    (params->ssid_length == 0) ||
	    ((params->security == WIFI_SECURITY_TYPE_PSK) &&
	     ((params->psk_length < 8) || (params->psk_length > 64) ||
	      (params->psk_length == 0) || !params->psk)) ||
	    ((params->channel != WIFI_CHANNEL_ANY) &&
	     (params->channel > WIFI_CHANNEL_MAX)) ||
	    !params->ssid) {
		return -EINVAL;
	}

	if (mgmt_api->start_ap == NULL)
		return -EIO;

	return mgmt_api->start_ap(dev, params);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_START_AP, wifi_start_ap);

static int wifi_stop_ap(u32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_api *mgmt_api =
		(struct net_wifi_mgmt_api *) dev->driver_api;

	if (mgmt_api->stop_ap == NULL)
		return -EIO;

	return mgmt_api->stop_ap(dev);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_STOP_AP, wifi_stop_ap);

static int wifi_del_station(u32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	struct device *dev = net_if_get_device(iface);
	struct net_wifi_mgmt_api *mgmt_api =
		(struct net_wifi_mgmt_api *) dev->driver_api;
	u8_t *mac = (u8_t *)data;

	if (mac == NULL)
		return -EINVAL;

	if (mgmt_api->del_station == NULL)
		return -EIO;

	return mgmt_api->del_station(dev, mac);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_DEL_STATION,
		wifi_del_station);


void wifi_mgmt_raise_connect_result_event(struct net_if *iface, int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_CONNECT_RESULT,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}

void wifi_mgmt_raise_disconnect_result_event(struct net_if *iface, int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_DISCONNECT_RESULT,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}

void wifi_mgmt_raise_new_station_event(struct net_if *iface,
		struct wifi_new_station_params *params)
{
	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_NEW_STATION,
					iface, params,
					sizeof(struct wifi_new_station_params));
}
