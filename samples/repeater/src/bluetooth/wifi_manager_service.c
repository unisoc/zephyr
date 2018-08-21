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

#include "wifi_manager_service.h"


//#include "../../wireless/wifimgr/wifimgr_api.h"

static struct bt_uuid_128 wifimgr_service_uuid = BT_UUID_INIT_128(
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xa5, 0xff, 0x00, 0x00);

static struct bt_uuid_128 wifimgr_char_uuid = BT_UUID_INIT_128(
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xa6, 0xff, 0x00, 0x00);


static u8_t wifimgr_long_value[256] = {0};

static struct bt_gatt_ccc_cfg wifimgr_ccc_cfg[BT_GATT_CCC_MAX] = {};
static u8_t wifimgr_is_enabled;


static void wifimgr_ccc_cfg_changed(const struct bt_gatt_attr *attr,
                u16_t value)
{
    BTD("%s", __func__);
    wifimgr_is_enabled = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static void wifimgr_set_conf(const void *buf)
{
    BTD("%s", __func__);
    wifi_config_type conf;
    const u8_t *p = buf;
    u16_t vlen = 0;

    memset(&conf, 0, sizeof(conf));
    vlen = sys_get_le16(p);
    p += 2;
    BTD("%s, AllDateLen = 0x%x", __func__, vlen);

    vlen = sys_get_le16(p);
    p += 2;
    BTD("%s, SsidDataLen = 0x%x", __func__, vlen);

    memcpy(conf.ssid,p,vlen);
    p+=vlen;
    BTD("%s, ssid = %s", __func__, conf.ssid);

    vlen = sys_get_le16(p);
    p += 2;
    BTD("%s, PsdDataLen = 0x%x", __func__, vlen);

    memcpy(conf.passwd,p,vlen);
    p+=vlen;
    BTD("%s, passwd = %s", __func__, conf.passwd);
#if 0
    if(wifimgr_get_ctrl_ops()->set_conf) {
        wifimgr_get_ctrl_ops()->set_conf(conf.ssid, conf.bssid, conf.passwd, conf.band, conf.channel);
    } else {
        BTD("%s, set_conf = NULL", __func__);
    }
#endif
}

static ssize_t wifi_manager_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
            const void *buf, u16_t len, u16_t offset,
            u8_t flags)
{
    u8_t op_code = 0;
    u8_t *value = attr->user_data;
    static u16_t total_len = 0;

    BTD("%s, len = %d, offset = %d, flags = %d", __func__,len,offset,flags);

    if (flags & BT_GATT_WRITE_FLAG_PREPARE) {
        return 0;
    }

    if (offset + len > sizeof(wifimgr_long_value)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if(0 == offset){
        memset(value, 0, sizeof(wifimgr_long_value));
        total_len = sys_get_le16(buf + OPCODE_BYTE) + OPCODE_BYTE +LEN_BYTE;
        BTD("%s,total_len = %d", __func__,total_len);
    }
    memcpy(value + offset, buf, len);
    total_len -= len;

    if(0 != total_len)
        return len;

    op_code = (u8_t)(*(value));
    BTD("%s,op_code=0x%x", __func__,op_code);
    switch(op_code){
        case CMD_SET_CONF:
            wifimgr_set_conf(&value[1]);
        break;
        default:
            BTD("%s,op_code=0x%x not found", __func__,op_code);
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
    BTD("%s", __func__);
    bt_gatt_service_register(&wifi_manager_svc);
}

void wifi_manager_notify(const void *data, u16_t len)
{
    BTD("%s, len = %d", __func__, len);
    if (!wifimgr_is_enabled) {
        BTD("%s, rx is not enabled", __func__);
        return;
    }

    bt_gatt_notify(NULL, &attrs[2], data, len);
}
