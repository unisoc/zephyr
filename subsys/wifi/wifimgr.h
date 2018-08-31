/****************************************************************************
 * apps/wireless/wifimgr/wifimgr.h
 *
#   Author: Keguang Zhang <keguang.zhang@unisoc.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

#ifndef _WIFIMGR_H_
#define _WIFIMGR_H_

#include <net/wifi_drv_iface.h>
#include <net/wifimgr_api.h>

#include "os_adapter.h"
#include "cmd_prcs.h"
#include "evt_lsnr.h"
#include "state_machine.h"

#define WIFIMGR_STA_DEVNAME	"UWP_STA"
#define WIFIMGR_AP_DEVNAME	"UWP_AP"

#define WIFIMGR_MAX_SSID_LEN	32
#define WIFIMGR_MAX_PSPHR_LEN	32
#define WIFIMGR_ETH_ALEN	6

#define WIFIMGR_SCAN_TIMEOUT	10
#define WIFIMGR_EVENT_TIMEOUT	10

#define E2S(x) case x: return #x;

struct wifimgr_config {
	char ssid[WIFIMGR_MAX_SSID_LEN];
	char bssid[WIFIMGR_ETH_ALEN];
	char passphrase[WIFIMGR_MAX_PSPHR_LEN];
	unsigned char band;
	unsigned char channel;
};

struct wifimgr_status {
	char ssid[WIFIMGR_MAX_SSID_LEN];
	char bssid[WIFIMGR_ETH_ALEN];
	unsigned char band;
	unsigned char channel;
	char rssi;
};

struct wifimgr_evt_connect {
	char status;
};

struct wifimgr_evt_disconnect {
	char reason_code;
};

struct wifimgr_evt_scan_done {
	uint8_t result;
};

struct wifimgr_evt_scan_result {
	char ssid[WIFIMGR_MAX_SSID_LEN];
	char bssid[WIFIMGR_ETH_ALEN];
	uint8_t band;
	uint8_t channel;
	int8_t rssi;
};

struct wifimgr_evt_new_station {
	uint8_t is_connect;
	uint8_t mac[6];
};

struct wifi_manager {
	bool fw_loaded;

	struct wifimgr_config sta_conf;
	struct wifimgr_status sta_sts;
	struct wifimgr_state_machine sta_sm;

	struct wifimgr_config ap_conf;
	struct wifimgr_status ap_sts;
	struct wifimgr_state_machine ap_sm;

	struct cmd_processor prcs;
	struct evt_listener lsnr;

	void *sta_iface;
	void *ap_iface;

	struct wifimgr_evt_connect evt_conn;
	struct wifimgr_evt_disconnect evt_disc;
	struct wifimgr_evt_scan_done evt_scan_done;
	struct wifimgr_evt_scan_result evt_scan_res;

	struct wifimgr_evt_new_station evt_new_sta;
};

int wifi_manager_get_status(void *handle);

int wifi_manager_get_sta_config(void *handle);
int wifi_manager_set_sta_config(void *handle);
int wifi_manager_open_station(void *handle);
int wifi_manager_close_station(void *handle);
int wifi_manager_scan(void *handle);
int wifi_manager_connect(void *handle);
int wifi_manager_disconnect(void *handle);
int wifi_manager_get_station(void *handle);

int wifi_manager_get_ap_config(void *handle);
int wifi_manager_set_ap_config(void *handle);
int wifi_manager_open_softap(void *handle);
int wifi_manager_close_softap(void *handle);
int wifi_manager_start_softap(void *handle);
int wifi_manager_stop_softap(void *handle);

const char *wifimgr_sts2str(struct wifi_manager *mgr, unsigned int cmd_id);
int wifi_manager_sm_start_timer(struct wifi_manager *mgr, unsigned int cmd_id);
int wifi_manager_sm_stop_timer(struct wifi_manager *mgr, unsigned int cmd_id);
int wifi_manager_sm_query_cmd(struct wifi_manager *mgr, unsigned int cmd_id);
void wifi_manager_sm_step_cmd(struct wifi_manager *mgr, unsigned int cmd_id);
void wifi_manager_sm_step_evt(struct wifi_manager *mgr, unsigned int evt_id);
void wifi_manager_sm_step_back(struct wifi_manager *mgr, unsigned int evt_id);
bool wifi_manager_first_run(struct wifi_manager *mgr);
int wifi_manager_low_level_init(struct wifi_manager *mgr);

#endif
