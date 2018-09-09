#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>


#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <net/wifimgr_api.h>


#include "wifi_manager_service.h"
#include "../../../../drivers/bluetooth/unisoc/uki_utlis.h"

#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}

#define GET_STATUS_TIMEOUT  K_SECONDS(5)
#define SCAN_TIMEOUT        K_SECONDS(10)
#define SCAN_NUM    3


static struct bt_uuid_128 wifimgr_service_uuid = BT_UUID_INIT_128(
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xa5, 0xff, 0x00, 0x00);

static struct bt_uuid_128 wifimgr_char_uuid = BT_UUID_INIT_128(
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xa6, 0xff, 0x00, 0x00);


static u8_t wifimgr_long_value[256] = {0};

static struct bt_gatt_ccc_cfg wifimgr_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t wifimgr_is_enabled;

static struct k_sem get_status_sem;
static struct k_sem scan_sem;

static wifi_status_type cur_wifi_status = {};

static u8_t wait_disconnect_done = 0;
static int disconnect_result = -1;

static struct k_sem disconnect_sem;

struct wifimgr_ctrl_cbs *get_wifimgr_cbs(void);
static int scan_result = 0;

#if 0
struct bt_conn *cur_conn;
static void exchange_func(struct bt_conn *conn, u8_t err,
                struct bt_gatt_exchange_params *params)
{
    printk("Exchange %s\n", err == 0 ? "successful" : "failed");
}

static struct bt_gatt_exchange_params exchange_params;

int do_gatt_exchange_mtu()
{
    int err;

    if (!cur_conn) {
        printk("Not connected\n");
        return 0;
    }

    exchange_params.func = exchange_func;

    err = bt_gatt_exchange_mtu(cur_conn, &exchange_params);
    if (err) {
        printk("Exchange failed (err %d)\n", err);
    } else {
        printk("Exchange pending\n");
    }

    return 0;
}
#endif
static void wifimgr_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                u16_t value)
{
    BTD("%s\n", __func__);
    wifimgr_is_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

int wifimgr_check_wifi_status(void)
{
    BTD("%s\n", __func__);
    int ret = -1;

    if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->get_status) {
        ret = wifimgr_get_ctrl_ops(get_wifimgr_cbs())->get_status(WIFIMGR_IFACE_NAME_STA);
    } else {
        BTD("%s, get_status = NULL\n", __func__);
        goto error;
    }

    if(0 != ret) {
        BTD("%s, get_status fail,err = %d\n", __func__,ret);
        goto error;
    } else {
        if(0 != k_sem_take(&get_status_sem, GET_STATUS_TIMEOUT)){
            BTD("%s, take get_status_sem fail\n", __func__);
            ret = -1;
        }else {
            BTD("%s, get_status success\n", __func__);
            ret = 0;
        }
    }
error:
    return ret;
}

