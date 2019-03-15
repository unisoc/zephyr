/*
 * @file
 * @brief Control interface header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFIMGR_CTRL_IFACE_H_
#define _WIFIMGR_CTRL_IFACE_H_

#include <net/wifimgr_api.h>

#include "api.h"
#include "cmd_prcs.h"
#include "config.h"
#include "notifier.h"

struct wifimgr_ctrl_iface {
	sem_t syncsem;			/* synchronization for async command */
	mqd_t mq;
	bool active_disconnect;
	char evt_status;
	scan_res_cb_t scan_res_cb;
	rtt_resp_cb_t rtt_resp_cb;
	connect_cb_t connect_cb;
	disconnect_cb_t disconnect_cb;
	/*scan_timeout_cb_t scan_timeout_cb;
	connect_timeout_cb_t connect_timeout_cb;
	disconnect_timeout_cb_t disconnect_timeout_cb;*/
	struct wifimgr_notifier_chain conn_chain;
	struct wifimgr_notifier_chain disc_chain;
	struct wifimgr_notifier_chain new_sta_chain;
	struct wifimgr_notifier_chain sta_leave_chain;
};

void wifimgr_ctrl_evt_scan_result(void *handle, struct wifi_scan_result *scan_res);
void wifimgr_ctrl_evt_scan_done(void *handle, char status);
void wifimgr_ctrl_evt_rtt_response(void *handle, struct wifi_rtt_response *rtt_resp);
void wifimgr_ctrl_evt_rtt_done(void *handle, char status);
void wifimgr_ctrl_evt_connect(void *handle, struct wifimgr_notifier_chain *conn_chain, char status);
void wifimgr_ctrl_evt_disconnect(void *handle, struct wifimgr_notifier_chain *disc_chain, char reason_code);
void wifimgr_ctrl_evt_new_station(void *handle, struct wifimgr_notifier_chain *chain, char status, char *mac);

void wifimgr_ctrl_evt_timeout(void *handle);

int wifimgr_ctrl_iface_get_conf(char *iface_name, struct wifi_config *conf);
int wifimgr_ctrl_iface_set_conf(char *iface_name, struct wifi_config *user_conf);
int wifimgr_ctrl_iface_get_capa(char *iface_name, union wifi_capa *capa);
int wifimgr_ctrl_iface_get_status(char *iface_name, struct wifi_status *sts);
int wifimgr_ctrl_iface_open(char *iface_name);
int wifimgr_ctrl_iface_close(char *iface_name);
int wifimgr_ctrl_iface_scan(char *iface_name, scan_res_cb_t scan_res_cb);
int wifimgr_ctrl_iface_rtt_request(struct wifi_rtt_request *rtt_req, rtt_resp_cb_t rtt_resp_cb);
int wifimgr_ctrl_iface_connect(void);
int wifimgr_ctrl_iface_disconnect(void);
int wifimgr_ctrl_iface_start_ap(void);
int wifimgr_ctrl_iface_stop_ap(void);
int wifimgr_ctrl_iface_del_station(char *mac);
int wifimgr_ctrl_iface_set_mac_acl(char subcmd, char *mac);

int wifimgr_init_ctrl_iface(char *iface_name, struct wifimgr_ctrl_iface *ctrl);
int wifimgr_destroy_ctrl_iface(char *iface_name, struct wifimgr_ctrl_iface *ctrl);

int wifimgr_ctrl_iface_send_cmd(struct wifimgr_ctrl_iface *ctrl, unsigned int cmd_id, void *buf, int buf_len);

#endif
