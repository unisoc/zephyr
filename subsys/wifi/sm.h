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

int sm_sta_timer_start(struct wifimgr_state_machine *sta_sm, unsigned int sec);
int sm_sta_timer_stop(struct wifimgr_state_machine *sta_sm,
		      unsigned int evt_id);
int sm_ap_timer_start(struct wifimgr_state_machine *ap_sm, unsigned int cmd_id);
int sm_ap_timer_stop(struct wifimgr_state_machine *ap_sm, unsigned int evt_id);

bool is_common_cmd(unsigned int cmd_id);

const char *sta_sts2str(int state);
bool is_sta_cmd(unsigned int cmd_id);
bool is_sta_evt(unsigned int evt_id);
int sm_sta_query(struct wifimgr_state_machine *sta_sm);
bool sm_sta_connected(struct wifimgr_state_machine *sta_sm);
int sm_sta_query_cmd(struct wifimgr_state_machine *sta_sm, unsigned int cmd_id);
void sm_sta_step_cmd(struct wifimgr_state_machine *sta_sm, unsigned int cmd_id);
void sm_sta_step_evt(struct wifimgr_state_machine *sta_sm, unsigned int evt_id);
void sm_sta_step_back(struct wifimgr_state_machine *sta_sm);
int sm_sta_init(struct wifimgr_state_machine *sta_sm);
int sm_sta_exit(struct wifimgr_state_machine *sta_sm);

const char *ap_sts2str(int state);
bool is_ap_cmd(unsigned int cmd_id);
bool is_ap_evt(unsigned int evt_id);
int sm_ap_query(struct wifimgr_state_machine *ap_sm);
bool sm_ap_started(struct wifimgr_state_machine *ap_sm);
void sm_ap_step_cmd(struct wifimgr_state_machine *ap_sm, unsigned int cmd_id);
int sm_ap_init(struct wifimgr_state_machine *ap_sm);

#endif
