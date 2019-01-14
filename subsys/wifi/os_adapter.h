/*
 * @file
 * @brief OS adapter
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_OS_ADAPTER_H_
#define _WIFIMGR_OS_ADAPTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <posix/sys/types.h>
#include <posix/pthread.h>
#include <posix/semaphore.h>
#include <posix/mqueue.h>

#include <misc/slist.h>

#include <net/net_linkaddr.h>

#define WIFIMGR_ETH_ALEN	NET_LINK_ADDR_MAX_LENGTH

#define wifimgr_err(...)	LOG_ERR(__VA_ARGS__)
#define wifimgr_warn(...)	LOG_WRN(__VA_ARGS__)
#define wifimgr_info(...)	printk(__VA_ARGS__)
#define wifimgr_dbg(...)	LOG_DBG(__VA_ARGS__)

#define wifimgr_hexdump(...)	LOG_HEXDUMP_DBG(__VA_ARGS__, NULL)

/*
 * Copied from include/linux/...
 */

#define container_of CONTAINER_OF

#define malloc(size)		k_malloc(size)
#define free(ptr)		k_free(ptr)

#define wifimgr_slist_init(list)		sys_slist_init(list)
#define wifimgr_slist_peek_head(list)		sys_slist_peek_head(list)
#define wifimgr_slist_peek_next(node)		sys_slist_peek_next(node)
#define wifimgr_slist_prepend(list, node)	sys_slist_prepend(list, node)
#define wifimgr_slist_append(list, node)	sys_slist_append(list, node)
#define wifimgr_slist_merge(list_a, list_b)	sys_slist_merge_slist(list_a, list_b)
#define wifimgr_slist_remove_first(list)	sys_slist_get(list)
#define wifimgr_slist_remove(list, node)	sys_slist_find_and_remove(list, node)

typedef sys_snode_t wifimgr_snode_t;
typedef sys_slist_t wifimgr_slist_t;

typedef struct k_work wifimgr_work;

#define wifimgr_init_work(...)	k_work_init(__VA_ARGS__)
#define wifimgr_queue_work(...)	k_work_submit(__VA_ARGS__)

#ifndef MAC2STR
#define MAC2STR(m) (m)[0], (m)[1], (m)[2], (m)[3], (m)[4], (m)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

/**
 * is_zero_ether_addr - Determine if give Ethernet address is all zeros.
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is all zeroes.
 */
static inline bool is_zero_ether_addr(const char *addr)
{
	return (addr[0] | addr[1] | addr[2] | addr[3] | addr[4] | addr[5]) == 0;
}

/**
 * is_broadcast_ether_addr - Determine if the Ethernet address is broadcast
 * @addr: Pointer to a six-byte array containing the Ethernet address
 *
 * Return true if the address is the broadcast address.
 */
static inline bool is_broadcast_ether_addr(const char *addr)
{
	return (addr[0] & addr[1] & addr[2] & addr[3] & addr[4] & addr[5]) ==
	    0xff;
}

/**
 *
 * @brief Check whether the memory area is zeroed
 *
 * @return 0 if <m> == 0, else non-zero
 */
static inline int memiszero(const void *m, size_t n)
{
	const char *c = m;

	while ((--n > 0) && !(*c)) {
		c++;
	}

	return *c;
}

#endif