void wifimgr_set_conf_and_connect(const void *buf)
{
    BTD("%s\n", __func__);
    wifi_config_type conf;
    const u8_t *p = buf;
    u16_t vlen = 0;
    int ret = -1;
    char data[2] = {0};
    u8_t res_result = RESULT_SUCCESS;
    u16_t bssid_len = 0;
    u16_t pwd_len = 0;
    u16_t ssid_len = 0;

    memset(&conf, 0, sizeof(conf));
    vlen = sys_get_le16(p);
    p += 2;
    BTD("%s, AllDateLen = 0x%x\n", __func__, vlen);

    ssid_len = sys_get_le16(p);
    p += 2;
    BTD("%s, SsidDataLen = 0x%x\n", __func__, ssid_len);

    memcpy(conf.ssid,p,ssid_len);
    p+=ssid_len;
    BTD("%s, ssid = %s\n", __func__, conf.ssid);

    bssid_len = sys_get_le16(p);
    p += 2;
    BTD("%s, bSsidDataLen = 0x%x\n", __func__, bssid_len);

    memcpy(conf.bssid,p,bssid_len);
    p+=bssid_len;
    for(int i = 0; i < bssid_len; i++){
        BTD("bssid = 0x%02x\n", conf.bssid[i]);
    }

    pwd_len = sys_get_le16(p);
    p += 2;
    BTD("%s, PsdDataLen = 0x%x\n", __func__, pwd_len);

    memcpy(conf.passwd,p,pwd_len);
    p+=pwd_len;
    BTD("%s, passwd = %s\n", __func__, conf.passwd);

    if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->set_conf) {
        ret = wifimgr_get_ctrl_ops(get_wifimgr_cbs())->set_conf(WIFIMGR_IFACE_NAME_STA,
                                                                    (ssid_len > 0 ? conf.ssid : NULL),
                                                                    (bssid_len > 0 ? conf.bssid : NULL),
                                                                    (pwd_len > 0 ? conf.passwd : NULL),
                                                                    conf.band,
                                                                    conf.channel);
    } else {
        BTD("%s, set_conf = NULL\n", __func__);
        res_result = RESULT_FAIL;
        goto error;
    }

    if(0 != ret) {
        BTD("%s, set_conf fail,error = %d\n", __func__,ret);
        res_result = RESULT_FAIL;
        goto error;
    }

    ret = wifimgr_check_wifi_status();
    if(0 == ret){
        switch(cur_wifi_status.status){
            case WIFI_STATUS_NODEV:
                if(0 != wifimgr_do_open()) {
                    BTD("%s, open fail\n", __func__);
                    res_result = RESULT_FAIL;
                    goto error;
                }
            break;
            case WIFI_STATUS_READY:
            break;
            case WIFI_STATUS_CONNECTED:
                if(0 != wifimgr_do_disconnect(1)) {
                    BTD("%s, STATUS_CONNECTED and do_disconnect fail\n", __func__);
                    res_result = RESULT_FAIL;
                    goto error;
                }
            break;
            case WIFI_STATUS_SCANNING:
            case WIFI_STATUS_CONNECTING:
            case WIFI_STATUS_DISCONNECTING:
                BTD("%s,status = %d ,wifi is busy\n", __func__, cur_wifi_status.status);
                res_result = RESULT_FAIL;
                goto error;
            break;

            default:
                BTD("%s,status = %d not found\n", __func__, cur_wifi_status.status);
                res_result = RESULT_FAIL;
                goto error;
            break;
        }
    }else {
        res_result = RESULT_FAIL;
        goto error;
    }

    if(0 != wifimgr_do_scan(SCAN_NUM)) {
        BTD("%s, scan fail\n", __func__);
        res_result = RESULT_FAIL;
        goto error;
     }

    if(0 != wifimgr_do_connect()) {
        BTD("%s, connect fail\n", __func__);
        res_result = RESULT_FAIL;
        goto error;
    }else {
        return;
    }

error:
    data[0] = RESULT_SET_CONF_AND_CONNECT;
    data[1] = res_result;

    wifi_manager_notify(data, sizeof(data));

}

void wifimgr_ctrl_iface_notify_connect(unsigned char result)
{
    BTD("%s ,result = %d\n", __func__, result);
    char data[2] = {0};
    u8_t res_result = RESULT_SUCCESS;

    if(0 == result){
        BTD("%s, connect succesfully\n", __func__);
        res_result = RESULT_SUCCESS;
    } else {
        BTD("%s, connect failed\n", __func__);
        res_result = RESULT_FAIL;
    }

    data[0] = RESULT_SET_CONF_AND_CONNECT;
    data[1] = res_result;

    wifi_manager_notify(data, sizeof(data));

#if defined(CONFIG_WIFI_REPEATER)
    if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->set_conf)
        wifimgr_get_ctrl_ops(get_wifimgr_cbs())->set_conf(WIFIMGR_IFACE_NAME_AP,
							"UNISOC_REPEATER",
							NULL,
							NULL,
							0,
							6);

    if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->open)
        wifimgr_get_ctrl_ops(get_wifimgr_cbs())->open(WIFIMGR_IFACE_NAME_AP);

    if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->start_ap)
        wifimgr_get_ctrl_ops(get_wifimgr_cbs())->start_ap();
