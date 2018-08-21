/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include "throughput.h"

static struct bt_uuid_128 throughput_service_uuid = BT_UUID_INIT_128(
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xa0, 0xff, 0x00, 0x00);

static struct bt_uuid_128 tx_char_uuid = BT_UUID_INIT_128(
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xa2, 0xff, 0x00, 0x00);

static struct bt_uuid_128 rx_char_uuid = BT_UUID_INIT_128(
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xa1, 0xff, 0x00, 0x00);


static struct bt_gatt_ccc_cfg rx_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t rx_is_enabled;
conn_dev_type ble_conn_dev;

static u8_t throught_tx_data[] = "1234567890abcdefghijklmn";

static void rx_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                u16_t value)
{
    BTD("%s", __func__);
    rx_is_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static void reset_trans_data(void)
{
    BTD("%s", __func__);
    memset(&ble_conn_dev.ble_trans.buf, 0x0, sizeof(ble_conn_dev.ble_trans.buf));
    ble_conn_dev.ble_trans.data_len = 0;
    ble_conn_dev.ble_trans.packet_total = 0;
}

static void data_device_to_host(struct k_work *work)
{
    u8_t data_type = 0x00;
    int data_length = 0;
    u8_t* my_buf = NULL;
    BTD("%s", __func__);

    if (ble_conn_dev.ble_trans.packet_total >= ble_conn_dev.ble_trans.packets_num) {
        ble_conn_dev.ble_trans.buf[0] = BLE_CMD_HEADER;
        ble_conn_dev.ble_trans.buf[1] = BLE_CMD_DATA_2_HOST_END;
        throughput_notify(ble_conn_dev.ble_trans.buf, 2);

        ble_conn_dev.ble_trans.buf[0] = BLE_CMD_HEADER;
        ble_conn_dev.ble_trans.buf[1] = BLE_CMD_RESULT_2_HOST;
        ble_conn_dev.ble_trans.buf[2] = 3;
        ble_conn_dev.ble_trans.buf[3] = (u8_t)(ble_conn_dev.ble_trans.data_len);
        ble_conn_dev.ble_trans.buf[4] = (u8_t)(ble_conn_dev.ble_trans.data_len >> 8);
        ble_conn_dev.ble_trans.buf[5] = 0;
        ble_conn_dev.ble_trans.buf[6] = 0;
        ble_conn_dev.ble_trans.buf[7] = 0;
        ble_conn_dev.ble_trans.buf[8] = 0;
        throughput_notify(ble_conn_dev.ble_trans.buf, 9);
        reset_trans_data();
    } else {
        data_type = ble_conn_dev.ble_trans.data_type;
        data_length = ble_conn_dev.ble_trans.packets_len;
        BTD("%s ,data_type = 0x%x ; data_length = %d ; MTU = %d", __func__, data_type, data_length,
            ble_conn_dev.ble_trans.data_mtu);
        if ((data_length == 0)||(data_length > (ble_conn_dev.ble_trans.data_mtu - 3))) {
            data_length = ble_conn_dev.ble_trans.data_mtu -3;
        }
        my_buf =(u8_t*)k_malloc(data_length);
        int i;
        switch(data_type){
            case 0x00:
                memset(my_buf, 0x00, data_length);
                break;
            case 0x01:
                memset(my_buf, 0x01, data_length);
                break;
            case 0x02:
                memset(my_buf, 0x00, data_length);
                for(i = 0; i < data_length; i++){
                    if(i % 2 == 0){
                        my_buf[i] = 0x00;
                    }else{
                        my_buf[i] = 0x01;
                    }
                }
                break;
            case 0x03:
                memset(my_buf, 0x00, data_length);
                for(i = 0; i < data_length; i++){
                    my_buf[i] = 0;//rand();
                }
                break;
            default:
                memset(my_buf, 0x00, data_length);
                break;
        }
        my_buf[0] = (int)(ble_conn_dev.ble_trans.packet_total & 0xFF);
        my_buf[1] = (int)((ble_conn_dev.ble_trans.packet_total >> 8) & 0xFF);
        throughput_notify(my_buf, data_length);

        ble_conn_dev.ble_trans.packet_total++;
        ble_conn_dev.ble_trans.data_len += data_length;
        k_free(my_buf);
        k_delayed_work_submit(&ble_conn_dev.timer, BLE_TEST_DEVICE_2_HOST_DATA_TIMER);
    }
}

static ssize_t throughput_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            void *buf, u16_t len, u16_t offset)
{
    const char *value = attr->user_data;
    BTD("%s", __func__);
    return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
                strlen(value));
}

