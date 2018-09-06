#ifndef __WIFI_MANAGER_SERVICE_H__
#define __WIFI_MANAGER_SERVICE_H__

#define MAX_SSID_LEN    32
#define MAX_PSWD_LEN    32
#define BSSID_LEN   6
#define OPCODE_BYTE   1
#define LEN_BYTE   2


#define CMD_OPEN                       0x01
#define CMD_CLOSE                      0x02
#define CMD_SET_CONF_AND_CONNECT       0x03
#define CMD_DISCONNECT                 0x04
#define CMD_GET_STATUS                 0x05
#define CMD_GET_CONF                   0x06

#define RESULT_OPEN                    0x81
#define RESULT_CLOSE                   0x82
#define RESULT_SET_CONF_AND_CONNECT    0x83
#define RESULT_DISCONNECT              0x84
#define RESULT_GET_STATUS              0x85
#define RESULT_GET_CONF                0x86


typedef struct {
    char ssid[MAX_SSID_LEN+1];
    char bssid[BSSID_LEN+1];
    char passwd[MAX_PSWD_LEN+1];
    unsigned char band;
    unsigned char channel;
}wifi_config_type;

typedef struct {
    unsigned char status;
    char wifi_mac[6];
}wifi_status_type;

enum {
    WIFI_STATUS_NODEV,
    WIFI_STATUS_READY,
    WIFI_STATUS_SCANNING,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_DISCONNECTING,
};

enum {
    RESULT_SUCCESS,
    RESULT_FAIL,
};

void wifi_manager_service_init(void);
void wifi_manager_notify(const void *data, u16_t len);
int wifimgr_do_scan(int retry_num);
int wifimgr_do_connect(void);
int wifimgr_do_disconnect(u8_t flags);
int wifimgr_do_open(void);
int wifimgr_check_wifi_status(void);
#endif
