/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/sys_log.h>
#include <string.h>
#include <sipc.h>
#include <sblock.h>

#include "wifi_main.h"

int wifi_ipc_create_channel(int ch, void (*callback)(int ch))
{
	int ret = 0;

	if (callback == NULL) {
		SYS_LOG_ERR("invalid callback.");
		return -1;
	}

	switch (ch) {
	case SMSG_CH_WIFI_CTRL:
		ret = sblock_create(0, ch,
				CTRLPATH_TX_BLOCK_NUM,
				CTRLPATH_TX_BLOCK_SIZE,
				CTRLPATH_RX_BLOCK_NUM,
				CTRLPATH_RX_BLOCK_SIZE);
		break;
	case SMSG_CH_WIFI_DATA_NOR:
		ret = sblock_create(0, ch,
				DATAPATH_NOR_TX_BLOCK_NUM,
				DATAPATH_NOR_TX_BLOCK_SIZE,
				DATAPATH_NOR_RX_BLOCK_NUM,
				DATAPATH_NOR_RX_BLOCK_SIZE);
		break;
	case SMSG_CH_WIFI_DATA_SPEC:
		ret = sblock_create(0, ch,
				DATAPATH_SPEC_TX_BLOCK_NUM,
				DATAPATH_SPEC_TX_BLOCK_SIZE,
				DATAPATH_SPEC_RX_BLOCK_NUM,
				DATAPATH_SPEC_RX_BLOCK_SIZE);
		break;
	default:
		ret = -1;
		break;
	}

	if (ret < 0) {
		SYS_LOG_ERR("ipc create channel %d failed.", ch);
		return ret;
	}

	ret = sblock_register_callback(ch, callback);
	if (ret < 0) {
		SYS_LOG_ERR("register ipc callback failed");
		return ret;
	}

	return ret;
}

int wifi_ipc_send(int ch, int prio, void *data, int len, int offset)
{
	int ret;
	struct sblock blk;

	ret = sblock_get(0, ch, &blk, 0);
	if (ret != 0) {
		SYS_LOG_ERR("get block error: %d", ch);
		return -1;
	}
	SYS_LOG_DBG("IPC Channel %d Send data:", ch);
	memcpy(blk.addr + BLOCK_HEADROOM_SIZE + offset, data, len);

	blk.length = len + offset;
	ret = sblock_send(0, ch, prio, &blk);

	return ret;
}

int wifi_ipc_recv(int ch, u8_t *data, int *len, int offset)
{
	int ret;
	struct sblock blk;

	ret = sblock_receive(0, ch, &blk, 0);
	if (ret) {
		return ret;
	}

	memcpy(data, blk.addr, blk.length);
	*len = blk.length;

	SYS_LOG_DBG("IPC Channel %d Get data:", ch);

	sblock_release(0, ch, &blk);

	return ret;
}

