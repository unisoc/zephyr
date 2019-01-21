/*
 * @file
 * @brief State machine related functions
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#include "wifimgr.h"

static int sm_sta_timer_start(struct wifimgr_state_machine *sta_sm,
			      unsigned int cmd_id)
{
	int ret = 0;

	switch (cmd_id) {
	case WIFIMGR_CMD_SCAN:
		ret =
		    wifimgr_timer_start(sta_sm->timerid, WIFIMGR_SCAN_TIMEOUT);
		break;
	case WIFIMGR_CMD_CONNECT:
	case WIFIMGR_CMD_DISCONNECT:
		ret =
		    wifimgr_timer_start(sta_sm->timerid, WIFIMGR_EVENT_TIMEOUT);
		break;
	default:
		break;
	}

	if (ret)
		wifimgr_err("failed to start STA timer! %d\n", ret);

	return ret;
}

static int sm_sta_timer_stop(struct wifimgr_state_machine *sta_sm,
			     unsigned int evt_id)
{
	int ret = 0;

	switch (evt_id) {
	case WIFIMGR_EVT_SCAN_DONE:
	case WIFIMGR_EVT_CONNECT:
		ret = wifimgr_timer_stop(sta_sm->timerid);
		break;
	case WIFIMGR_EVT_DISCONNECT:
		if (sta_sm->cur_cmd == WIFIMGR_CMD_DISCONNECT)
			ret = wifimgr_timer_stop(sta_sm->timerid);
		break;
	default:
		break;
	}

	if (ret)
		wifimgr_err("failed to stop STA timer! %d\n", ret);

	return ret;
}

static int sm_ap_timer_start(struct wifimgr_state_machine *ap_sm,
			     unsigned int cmd_id)
{
	int ret = 0;

	switch (cmd_id) {
	case WIFIMGR_CMD_DEL_STA:
		ret =
		    wifimgr_timer_start(ap_sm->timerid, WIFIMGR_EVENT_TIMEOUT);
		break;
	default:
		break;
	}

	if (ret)
		wifimgr_err("failed to start AP timer! %d\n", ret);

	return ret;
}

static int sm_ap_timer_stop(struct wifimgr_state_machine *ap_sm,
			    unsigned int evt_id)
{
	int ret = 0;

	switch (evt_id) {
	case WIFIMGR_EVT_NEW_STATION:
		if (ap_sm->cur_cmd == WIFIMGR_CMD_DEL_STA)
			ret = wifimgr_timer_stop(ap_sm->timerid);
		break;
	default:
		break;
	}

	if (ret)
		wifimgr_err("failed to stop AP timer! %d\n", ret);

	return ret;
}

const char *sta_sts2str(int state)
{
	char *str = NULL;

	switch (state) {
	case WIFIMGR_SM_STA_NODEV:
		str = "STA <UNAVAILABLE>";
		break;
	case WIFIMGR_SM_STA_READY:
		str = "STA <READY>";
		break;
	case WIFIMGR_SM_STA_SCANNING:
		str = "STA <SCANNING>";
		break;
	case WIFIMGR_SM_STA_CONNECTING:
		str = "STA <CONNECTING>";
		break;
	case WIFIMGR_SM_STA_CONNECTED:
		str = "STA <CONNECTED>";
		break;
	case WIFIMGR_SM_STA_DISCONNECTING:
		str = "STA <DISCONNECTING>";
		break;
	default:
		str = "STA <UNKNOWN>";
		break;
	}
	return str;
}

static bool is_sta_common_cmd(unsigned int cmd_id)
{
	return ((cmd_id >= WIFIMGR_CMD_GET_STA_CONFIG)
		&& (cmd_id <= WIFIMGR_CMD_GET_STA_CAPA));
}

static bool is_sta_cmd(unsigned int cmd_id)
{
	return ((cmd_id >= WIFIMGR_CMD_OPEN_STA)
		&& (cmd_id < WIFIMGR_CMD_OPEN_AP));
}

static bool is_sta_evt(unsigned int evt_id)
{
	return ((evt_id >= WIFIMGR_EVT_CONNECT)
		&& (evt_id <= WIFIMGR_EVT_SCAN_DONE));
}

int sm_sta_query(struct wifimgr_state_machine *sta_sm)
{
	return sta_sm->state;
}

bool sm_sta_connected(struct wifimgr_state_machine *sta_sm)
{
	return sm_sta_query(sta_sm) == WIFIMGR_SM_STA_CONNECTED;
}

static int sm_sta_query_cmd(struct wifimgr_state_machine *sta_sm,
			    unsigned int cmd_id)
{
	int ret = 0;

	switch (sm_sta_query(sta_sm)) {
	case WIFIMGR_SM_STA_SCANNING:
	case WIFIMGR_SM_STA_CONNECTING:
	case WIFIMGR_SM_STA_DISCONNECTING:
		ret = -EBUSY;
		break;
	}

	return ret;
}

static void sm_sta_step(struct wifimgr_state_machine *sta_sm,
			unsigned int next_state)
{
	sta_sm->old_state = sta_sm->state;
	sta_sm->state = next_state;
	wifimgr_info("(%s) -> (%s)\n", sta_sts2str(sta_sm->old_state),
		     sta_sts2str(sta_sm->state));
}

void sm_sta_step_back(struct wifimgr_state_machine *sta_sm)
{
	wifimgr_info("(%s) -> (%s)\n", sta_sts2str(sta_sm->state),
		     sta_sts2str(sta_sm->old_state));

	sem_wait(&sta_sm->exclsem);
	if (sta_sm->state != sta_sm->old_state)
		sta_sm->state = sta_sm->old_state;
	sem_post(&sta_sm->exclsem);
}

static void sm_sta_cmd_step(struct wifimgr_state_machine *sta_sm,
			    unsigned int cmd_id)
{
	sem_wait(&sta_sm->exclsem);
	sta_sm->old_state = sta_sm->state;

	switch (sta_sm->state) {
	case WIFIMGR_SM_STA_NODEV:
		if (cmd_id == WIFIMGR_CMD_OPEN_STA)
			sm_sta_step(sta_sm, WIFIMGR_SM_STA_READY);
		break;
	case WIFIMGR_SM_STA_READY:
		if (cmd_id == WIFIMGR_CMD_SCAN)
			sm_sta_step(sta_sm, WIFIMGR_SM_STA_SCANNING);
		else if (cmd_id == WIFIMGR_CMD_CONNECT)
			sm_sta_step(sta_sm, WIFIMGR_SM_STA_CONNECTING);
		else if (cmd_id == WIFIMGR_CMD_CLOSE_STA)
			sm_sta_step(sta_sm, WIFIMGR_SM_STA_NODEV);
		break;
	case WIFIMGR_SM_STA_CONNECTED:
		if (cmd_id == WIFIMGR_CMD_SCAN)
			sm_sta_step(sta_sm, WIFIMGR_SM_STA_SCANNING);
		else if (cmd_id == WIFIMGR_CMD_DISCONNECT)
			sm_sta_step(sta_sm, WIFIMGR_SM_STA_DISCONNECTING);
		else if (cmd_id == WIFIMGR_CMD_CLOSE_STA)
			sm_sta_step(sta_sm, WIFIMGR_SM_STA_NODEV);
		break;
	default:
		break;
	}

	sta_sm->cur_cmd = cmd_id;
	sem_post(&sta_sm->exclsem);
}

static void sm_sta_evt_step(struct wifimgr_state_machine *sta_sm,
			    unsigned int evt_id)
{
	sem_wait(&sta_sm->exclsem);

	switch (sta_sm->state) {
	case WIFIMGR_SM_STA_SCANNING:
		if (evt_id == WIFIMGR_EVT_SCAN_DONE)
			sm_sta_step(sta_sm, sta_sm->old_state);
		break;
	case WIFIMGR_SM_STA_CONNECTING:
		if (evt_id == WIFIMGR_EVT_CONNECT)
			sm_sta_step(sta_sm, WIFIMGR_SM_STA_CONNECTED);
		break;
	case WIFIMGR_SM_STA_DISCONNECTING:
		if (evt_id == WIFIMGR_EVT_DISCONNECT)
			sm_sta_step(sta_sm, WIFIMGR_SM_STA_READY);
		break;
	case WIFIMGR_SM_STA_CONNECTED:
		if (evt_id == WIFIMGR_EVT_DISCONNECT)
			sm_sta_step(sta_sm, WIFIMGR_SM_STA_READY);
		break;
	default:
		break;
	}

	sem_post(&sta_sm->exclsem);
}

const char *ap_sts2str(int state)
{
	char *str = NULL;

	switch (state) {
	case WIFIMGR_SM_AP_NODEV:
		str = "AP <UNAVAILABLE>";
		break;
	case WIFIMGR_SM_AP_READY:
		str = "AP <READY>";
		break;
	case WIFIMGR_SM_AP_STARTED:
		str = "AP <STARTED>";
		break;
	default:
		str = "AP <UNKNOWN>";
		break;
	}
	return str;
}

static bool is_ap_common_cmd(unsigned int cmd_id)
{
	return ((cmd_id >= WIFIMGR_CMD_GET_AP_CONFIG)
		&& (cmd_id <= WIFIMGR_CMD_GET_AP_CAPA));
}

static bool is_ap_cmd(unsigned int cmd_id)
{
	return ((cmd_id >= WIFIMGR_CMD_OPEN_AP)
		&& (cmd_id < WIFIMGR_CMD_MAX));
}

static bool is_ap_evt(unsigned int evt_id)
{
	return ((evt_id >= WIFIMGR_EVT_NEW_STATION)
		&& (evt_id <= WIFIMGR_EVT_MAX));
}

int sm_ap_query(struct wifimgr_state_machine *ap_sm)
{
	return ap_sm->state;
}

bool sm_ap_started(struct wifimgr_state_machine *ap_sm)
{
	return sm_ap_query(ap_sm) == WIFIMGR_SM_AP_STARTED;
}

static void sm_ap_step(struct wifimgr_state_machine *ap_sm,
		       unsigned int next_state)
{
	ap_sm->old_state = ap_sm->state;
	ap_sm->state = next_state;
	wifimgr_info("(%s) -> (%s)\n", ap_sts2str(ap_sm->old_state),
		     ap_sts2str(ap_sm->state));
}

static void sm_ap_cmd_step(struct wifimgr_state_machine *ap_sm,
			   unsigned int cmd_id)
{
	sem_wait(&ap_sm->exclsem);
	ap_sm->old_state = ap_sm->state;

	switch (ap_sm->state) {
	case WIFIMGR_SM_AP_NODEV:
		if (cmd_id == WIFIMGR_CMD_OPEN_AP)
			sm_ap_step(ap_sm, WIFIMGR_SM_AP_READY);
		break;
	case WIFIMGR_SM_AP_READY:
		if (cmd_id == WIFIMGR_CMD_START_AP)
			sm_ap_step(ap_sm, WIFIMGR_SM_AP_STARTED);
		else if (cmd_id == WIFIMGR_CMD_CLOSE_AP)
			sm_ap_step(ap_sm, WIFIMGR_SM_AP_NODEV);
		break;
	case WIFIMGR_SM_AP_STARTED:
		if (cmd_id == WIFIMGR_CMD_STOP_AP)
			sm_ap_step(ap_sm, WIFIMGR_SM_AP_READY);
		else if (cmd_id == WIFIMGR_CMD_CLOSE_AP)
			sm_ap_step(ap_sm, WIFIMGR_SM_AP_NODEV);
		break;
	default:
		break;
	}

	ap_sm->cur_cmd = cmd_id;
	sem_post(&ap_sm->exclsem);
}

const char *wifimgr_cmd2str(int cmd)
{
	switch (cmd) {
	C2S(WIFIMGR_CMD_GET_STA_CONFIG)
	C2S(WIFIMGR_CMD_SET_STA_CONFIG)
	C2S(WIFIMGR_CMD_GET_STA_STATUS)
	C2S(WIFIMGR_CMD_GET_STA_CAPA)
	C2S(WIFIMGR_CMD_GET_AP_CONFIG)
	C2S(WIFIMGR_CMD_SET_AP_CONFIG)
	C2S(WIFIMGR_CMD_GET_AP_STATUS)
	C2S(WIFIMGR_CMD_GET_AP_CAPA)
	C2S(WIFIMGR_CMD_OPEN_STA)
	C2S(WIFIMGR_CMD_CLOSE_STA)
	C2S(WIFIMGR_CMD_SCAN)
	C2S(WIFIMGR_CMD_CONNECT)
	C2S(WIFIMGR_CMD_DISCONNECT)
	C2S(WIFIMGR_CMD_OPEN_AP)
	C2S(WIFIMGR_CMD_CLOSE_AP)
	C2S(WIFIMGR_CMD_START_AP)
	C2S(WIFIMGR_CMD_STOP_AP)
	C2S(WIFIMGR_CMD_DEL_STA)
	C2S(WIFIMGR_CMD_SET_MAC_ACL)
	default:
		return "WIFIMGR_CMD_UNKNOWN";
	}
}

const char *wifimgr_evt2str(int evt)
{
	switch (evt) {
	C2S(WIFIMGR_EVT_SCAN_RESULT)
	C2S(WIFIMGR_EVT_SCAN_DONE)
	C2S(WIFIMGR_EVT_CONNECT)
	C2S(WIFIMGR_EVT_DISCONNECT)
	C2S(WIFIMGR_EVT_NEW_STATION)
	default:
		return "WIFIMGR_EVT_UNKNOWN";
	}
}

const char *wifimgr_sts2str_cmd(void *handle, unsigned int cmd_id)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;

	if (is_sta_cmd(cmd_id) || is_sta_common_cmd(cmd_id) == true)
		return sta_sts2str(mgr->sta_sm.state);
	else if (is_ap_cmd(cmd_id) == true || is_ap_common_cmd(cmd_id))
		return ap_sts2str(mgr->ap_sm.state);

	return NULL;
}

const char *wifimgr_sts2str_evt(void *handle, unsigned int evt_id)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;

	if (is_sta_evt(evt_id) == true)
		return sta_sts2str(mgr->sta_sm.state);
	else if (is_ap_evt(evt_id) == true)
		return ap_sts2str(mgr->ap_sm.state);

	return NULL;
}

int wifimgr_sm_query_cmd(void *handle, unsigned int cmd_id)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	int ret = 0;

	if (is_sta_cmd(cmd_id) == true) {
		ret = sm_sta_query_cmd(&mgr->sta_sm, cmd_id);
	} else if (is_ap_cmd(cmd_id) == true) {
		/* softAP does not need query cmd for now */
	}

	return ret;
}

