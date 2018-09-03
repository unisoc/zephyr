/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <shell/shell.h>


#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>


#include "../../../../drivers/bluetooth/unisoc/uki_utlis.h"
#include "../../../../drivers/bluetooth/unisoc/uki_config.h"
#include "blues.h"
#include "throughput.h"
#include "wifi_manager_service.h"
#include "mesh.h"

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

static blues_config_t  blues_config;
static const conf_entry_t blues_config_table[] = {
    CONF_ITEM_TABLE(role, 0, blues_config, 1),
    CONF_ITEM_TABLE(address, 0, blues_config, 6),
    CONF_ITEM_TABLE(auto_run, 0, blues_config, 1),
	CONF_ITEM_TABLE(net_key, 0, blues_config, 16),
	CONF_ITEM_TABLE(device_key, 0, blues_config, 16),
	CONF_ITEM_TABLE(app_key, 0, blues_config, 16),
	CONF_ITEM_TABLE(node_address, 0, blues_config, 6),
	{0, 0, 0, 0, 0}
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	throughput_init();
	wifi_manager_service_init();

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	printk("Advertising successfully started\n");
}

static int cmd_init(int argc, char *argv[])
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	return err;
}

static int cmd_vlog(int argc, char *argv[])
{
	if (argc < 2) {
		printk("%s, argc: %d", __func__, argc);
		return -1;
	}
	int level = strtoul(argv[1], NULL, 0);
	set_vendor_log_level(level);
	return 0;
}

static int cmd_slog(int argc, char *argv[])
{
	if (argc < 2) {
		printk("%s, argc: %d", __func__, argc);
		return -1;
	}
	int level = strtoul(argv[1], NULL, 0);
	set_stack_log_level(level);
	return 0;
}

void blues_init(void)
{
	BTD("%s\n", __func__);
	uki_config_init();

	memset(&blues_config, 0, sizeof(blues_config_t));
	vnd_load_configure(BT_CONFIG_INFO_FILE, &blues_config_table[0], 1);

	if (blues_config.role == DEVICE_ROLE_MESH
		&& blues_config.auto_run)
		mesh_init();
}

static const struct shell_cmd blues_commands[] = {
	{ "init", cmd_init, NULL },
	{ "vlog", cmd_vlog, NULL },
	{ "slog", cmd_slog, NULL },
	{ "mesh", cmd_mesh, NULL },
	{ "led", cmd_led, NULL },

	{ NULL, NULL, NULL}
};

SHELL_REGISTER("blues", blues_commands);

