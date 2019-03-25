/*
 * @file
 * @brief WiFi manager APIs for the external caller
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_API_H_
#define ZEPHYR_INCLUDE_NET_WIFI_API_H_

#include <net/wifi_drv.h>

#define WIFI_MAC_ADDR_LEN	NET_LINK_ADDR_MAX_LENGTH
#define WIFI_MAX_SSID_LEN	32
#define WIFI_MAX_PSPHR_LEN	63

struct wifi_config {
	char ssid[WIFI_MAX_SSID_LEN + 1];
	char bssid[WIFI_MAC_ADDR_LEN];
	char security;
	char passphrase[WIFI_MAX_PSPHR_LEN + 1];
	unsigned char band;
	unsigned char channel;
	unsigned char ch_width;
	int autorun;
};

enum wifi_sta_state {
	WIFI_STATE_STA_NODEV,
	WIFI_STATE_STA_READY,
	WIFI_STATE_STA_SCANNING,
	WIFI_STATE_STA_RTTING,
	WIFI_STATE_STA_CONNECTING,
	WIFI_STATE_STA_CONNECTED,
	WIFI_STATE_STA_DISCONNECTING,
};

enum wifi_ap_state {
	WIFI_STATE_AP_NODEV,
	WIFI_STATE_AP_READY,
	WIFI_STATE_AP_STARTED,
};

struct wifi_status {
	char state;
	char own_mac[WIFI_MAC_ADDR_LEN];
	union {
		struct {
			char host_found;
			char host_bssid[WIFI_MAC_ADDR_LEN];
			signed char host_rssi;
		} sta;
		struct {
			unsigned char nr_sta;
			char (*sta_mac_addrs)[WIFI_MAC_ADDR_LEN];
			unsigned char nr_acl;
			char (*acl_mac_addrs)[WIFI_MAC_ADDR_LEN];
		} ap;
	} u;
};

enum wifi_security {
	WIFI_SECURITY_UNKNOWN,
	WIFI_SECURITY_OPEN,
	WIFI_SECURITY_PSK,
	WIFI_SECURITY_OTHERS,
};

struct wifi_scan_result {
	char ssid[WIFI_MAX_SSID_LEN];
	char bssid[WIFI_MAC_ADDR_LEN];
	unsigned char band;
	unsigned char channel;
	signed char rssi;
	enum wifi_security security;
	char rtt_supported;
};

struct wifi_rtt_request {
	unsigned char nr_peers;
	struct wifi_rtt_peers *peers;
};

struct wifi_rtt_response {
	char bssid[WIFI_MAC_ADDR_LEN];
	int range;
};

union wifi_notifier_val {
	char val_char;
	void *val_ptr;
};

typedef void (*wifi_notifier_fn_t)(union wifi_notifier_val val);

int wifi_register_connection_notifier(wifi_notifier_fn_t notifier_call);
int wifi_unregister_connection_notifier(wifi_notifier_fn_t notifier_call);
int wifi_register_disconnection_notifier(wifi_notifier_fn_t notifier_call);
int wifi_unregister_disconnection_notifier(wifi_notifier_fn_t notifier_call);
int wifi_register_new_station_notifier(wifi_notifier_fn_t notifier_call);
int wifi_unregister_new_station_notifier(wifi_notifier_fn_t notifier_call);
int wifi_register_station_leave_notifier(wifi_notifier_fn_t notifier_call);
int wifi_unregister_station_leave_notifier(wifi_notifier_fn_t notifier_call);

typedef void (*scan_res_cb_t)(struct wifi_scan_result *scan_res);
typedef void (*rtt_resp_cb_t)(struct wifi_rtt_response *rtt_resp);

int wifi_sta_set_conf(struct wifi_config *conf);
int wifi_sta_clear_conf(void);
int wifi_sta_get_conf(struct wifi_config *conf);
int wifi_sta_get_capa(union wifi_drv_capa *capa);
int wifi_sta_get_status(struct wifi_status *sts);
int wifi_sta_open(void);
int wifi_sta_close(void);
int wifi_sta_scan(scan_res_cb_t scan_res_cb);
int wifi_sta_rtt_request(struct wifi_rtt_request *rtt_req, rtt_resp_cb_t rtt_resp_cb);
int wifi_sta_connect(void);
int wifi_sta_disconnect(void);
int wifi_ap_set_conf(struct wifi_config *conf);
int wifi_ap_clear_conf(void);
int wifi_ap_get_conf(struct wifi_config *conf);
int wifi_ap_get_capa(union wifi_drv_capa *capa);
int wifi_ap_get_status(struct wifi_status *sts);
int wifi_ap_open(void);
int wifi_ap_close(void);
int wifi_ap_start_ap(void);
int wifi_ap_stop_ap(void);
int wifi_ap_del_station(char *mac);
int wifi_ap_set_mac_acl(char subcmd, char *mac);

#ifndef MAC2STR
#define MAC2STR(m) (m)[0], (m)[1], (m)[2], (m)[3], (m)[4], (m)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#endif
