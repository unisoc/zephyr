#ifndef __WIFI_CMDEVT_H__
#define __WIFI_CMDEVT_H__

#define MAX_SSID_LEN 32
#define MAX_KEY_LEN 128
#define ETH_ALEN 6
#define INI_SIZE  1600

#include <net/wifi_mgmt.h>

//#include "wifi_main.h"
struct wifi_priv;

enum cmd_type {
	WIFI_CMD_BEGIN = 0x00,
	//common
	WIFI_CMD_GET_CP_INFO,	//get cp version number 
	WIFI_CMD_DOWNLOAD_INI, //TODO
	WIFI_CMD_OPEN,	//start a wifi mode  TODO
	WIFI_CMD_CLOSE,	//stop wifi  TODO
	WIFI_CMD_NPI_MSG,  ///TODO
	//sta
	WIFI_CMD_SCAN,	// TODO
	WIFI_CMD_CONNECT,	//CP only return status. TODO
	WIFI_CMD_DISCONNECT,	//ap pass reason code .TODO
	WIFI_CMD_GET_STATION,	//get sta's RSSI and tx rate .TODO
	//ap
	WIFI_CMD_START_AP,	//set softap running information,such as ssid,auth mode,etc
	WIFI_CMD_DEL_STATION,	//disconnect sta connected with us
	WIFI_CMD_SET_BLACKLIST,	//disconnect sta connected with us
	WIFI_CMD_SET_WHITELIST,	//disconnect sta connected with us

	WIFI_CMD_MAX,
};

enum event_type {
	WIFI_EVENT_BEGIN = 0x80,
	WIFI_EVENT_CONNECT, //CP only return status  TODO
	WIFI_EVENT_DISCONNECT,  //cp return reason code  TODO
	WIFI_EVENT_SCAN_RESULT, // scan a ap frame  TODO
	WIFI_EVENT_SCAN_DONE,   //scan done event  TODO
	//ap
	WIFI_EVENT_NEW_STATION, //event send by cp to report sta disconnected with us
	WIFI_EVENT_MAX,
};

/* this structure is shared by cmd and event. */
struct trans_hdr{
	u8_t type;// event_type
	u8_t seq; //if cp response ap event,should set it the same as ap event seq,or else should set to 0xff.
	u8_t response;//to identify the event is a cp response ,or cp report.if it is a response ,set to 1,if is a report ,set to 0
	char status;//if response by cp,indicate cp handle fail or success,0 for success,other for fail.
	u16_t len;//msg len, incluld the event header
	char data[0];//the start of msg data.
}__attribute__ ((packed));

struct cmd_download_ini {
	struct trans_hdr trans_header;
	u32_t sec_num;
	char data[INI_SIZE];
} __attribute__ ((packed));

struct cmd_start{
	struct trans_hdr trans_header;
	char mode;
	char mac[6];
}__attribute__ ((packed));

struct cmd_get_cp_info{
	struct trans_hdr trans_header;
	u32_t version;
	char mac[6];
}__attribute__ ((packed));

struct cmd_stop{
	struct trans_hdr trans_header;
	char mode;
	char mac[6];
}__attribute__ ((packed));

/* cmd struct for ap */
struct cmd_start_ap {
	struct trans_hdr trans_header;
	u8_t ssid_len;
	u8_t ssid[MAX_SSID_LEN];
	u8_t password_len;
	char password[MAX_KEY_LEN];
	u8_t channel;
	u8_t vht_chwidth;
	u8_t vht_chan_center_freq_seg0_idx;
	u8_t vht_chan_center_freq_seg1_idx;
} __attribute__ ((packed));

struct cmd_del_station {
	/* if mac set to FF:FF:FF:FF:FF:FF,
	 * all station will be disconnected
	 */
	u8_t mac[6];
	u16_t reason_code;
} __attribute__ ((packed));

/* cmd struct for sta */
struct cmd_scan {
	struct trans_hdr trans_header; //common header for all event.
	u32_t channels;  /* One bit for one channel */
	u32_t flags;
	u16_t ssid_len;  /* hidden ssid length */
	u8_t ssid[0];   /* hidden ssid  */
	u16_t channels_5g_cnt; /* number of 5G channels */
	u16_t channels_5g[0]; /* 5G channels to be scanned. */
} __attribute__ ((packed));

struct cmd_connect {
	struct trans_hdr trans_header; //common header for all event.
	u32_t wpa_versions;
	u8_t bssid[ETH_ALEN];
	u8_t channel;
	u8_t auth_type;
	u8_t pairwise_cipher;
	u8_t group_cipher;
	u8_t key_mgmt;
	u8_t mfp_enable;
	u8_t passphrase_len; //FILL
	u8_t ssid_len; //FILL
	u8_t passphrase[MAX_KEY_LEN]; //FILL
	u8_t ssid[MAX_SSID_LEN]; //FILL
} __attribute__ ((packed));

struct cmd_disconnect {
	u8_t reason_code;
} __attribute__ ((packed));

struct event_scan_result {
	u8_t band;
	u8_t channel;
	int8_t  rssi;
	char bssid[ETH_ALEN];
	char ssid[MAX_SSID_LEN];
} __attribute__ ((packed));

struct event_scan_done {
#define WIFI_EVENT_SCAN_SUCC    0
#define WIFI_EVENT_SCAN_ERROR   1
	u8_t result;
} __attribute__ ((packed));

struct event_connect {
	u8_t status; /* TODO: sync status code with cp */
} __attribute__ ((packed));

struct event_disconnect {
	u8_t reason_code; /* TODO: sync reason_code with cp */
} __attribute__ ((packed));

//int wifi_cmd_load_ini(u8_t *pAd);
extern int wifi_cmd_set_sta_connect_info(u8_t *pAd,char *ssid,char *key);
extern int wifi_cmd_get_cp_info(struct wifi_priv *priv);
extern int wifi_cmd_start(struct wifi_priv *priv);
extern int wifi_cmd_stop(struct wifi_priv *priv);
extern int wifi_cmd_scan(struct wifi_priv *priv);
extern int wifi_cmd_connect(struct wifi_priv *priv,
		struct wifi_connect_req_params *params);
extern int wifi_cmd_disconnect(struct wifi_priv *priv);
extern int wifi_cmd_start_ap(struct wifi_priv *priv, struct wifi_start_ap_req_params *params);
extern int wifi_cmd_stop_ap(struct wifi_priv *priv);
extern int wifi_cmd_npi_send(int ictx_id,char * t_buf,u32_t t_len,char *r_buf,u32_t *r_len);
extern int wifi_cmd_npi_get_mac(int ictx_id,char * buf);

extern int wifi_cmd_send(u8_t cmd, char *data, int len,
		char * rbuf, int *rlen);
extern int wifi_cmd_load_ini(u8_t * data, u32_t len, u8_t sec_num);
extern int wifi_cmdevt_process(struct wifi_priv *priv, char *data, int len);
extern int wifi_cmdevt_init(void);

#endif /* __WIFI_CMDEVT_H__ */
