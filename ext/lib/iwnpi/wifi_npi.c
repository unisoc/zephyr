/*
* Authors:<jessie.xin@spreadtrum.com>
* Owner:
*/

#include "wifi_npi.h"

int npi_wifi_iface_init(struct device **dev)
{
	char *devname = WIFINPI_DEV_NAME_STA;

	ENG_LOG("enter %s\n", __func__);

	if (!devname)
		return NULL;

	*dev = device_get_binding(devname);
//	ENG_LOG("%s dev addr is %p\n", __func__, *dev);
	if (!(*dev)) {
		ENG_LOG("%s() failed to get device %s!\n", __func__, devname);
		return -1;
	}

	ENG_LOG("leaving %s\n", __func__);
	return 0;
}

int npi_open_station(struct device *dev)
{
	struct net_wifi_mgmt_api *mgmt_api =
	    (struct net_wifi_mgmt_api *)dev->driver_api;

	ENG_LOG("enter %s\n", __func__);

	if (!mgmt_api->open)
		return -EIO;

	ENG_LOG("leaving %s\n", __func__);
	return mgmt_api->open(dev);
}

int npi_close_station(struct device *dev)
{
	struct net_wifi_mgmt_api *mgmt_api =
	    (struct net_wifi_mgmt_api *)dev->driver_api;

	ENG_LOG("enter %s\n", __func__);

	if (!mgmt_api->close)
		return -EIO;

	ENG_LOG("leaving %s\n", __func__);
	return mgmt_api->close(dev);
}
