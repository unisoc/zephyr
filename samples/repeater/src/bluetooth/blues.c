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

#include <i2c.h>

#include "../../../../drivers/bluetooth/unisoc/uki_utlis.h"
#include "../../../../drivers/bluetooth/unisoc/uki_config.h"
#include "blues.h"
#include "throughput.h"
#include "wifi_manager_service.h"
#include "mesh.h"

#define DEVICE_NAME		CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN		(sizeof(DEVICE_NAME) - 1)

blues_config_t  blues_config;
static const conf_entry_t blues_config_table[] = {
	CONF_ITEM_TABLE(role, 0, blues_config, 1),
	CONF_ITEM_TABLE(address, 0, blues_config, 6),
	CONF_ITEM_TABLE(auto_run, 0, blues_config, 1),
	CONF_ITEM_TABLE(mesh_proved, 0, blues_config, 1),
	CONF_ITEM_TABLE(profile_health_enabled, 0, blues_config, 1),
	CONF_ITEM_TABLE(profile_light_enabled, 0, blues_config, 1),
	CONF_ITEM_TABLE(net_key, 0, blues_config, 16),
	CONF_ITEM_TABLE(device_key, 0, blues_config, 16),
	CONF_ITEM_TABLE(app_key, 0, blues_config, 16),
	CONF_ITEM_TABLE(node_address, 0, blues_config, 1),
	CONF_ITEM_TABLE(firmware_log_mode, 0, blues_config, 1),

	CONF_ITEM_TABLE(ali_mesh, 0, blues_config, 1),
	CONF_ITEM_TABLE(ali_pid, 0, blues_config, 1),
	CONF_ITEM_TABLE(ali_mac, 0, blues_config, 6),
	CONF_ITEM_TABLE(ali_secret, 0, blues_config, 16),

	{0, 0, 0, 0, 0}
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};

static struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

extern void get_mac_address(char *addr);
char bt_name[32] = {0};

static void change_device_name(void)
{
	uint8_t addr[6];
	char mac[10] = {0};
	int i;
	char hex_str[]= "0123456789ABCDEF";

	get_mac_address(addr);
	strcat(bt_name, DEVICE_NAME);
	strcat(bt_name, "_");

	for (i = 0; i < 2; i++) {
		mac[(i * 2) + 0] = hex_str[(addr[1-i] >> 4) & 0x0F];
		mac[(i * 2) + 1] = hex_str[(addr[1-i]     ) & 0x0F];
	}

	strcat(bt_name, mac);

	BTI("bt_name = %s,len = %d\n", bt_name,strlen(bt_name));
	sd[0].data = bt_name;
	sd[0].data_len = strlen(bt_name);
}

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	throughput_init();
#ifdef CONFIG_BT_WIFIMGR_SERVICE
	wifi_manager_service_init();
#endif
	change_device_name();

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

	memset(&blues_config, 0, sizeof(blues_config_t));
	vnd_load_configure(BT_CONFIG_INFO_FILE, &blues_config_table[0], 1);

	if (blues_config.role == DEVICE_ROLE_MESH
		&& blues_config.auto_run)
		mesh_init();

	if (blues_config.role == DEVICE_ROLE_BLUES
		&& blues_config.auto_run)
		cmd_init(0,NULL);
}
#if 0
static int wifi_test(int argc, char *argv[])
{
	if (argc < 2) {
		printk("%s, argc: %d", __func__, argc);
		return -1;
	}
    char data[255]={0};
	if(!strcmp(argv[1], "open")) {
		wifimgr_open(data);
	}else if(!strcmp(argv[1], "close")) {
        wifimgr_close(data);
    }else if(!strcmp(argv[1], "connect")) {
        data[0]=16;
        data[1]=0;
        data[2]=10;
        data[3]=0;
        data[4]='M';
        data[5]='C';
        data[6]='U';
        data[7]='_';
        data[8]='M';
        data[9]='O';
        data[10]='D';
        data[11]='U';
        data[12]='L';
        data[13]='E';
        data[14]=0;
        data[15]=0;
        data[16]=0;
        data[17]=0;
        wifimgr_set_conf_and_connect(data);
    }else if(!strcmp(argv[1], "disconnect")) {
        wifimgr_disconnect(data);
    }else if(!strcmp(argv[1], "status")) {
        wifimgr_get_status(data);
    }else if(!strcmp(argv[1], "conf")) {
        wifimgr_get_conf(data);
    }else if(!strcmp(argv[1], "scan")) {
        wifimgr_do_scan(3);
    }
	return 0;
}
#endif
static int cmd_btmac(int argc, char *argv[])
{
    uint8_t addr[6];
    get_mac_address(addr);
    BTI("Current bt addr = %02X:%02X:%02X:%02X:%02X:%02X\n", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
    return 0;
}


static int cmd_test(int argc, char *argv[])
{
	struct device *i2c = device_get_binding("uwp56xx_i2c");

	u8_t data[] = {0x55};
	struct i2c_msg msgs[1];


	/* Setup I2C messages */

	/* Send the address to read */
	msgs[0].buf = data;
	msgs[0].len = sizeof(data);
	msgs[0].flags = I2C_MSG_WRITE;

	return i2c_transfer(i2c, &msgs[0], 1, 0x34);
}



static const struct shell_cmd blues_commands[] = {
	{ "init", cmd_init, NULL },
	{ "vlog", cmd_vlog, NULL },
	{ "slog", cmd_slog, NULL },
	{ "mesh", cmd_mesh, NULL },
	{ "led", cmd_led, NULL },
	{ "test", cmd_test, NULL },
#if 0
	{ "wifi", wifi_test, NULL },
#endif
	{ "btmac", cmd_btmac, NULL },
	{ NULL, NULL, NULL}
};

SHELL_REGISTER("blues", blues_commands);