#endif
}

void wifimgr_ctrl_iface_notify_connect_timeout(void)
{
    BTD("%s\n", __func__);
    char data[2] = {0};
    u8_t res_result = RESULT_FAIL;

    data[0] = RESULT_SET_CONF_AND_CONNECT;
    data[1] = res_result;

    wifi_manager_notify(data, sizeof(data));
}

void wifimgr_get_conf(const void *buf)
{
    BTD("%s\n", __func__);
    int ret = -1;
    char data[2] = {0};
    u8_t res_result = RESULT_SUCCESS;

    if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->get_conf) {
        ret = wifimgr_get_ctrl_ops(get_wifimgr_cbs())->get_conf(WIFIMGR_IFACE_NAME_STA);
    } else {
        BTD("%s, get_conf = NULL\n", __func__);
        res_result = RESULT_FAIL;
    }

    if(0 != ret) {
        BTD("%s, get_conf fail,err = %d\n", __func__,ret);
        res_result = RESULT_FAIL;
    }else{
        return;
    }

    data[0] = RESULT_GET_CONF;
    data[1] = res_result;

    wifi_manager_notify(data, sizeof(data));
}

void wifimgr_ctrl_iface_get_conf_cb(char *iface_name, char *ssid, char *bssid,
                char *passphrase, unsigned char band, unsigned char channel)
{
    BTD("%s\n", __func__);
    wifi_config_type conf;
    u8_t res_result = RESULT_SUCCESS;
    char data[255] = {0};
    u16_t data_len = 0;
    u16_t ssid_len = 0;
    u16_t bssid_len = 0;
    u16_t passwd_len = 0;
    char *p = NULL;
    int i;

    if(ssid){
        ssid_len = strlen(ssid);
        BTD("%s ,ssid = %s\n", __func__,ssid);
    }else{
        BTD("%s ,ssid = NULL\n", __func__);
        ssid_len = 0;
    }

    if(passphrase){
        BTD("%s ,pwd = %s\n", __func__,passphrase);
        passwd_len = strlen(passphrase);
    }else{
        BTD("%s ,pwd = NULL\n", __func__);
        passwd_len = 0;
    }

    if(bssid){
        bssid_len = BSSID_LEN;
        for(i = 0; i < bssid_len; i++){
            BTD("bssid = 0x%02x\n", bssid[i]);
        }
    }else{
        BTD("%s ,bssid = NULL\n", __func__);
        bssid_len = 0;
    }

    BTD("%s, band:%d, channel:%d\n", __func__, band, channel);

    if((ssid_len > MAX_SSID_LEN)
        || (bssid_len > BSSID_LEN)
        || (passwd_len > MAX_PSWD_LEN)) {
        res_result = RESULT_FAIL;
        data[0] = RESULT_GET_STATUS;
        data[1] = res_result;

        BTD("%s ,len error\n", __func__);
        wifi_manager_notify(data, 2);
        return;
    }

    memset(&conf, 0, sizeof(conf));
    memcpy(conf.ssid, ssid, ssid_len);
    memcpy(conf.bssid, bssid, bssid_len);
    memcpy(conf.passwd, passphrase, passwd_len);
    data_len = 6 + ssid_len + bssid_len +passwd_len;
    res_result = RESULT_SUCCESS;

    p = data;
    UINT8_TO_STREAM(p, RESULT_GET_CONF);
    UINT8_TO_STREAM(p, res_result);
    UINT16_TO_STREAM(p, data_len);

    UINT16_TO_STREAM(p, ssid_len);
    for (i = 0; i < ssid_len; i++) {
        UINT8_TO_STREAM(p, conf.ssid[i]);
    }

    UINT16_TO_STREAM(p, bssid_len);
    for (i = 0; i < bssid_len; i++) {
        UINT8_TO_STREAM(p, conf.bssid[i]);
    }

    UINT16_TO_STREAM(p, passwd_len);
    for (i = 0; i < passwd_len; i++) {
        UINT8_TO_STREAM(p, conf.passwd[i]);
    }

    wifi_manager_notify(data, data_len + 4);
}