static ssize_t throughput_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            const void *buf, u16_t len, u16_t offset,
            u8_t flags)
{
    const u8_t *value = buf;
    u8_t header = 0;
    u8_t op_code = 0;

    BTD("%s, len = %d, offset = %d, flags = %d", __func__,len,offset,flags);

    if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
        return 0;
    }

    if(0 == offset){
        header = value[0];
        op_code = value[1];
        BTD("%s,header=0x%x,op_code=0x%x", __func__,header,op_code);
        if (BLE_CMD_HEADER == header) {
            if (BLE_CMD_DATA_2_HOST == op_code) {
                if (BLE_CMD_DATA_2_HOST_START == value[2]) {
                    if (len == 8) {
                        ble_conn_dev.ble_trans.packets_num = (int)(((value[3] & 0xff) << 8)|(value[4] & 0xff));
                        ble_conn_dev.ble_trans.packets_len = (int)(((value[5] & 0xff) << 8)|(value[6] & 0xff));
                        ble_conn_dev.ble_trans.data_type = value[7];
                    }
                    if (0 == ble_conn_dev.ble_trans.packets_num){
                        ble_conn_dev.ble_trans.packets_num = 50;
                    }
                    reset_trans_data();
                    k_delayed_work_cancel(&ble_conn_dev.timer);

                    ble_conn_dev.ble_trans.buf[0] = BLE_CMD_HEADER;
                    ble_conn_dev.ble_trans.buf[1] = BLE_CMD_DATA_2_HOST_START;
                    ble_conn_dev.ble_trans.buf[2] = (u8_t)(BLE_TOTAL_DATA_LEN_2_HOST & 0xff);
                    ble_conn_dev.ble_trans.buf[3] = (u8_t)(BLE_TOTAL_DATA_LEN_2_HOST >> 8);

                    throughput_notify(ble_conn_dev.ble_trans.buf, 4);

                    ble_conn_dev.ble_trans.status = BLE_TRANS_DEVICE_2_HOST;
                    ble_conn_dev.ble_trans.data_mtu = bt_gatt_get_mtu(conn);

                    k_delayed_work_submit(&ble_conn_dev.timer, BLE_TEST_DEVICE_2_HOST_DATA_TIMER);
                }
            }
            else if (BLE_CMD_DATA_2_DEVICE_START == op_code) {
                reset_trans_data();
                ble_conn_dev.ble_trans.status = BLE_TRANS_HOST_2_DEVICE;
                return len;
            }
            else if (BLE_CMD_DATA_2_DEVICE_END == op_code) {
                ble_conn_dev.ble_trans.buf[0] = BLE_CMD_HEADER;
                ble_conn_dev.ble_trans.buf[1] = BLE_CMD_RESULT_2_HOST;
                ble_conn_dev.ble_trans.buf[2] = ALL_ONE;
                ble_conn_dev.ble_trans.buf[3] = (u8_t)(ble_conn_dev.ble_trans.data_len);
                ble_conn_dev.ble_trans.buf[4] = (u8_t)(ble_conn_dev.ble_trans.data_len >> 8);
                ble_conn_dev.ble_trans.buf[5] = 0;
                ble_conn_dev.ble_trans.buf[6] = 0;
                ble_conn_dev.ble_trans.buf[7] = (u8_t)(BLE_PARAMETER_CONNECT_INTVL_US);
                ble_conn_dev.ble_trans.buf[8] = (u8_t)(BLE_PARAMETER_CONNECT_INTVL_US >> 8);
                BTD("%s,recieve packet_total = %d", __func__,ble_conn_dev.ble_trans.packet_total);
                ble_conn_dev.ble_trans.status = BLE_TRANS_IDLE;

                throughput_notify(ble_conn_dev.ble_trans.buf, 9);
                reset_trans_data();
            }
        }
    }
    if (BLE_TRANS_HOST_2_DEVICE == ble_conn_dev.ble_trans.status){
        ble_conn_dev.ble_trans.data_len += len;
        if(0 == offset){
            ble_conn_dev.ble_trans.packet_total++;
        }
    }
    return len;
}

/* Throughtput Service Declaration */
static struct bt_gatt_attr attrs[] = {
    BT_GATT_PRIMARY_SERVICE(&throughput_service_uuid),
    BT_GATT_CHARACTERISTIC(&tx_char_uuid.uuid, BT_GATT_CHRC_READ|BT_GATT_CHRC_WRITE|BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                    BT_GATT_PERM_READ|BT_GATT_PERM_WRITE|BT_GATT_PERM_PREPARE_WRITE, throughput_read, throughput_write, throught_tx_data),
    BT_GATT_CHARACTERISTIC(&rx_char_uuid.uuid, BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_NONE, NULL, NULL, NULL),
    BT_GATT_CCC(rx_ccc_cfg, rx_ccc_cfg_changed),
};

static struct bt_gatt_service throughput_svc = BT_GATT_SERVICE(attrs);

void throughput_init(void)
{
    BTD("%s", __func__);
    bt_gatt_service_register(&throughput_svc);
    k_delayed_work_init(&ble_conn_dev.timer, data_device_to_host);
}

void throughput_notify(const void *data, u16_t len)
{
    BTD("%s, len = %d", __func__, len);
    if (!rx_is_enabled) {
        BTD("%s, rx is not enabled", __func__);
        return;
    }

    bt_gatt_notify(NULL, &attrs[4], data, len);
}

