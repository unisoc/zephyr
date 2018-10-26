#ifndef _WIFI_NPI_H_
#define _WIFI_NPI_H_

#include"wlnpi.h"

#define WIFINPI_DEV_NAME_STA	"UWP_STA"

int npi_wifi_iface_init(struct device **dev);
int npi_open_station(struct device *dev);
int npi_close_station(struct device *dev);

#endif
