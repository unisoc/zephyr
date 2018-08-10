/* uart_h5.c - UART based Bluetooth driver */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr.h>

#include <board.h>
#include <init.h>
#include <uart.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include <misc/stack.h>
#include <misc/printk.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_driver.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_DRIVER)
#include "common/log.h"

#include "../util.h"

#define HCI_COMMAND_PKT		0x01
#define HCI_ACLDATA_PKT		0x02
#define HCI_SCODATA_PKT		0x03
#define HCI_EVENT_PKT		0x04
#define HCI_VENDOR_PKT		0xff

static void hexdump(const char *str, const u8_t *packet, size_t length)
{
	int n = 0;

	if (!length) {
		printk("%s zero-length signal packet\n", str);
		return;
	}

	while (length--) {
		if (n % 16 == 0) {
			printk("%s %08X ", str, n);
		}

		printk("%02X ", *packet++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printk("\n");
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		printk("\n");
	}
}

static int sipc_send(struct net_buf *buf)
{
	u8_t type;

	BT_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_CMD:
		type = HCI_COMMAND_PKT;
		break;
	case BT_BUF_ACL_OUT:
		type = HCI_ACLDATA_PKT;
		break;
	default:
		BT_ERR("Unknown packet type %u", bt_buf_get_type(buf));
		return -1;
	}

	return 0;
}


static int sipc_open(void)
{
	BT_DBG("");
	return 0;
}

static const struct bt_hci_driver drv = {
	.name		= "BT SIPC",
	.bus		= BT_HCI_DRIVER_BUS_UART,
	.open		= sipc_open,
	.send		= sipc_send,
};

static int _bt_sipc_init(struct device *unused)
{
	ARG_UNUSED(unused);

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(_bt_sipc_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
