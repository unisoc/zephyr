#ifndef __BLUETOOTH_THROUGHPUT_H__
#define __BLUETOOTH_THROUGHPUT_H__

#define BLE_TRANS_IDLE                  0
#define BLE_TRANS_DEVICE_2_HOST         1
#define BLE_TRANS_DEVICE_2_HOST_END     2
#define BLE_TRANS_HOST_2_DEVICE         3

#define BLE_CMD_HEADER               0xF0
#define BLE_CMD_DATA_2_HOST          0x8E
#define BLE_CMD_DATA_2_HOST_START    0x0D
#define BLE_CMD_DATA_2_HOST_END      0x4D
#define BLE_CMD_RESULT_2_HOST        0x0C
#define BLE_CMD_DATA_2_DEVICE_START  0x8D
#define BLE_CMD_DATA_2_DEVICE_END    0xCD

#define BLE_PARAMETER_CONNECT_INTVL_US     0
#define BLE_TOTAL_DATA_LEN_2_HOST          2000

#define BLE_TEST_DEVICE_2_HOST_DATA_TIMER    K_MSEC(20)

typedef enum {
    DATA_RAW,
    ALL_ZERO,
    ALL_ONE,
    RAND
}e_data_type;

typedef struct {
    u8_t status;
    u8_t packet_total;
    u16_t data_mtu;
    int data_len;
    u8_t buf[10];
    int packets_num;
    int packets_len;
    int data_type;
}ble_trans_type;

typedef struct {
    struct k_delayed_work timer;
    ble_trans_type        ble_trans;
}conn_dev_type;

void throughput_init(void);
void throughput_notify(const void *data, u16_t len);

#endif
