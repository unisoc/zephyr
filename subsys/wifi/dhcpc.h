/*
 * @file
 * @brief DHCP header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_WIFIMGR_DHCPC)
void wifimgr_dhcp_start(void *handle);
void wifimgr_dhcp_stop(void *handle);
#else
#define wifimgr_dhcp_start(...)
#define wifimgr_dhcp_stop(...)
#endif
