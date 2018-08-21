#ifndef __WIFI_MANAGER_SERVICE_H__
#define __WIFI_MANAGER_SERVICE_H__

#define MAX_SSID_LEN    32
#define MAX_PSWD_LEN    32
#define BSSID_LEN   6
#define OPCODE_BYTE   1
#define LEN_BYTE   2

#define CMD_SET_CONF    0x01

#if 1
#define BTD(fmt, ...) {printk(fmt"\n", ##__VA_ARGS__);}
#else
#define BTD(fmt, ...) do {}while(0)
#endif

typedef struct {
    char ssid[MAX_SSID_LEN];
    char bssid[BSSID_LEN];
    char passwd[MAX_PSWD_LEN];
    unsigned char band;
    unsigned char channel;
}wifi_config_type;

void wifi_manager_service_init(void);
void wifi_manager_notify(const void *data, u16_t len);

#endif