void wifimgr_get_status(const void *buf)
{
    BTD("%s\n", __func__);
    int ret = -1;
    char data[9] = {0};
    u8_t res_result = RESULT_SUCCESS;
    int data_len = 0;

    ret = wifimgr_check_wifi_status();
    if(0 != ret){
        data_len = 2;
        res_result = RESULT_FAIL;
    }else{
        data_len = 9;
        data[2] = cur_wifi_status.status;
        memcpy(&data[3], cur_wifi_status.wifi_mac, 6);
        res_result = RESULT_SUCCESS;
    }
    data[0] = RESULT_GET_STATUS;
    data[1] = res_result;

    wifi_manager_notify(data, data_len);
}

void wifimgr_ctrl_iface_get_status_cb(char *iface_name, unsigned char status,
                char *own_mac, signed char signal)
{
    BTD("%s,iface_name:%s,status:%d,own_mac:%s,signal:%d\n", __func__,iface_name,status,own_mac,signal);

    cur_wifi_status.status = status;

    memcpy(cur_wifi_status.wifi_mac, own_mac, 6);

    k_sem_give(&get_status_sem);
}

void wifimgr_open(const void *buf)
{
    BTD("%s\n", __func__);
    char data[2] = {0};
    u8_t res_result = RESULT_SUCCESS;

    if(0 != wifimgr_do_open()) {
        BTD("%s, open fail\n", __func__);
        res_result = RESULT_FAIL;
    }else{
        BTD("%s, open success\n", __func__);
        res_result = RESULT_SUCCESS;
    }

    data[0] = RESULT_OPEN;
    data[1] = res_result;

    wifi_manager_notify(data, sizeof(data));
}

void wifimgr_close(const void *buf)
{
    BTD("%s\n", __func__);
    int ret = -1;
    char data[2] = {0};
    u8_t res_result = RESULT_SUCCESS;

    if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->close) {
        ret = wifimgr_get_ctrl_ops(get_wifimgr_cbs())->close(WIFIMGR_IFACE_NAME_STA);
    } else {
        BTD("%s, close = NULL\n", __func__);
        res_result = RESULT_FAIL;
    }

    if(0 != ret) {
        BTD("%s, close fail,err = %d\n", __func__,ret);
        res_result = RESULT_FAIL;
    }

    data[0] = RESULT_CLOSE;
    data[1] = res_result;

    wifi_manager_notify(data, sizeof(data));
}


int wifimgr_do_scan(int retry_num)
{
    //before connect need scan
    BTD("%s\n", __func__);
    int ret = -1;
    int i =0;

    for(i = 0, scan_result = 0; (i < retry_num) && (1 != scan_result); i++){
        BTD("%s,do the %dth scan\n", __func__,i+1);
        if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->scan) {
            ret = wifimgr_get_ctrl_ops(get_wifimgr_cbs())->scan();
        } else {
            BTD("%s, scan = NULL\n", __func__);
            goto error;
        }

        if(0 != ret) {
            BTD("%s, scan fail,err = %d\n", __func__,ret);
            goto error;
        } else {
            if(0 != k_sem_take(&scan_sem, SCAN_TIMEOUT)){
                BTD("%s, take scan_sem fail\n", __func__);
                scan_result = 0;
                ret = -1;
            }else {
                ret = ((1 == scan_result) ? 0 : -1);
            }
        }
    }
error:
    return ret;
}

int wifimgr_do_connect(void)
{
    BTD("%s\n", __func__);
    int ret = -1;

    if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->connect) {
        ret = wifimgr_get_ctrl_ops(get_wifimgr_cbs())->connect();
    } else {
        BTD("%s, connect = NULL\n", __func__);
        goto error;
    }

    if(0 != ret) {
        BTD("%s, connect fail,err = %d\n", __func__,ret);
        goto error;
    } else {
        ret = 0;
        goto error;
    }
error:
    return ret;
}