void wifimgr_sm_cmd_step(void *handle, unsigned int cmd_id, char indication)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sm;

	if (is_sta_cmd(cmd_id) == true) {
		sm = &mgr->sta_sm;
		if (!indication) {
			/* Step to next state and start timer on success */
			sm_sta_cmd_step(sm, cmd_id);
			sm_sta_timer_start(sm, cmd_id);
		} else {
			/* Remain current state on failure */
			wifimgr_err("failed to exec [%s]! remains %s\n",
				    wifimgr_cmd2str(cmd_id),
				    sta_sts2str(sm->state));
		}
	} else if (is_ap_cmd(cmd_id) == true) {
		sm = &mgr->ap_sm;
		if (!indication) {
			/* Step to next state and start timer on success */
			sm_ap_cmd_step(sm, cmd_id);
			sm_ap_timer_start(sm, cmd_id);
		} else {
			/* Remain current state on failure */
			wifimgr_err("failed to exec [%s]! remains %s\n",
				    wifimgr_cmd2str(cmd_id),
				    ap_sts2str(sm->state));
		}
	}
}

void wifimgr_sm_evt_step(void *handle, unsigned int evt_id, char indication)
{
	struct wifi_manager *mgr = (struct wifi_manager *)handle;
	struct wifimgr_state_machine *sm = NULL;

	/* Stop timer when receiving an event */
	if (is_sta_evt(evt_id) == true) {
		sm = &mgr->sta_sm;
		sm_sta_timer_stop(sm, evt_id);
		if (!indication) {
			/* Step to next state on success */
			sm_sta_evt_step(sm, evt_id);
			/*softAP does not need step evt for now */
		} else {
			/*Roll back to previous state on failure */
			sm_sta_step_back(sm);
		}
	} else if (is_ap_evt(evt_id) == true) {
		sm = &mgr->ap_sm;
		sm_ap_timer_stop(sm, evt_id);
		/* softAP does not need step evt/back for now */
	}

}

int wifimgr_sm_init(struct wifimgr_state_machine *sm, void *work_handler)
{
	int ret;

	sem_init(&sm->exclsem, 0, 1);
	wifimgr_init_work(&sm->work, work_handler);

	ret = wifimgr_timer_init(&sm->work, wifimgr_timeout, &sm->timerid);
	if (ret < 0)
		wifimgr_err("failed to init WiFi timer! %d\n", ret);

	return ret;
}

void wifimgr_sm_exit(struct wifimgr_state_machine *sm)
{
	if (sm->timerid)
		wifimgr_timer_release(sm->timerid);
}
