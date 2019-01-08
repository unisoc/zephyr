/*
 * @file
 * @brief State machine header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFI_SM_H_
#define _WIFI_SM_H_

enum wifimgr_sm_sta_state {
	WIFIMGR_SM_STA_NODEV,
	WIFIMGR_SM_STA_READY,
	WIFIMGR_SM_STA_SCANNING,
	WIFIMGR_SM_STA_CONNECTING,
	WIFIMGR_SM_STA_CONNECTED,
	WIFIMGR_SM_STA_DISCONNECTING,
};

enum wifimgr_sm_ap_state {
	WIFIMGR_SM_AP_NODEV,
	WIFIMGR_SM_AP_READY,
	WIFIMGR_SM_AP_STARTED,
};

struct wifimgr_state_machine {
	wifimgr_work work;
	unsigned int state;
	unsigned int old_state;
	unsigned int cur_cmd;	/* record the command under processing */
	sem_t exclsem;		/* exclusive access to the struct */
	timer_t timerid;	/* timer for event */
};

const char *sta_sts2str(int state);
int sm_sta_query(struct wifimgr_state_machine *sta_sm);
bool sm_sta_connected(struct wifimgr_state_machine *sta_sm);
void sm_sta_step_back(struct wifimgr_state_machine *sta_sm);

const char *ap_sts2str(int state);
int sm_ap_query(struct wifimgr_state_machine *ap_sm);
bool sm_ap_started(struct wifimgr_state_machine *ap_sm);

const char *wifimgr_cmd2str(int cmd);
const char *wifimgr_evt2str(int evt);
const char *wifimgr_sts2str_cmd(void *handle, unsigned int cmd_id);
const char *wifimgr_sts2str_evt(void *handle, unsigned int evt_id);
int wifimgr_sm_query_cmd(void *handle, unsigned int cmd_id);
void wifimgr_sm_cmd_step(void *handle, unsigned int cmd_id, char indication);
void wifimgr_sm_evt_step(void *handle, unsigned int evt_id, char indication);
int wifimgr_sm_init(struct wifimgr_state_machine *sm, void *work_handler);
void wifimgr_sm_exit(struct wifimgr_state_machine *sm);

#endif
