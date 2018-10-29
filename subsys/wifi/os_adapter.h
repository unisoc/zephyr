/*
 * @file
 * @brief OS adapter
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _OS_ADAPTER_H_
#define _OS_ADAPTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <posix/sys/types.h>
#include <posix/pthread.h>
#include <posix/semaphore.h>
#include <posix/mqueue.h>

#include <misc/slist.h>

#include <logging/sys_log.h>

#include <net/net_if.h>

#undef LOG_ERR
#define LOG_ERR         SYS_LOG_LEVEL_ERROR
#define LOG_WARNING     SYS_LOG_LEVEL_WARNING
#define LOG_INFO        SYS_LOG_LEVEL_INFO
#define LOG_DEBUG       SYS_LOG_LEVEL_DEBUG

#define syslog(log_level, ...)	SYS_ ## log_level(__VA_ARGS__)
#define SYS_LOG_WARNING(...) SYS_LOG_WRN(__VA_ARGS__)
#define SYS_LOG_INFO(...) printk(__VA_ARGS__)
#define SYS_LOG_DEBUG(...) SYS_LOG_DBG(__VA_ARGS__)

/*
 * Copied from include/linux/...
 */

#define container_of CONTAINER_OF

#define malloc(size)		k_malloc(size)
#define free(ptr)		k_free(ptr)

#if !defined(CONFIG_NATIVE_POSIX_STDOUT_CONSOLE)
#define fflush(...)
#endif

#define wifimgr_slist_init(list)		sys_slist_init(list)
#define wifimgr_slist_peek_head(list)		sys_slist_peek_head(list)
#define wifimgr_slist_peek_next(node)		sys_slist_peek_next(node)
#define wifimgr_slist_prepend(list, node)	sys_slist_prepend(list, node)
#define wifimgr_slist_append(list, node)	sys_slist_append(list, node)
#define wifimgr_slist_remove_first(list)	sys_slist_get(list)
#define wifimgr_slist_remove(list, node)	sys_slist_find_and_remove(list, node)

typedef sys_snode_t wifimgr_snode_t;
typedef sys_slist_t wifimgr_slist_t;

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

#endif