int wifimgr_do_disconnect(u8_t flags)
{
    BTD("%s\n", __func__);
    int ret = -1;

    if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->disconnect) {
        ret = wifimgr_get_ctrl_ops(get_wifimgr_cbs())->disconnect();
    } else {
        BTD("%s, disconnect = NULL\n", __func__);
        goto error;
    }

    if(0 != ret) {
        BTD("%s, disconnect fail-1,err = %d\n", __func__,ret);
        goto error;
    } else {
        if(1 == flags){
            wait_disconnect_done = 1;
            k_sem_take(&disconnect_sem, K_FOREVER);
            if(-1 == disconnect_result) {
                BTD("%s, disconnect fail-2\n", __func__);
                ret = -1;
            } else {
                ret = 0;
            }
            wait_disconnect_done = 0;
        } else {
            ret = 0;
        }
        goto error;
    }
error:
    return ret;
}

int wifimgr_do_open(void)
{
    BTD("%s\n", __func__);
    int ret = -1;

    if(wifimgr_get_ctrl_ops(get_wifimgr_cbs())->open) {
        ret = wifimgr_get_ctrl_ops(get_wifimgr_cbs())->open(WIFIMGR_IFACE_NAME_STA);
    } else {
        BTD("%s, open = NULL\n", __func__);
        goto error;
    }

    if(0 != ret) {
        BTD("%s, open fail,err = %d\n", __func__,ret);
        goto error;
    } else {
        ret = 0;
        goto error;
    }
error:
    return ret;
}


void wifimgr_disconnect(const void *buf)
{
    BTD("%s\n", __func__);
    char data[2] = {0};
    u8_t res_result = RESULT_SUCCESS;

    if(0 != wifimgr_do_disconnect(0)) {
        BTD("%s, disconnect fail\n", __func__);
        res_result = RESULT_FAIL;
    }else{
        return;
    }

    data[0] = RESULT_DISCONNECT;
    data[1] = res_result;

    wifi_manager_notify(data, sizeof(data));
}

void wifimgr_ctrl_iface_notify_disconnect(unsigned char reason)
{
    BTD("%s ,reason = %d\n", __func__,reason);
    char data[2] = {0};
    u8_t res_result = RESULT_SUCCESS;

    data[0] = RESULT_DISCONNECT;
    data[1] = res_result;

    if(1 == wait_disconnect_done){
        disconnect_result = 0;
        k_sem_give(&disconnect_sem);
    }else{
        wifi_manager_notify(data, sizeof(data));
    }
}

void wifimgr_ctrl_iface_notify_disconnect_timeout(void)
{
    BTD("%s \n", __func__);
    char data[2] = {0};
    u8_t res_result = RESULT_FAIL;

    data[0] = RESULT_DISCONNECT;
    data[1] = res_result;

    if(1 == wait_disconnect_done){
        disconnect_result = -1;
        k_sem_give(&disconnect_sem);
    }else{
        wifi_manager_notify(data, sizeof(data));
    }
}

static ssize_t wifi_manager_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            const void *buf, u16_t len, u16_t offset,
            u8_t flags)
{
    u8_t op_code = 0;
    u8_t *value = attr->user_data;
    static u16_t total_len = 0;

    BTD("%s, len = %d, offset = %d, flags = %d\n", __func__,len,offset,flags);
    //cur_conn = conn;

    if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
        return 0;
    }

    if (offset + len > sizeof(wifimgr_long_value)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if(0 == offset){
        memset(value, 0, sizeof(wifimgr_long_value));
        total_len = sys_get_le16(buf + OPCODE_BYTE) + OPCODE_BYTE +LEN_BYTE;
        BTD("%s,total_len = %d\n", __func__,total_len);
    }
    memcpy(value + offset, buf, len);
    total_len -= len;

    if(0 != total_len)
        return len;

    op_code = (u8_t)(*(value));
    BTD("%s,op_code=0x%x\n", __func__,op_code);
    switch(op_code){
        case CMD_SET_CONF_AND_CONNECT:
            wifimgr_set_conf_and_connect(&value[1]);
        break;
        case CMD_GET_CONF:
            wifimgr_get_conf(&value[1]);
        break;
        case CMD_GET_STATUS:
            wifimgr_get_status(&value[1]);
        break;
        case CMD_OPEN:
            wifimgr_open(&value[1]);
        break;
        case CMD_CLOSE:
            wifimgr_close(&value[1]);
        break;
        case CMD_DISCONNECT:
            wifimgr_disconnect(&value[1]);
        break;

        default:
            BTD("%s,op_code=0x%x not found\n", __func__,op_code);
        break;
    }
    return len;
}

/* WiFi Manager Service Declaration */
static struct bt_gatt_attr attrs[] = {
    BT_GATT_PRIMARY_SERVICE(&wifimgr_service_uuid),
    BT_GATT_CHARACTERISTIC(&wifimgr_char_uuid.uuid, BT_GATT_CHRC_WRITE|BT_GATT_CHRC_NOTIFY,
                    BT_GATT_PERM_READ|BT_GATT_PERM_WRITE|BT_GATT_PERM_PREPARE_WRITE, NULL, wifi_manager_write,
                    &wifimgr_long_value),
    BT_GATT_CCC(wifimgr_ccc_cfg, wifimgr_ccc_cfg_changed),
};

static struct bt_gatt_service wifi_manager_svc = BT_GATT_SERVICE(attrs);

void wifi_manager_service_init(void)
{
    BTD("%s\n", __func__);
    bt_gatt_service_register(&wifi_manager_svc);
    k_sem_init(&get_status_sem, 0, 1);
    k_sem_init(&scan_sem, 0, 1);
    k_sem_init(&disconnect_sem, 0, 1);
}

void wifi_manager_notify(const void *data, u16_t len)
{
    BTD("%s, len = %d\n", __func__, len);
    if (!wifimgr_is_enabled) {
        BTD("%s, rx is not enabled\n", __func__);
        return;
    }
    /*
    if(len > bt_gatt_get_mtu(cur_conn)){
        BTD("%s, len > MTU\n", __func__);
        do_gatt_exchange_mtu();
    }*/
    bt_gatt_notify(NULL, &attrs[2], data, len);
}

void wifimgr_ctrl_iface_notify_scan_res(char *ssid, char *bssid, unsigned char band,
                unsigned char channel, signed char signal)
{
    BTD("%s\n", __func__);

}

void wifimgr_ctrl_iface_notify_scan_done(unsigned char result)
{
    BTD("%s,result = %d\n", __func__,result);
    scan_result = result;
    k_sem_give(&scan_sem);
}

void wifimgr_ctrl_iface_notify_scan_timeout(void)
{
    BTD("%s\n", __func__);
    scan_result = 0;
    k_sem_give(&scan_sem);

}

void wifimgr_ctrl_iface_notify_new_station(unsigned char status, unsigned char *mac)
{
    BTD("%s\n", __func__);

}

void wifimgr_ctrl_iface_notify_del_station_timeout(void)
{
    BTD("%s\n", __func__);

}

static struct wifimgr_ctrl_cbs wifi_ctrl_cbs = {
    .get_conf_cb = wifimgr_ctrl_iface_get_conf_cb,
    .get_status_cb = wifimgr_ctrl_iface_get_status_cb,
    .notify_scan_res = wifimgr_ctrl_iface_notify_scan_res,
    .notify_scan_done = wifimgr_ctrl_iface_notify_scan_done,
    .notify_connect = wifimgr_ctrl_iface_notify_connect,
    .notify_disconnect = wifimgr_ctrl_iface_notify_disconnect,
    .notify_scan_timeout = wifimgr_ctrl_iface_notify_scan_timeout,
    .notify_connect_timeout = wifimgr_ctrl_iface_notify_connect_timeout,
    .notify_disconnect_timeout = wifimgr_ctrl_iface_notify_disconnect_timeout,
    .notify_new_station = wifimgr_ctrl_iface_notify_new_station,
    .notify_del_station_timeout = wifimgr_ctrl_iface_notify_del_station_timeout,
};

struct wifimgr_ctrl_cbs *get_wifimgr_cbs(void)
{
    return &wifi_ctrl_cbs;
}
