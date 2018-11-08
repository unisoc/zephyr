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

#define sm_timer_stop(timerid) sm_timer_start(timerid, 0)

static int sm_timer_start(timer_t timerid, unsigned int sec)
{
	struct itimerspec todelay;
	int ret;

	/* Start, restart, or stop the timer */
	todelay.it_interval.tv_sec = 0; /* Nonrepeating */
	todelay.it_interval.tv_nsec = 0;
	todelay.it_value.tv_sec = sec;
	todelay.it_value.tv_nsec = 0;

	ret = timer_settime(timerid, 0, &todelay, NULL);
	if (ret < 0) {
		wifimgr_err("failed to set the timer: %d\n", errno);
		ret = -errno;
	}

	return ret;
}

static int sm_timer_init(struct wifimgr_state_machine *sm, void *sighand)
{
	struct sigevent toevent;
	int ret;

	/* Create a POSIX timer to handle timeouts */
	toevent.sigev_value.sival_ptr = sm;
	toevent.sigev_notify = SIGEV_SIGNAL;
	toevent.sigev_notify_function = sighand;
	toevent.sigev_notify_attributes = NULL;

	ret = timer_create(CLOCK_MONOTONIC, &toevent, &sm->timerid);
	if (ret < 0) {
		int errorcode = errno;
		wifimgr_err("failed to create a timer: %d\n", errorcode);
		return -errorcode;
	}

	return ret;
}

#if 0
static int sm_timer_release(timer_t timerid)
{
	int result;
	int ret = 0;

	/* Delete the POSIX timer */
	result = timer_delete(timerid);
	if (result < 0) {
		int errorcode = errno;
		wifimgr_err("failed to delete the timer: %d\n", errorcode);
		ret = -errorcode;
	}

	return ret;
}
#endif

static void sm_sta_timeout(void *sival_ptr)
{
	struct wifimgr_state_machine *sta_sm =
	    (struct wifimgr_state_machine *)sival_ptr;
	struct wifi_manager *mgr =
	    container_of(sta_sm, struct wifi_manager, sta_sm);
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	unsigned int expected_evt;

	/* Remove event receiver, notify the external caller */
	switch (sta_sm->cur_cmd) {
	case WIFIMGR_CMD_SCAN:
		expected_evt = WIFIMGR_EVT_SCAN_DONE;
		event_listener_remove_receiver(&mgr->lsnr,
					       WIFIMGR_EVT_SCAN_RESULT);
		event_listener_remove_receiver(&mgr->lsnr, expected_evt);

		if (cbs && cbs->notify_scan_timeout)
			cbs->notify_scan_timeout();

		wifimgr_err("[%s] timeout!\n",
		       wifimgr_evt2str(expected_evt));
		break;
	case WIFIMGR_CMD_CONNECT:
		expected_evt = WIFIMGR_EVT_CONNECT;
		event_listener_remove_receiver(&mgr->lsnr, expected_evt);

		if (cbs && cbs->notify_connect_timeout)
			cbs->notify_connect_timeout();

		wifimgr_err("[%s] timeout!\n",
		       wifimgr_evt2str(expected_evt));
		break;
	case WIFIMGR_CMD_DISCONNECT:
		expected_evt = WIFIMGR_EVT_DISCONNECT;

		if (cbs && cbs->notify_disconnect_timeout)
			cbs->notify_disconnect_timeout();

		wifimgr_err("[%s] timeout!\n",
		       wifimgr_evt2str(expected_evt));
		break;
	default:
		break;
	}

	sm_sta_step_back(sta_sm);
}

int sm_sta_timer_start(struct wifimgr_state_machine *sta_sm,
		       unsigned int cmd_id)
{
	int ret = 0;

	switch (cmd_id) {
	case WIFIMGR_CMD_SCAN:
		ret = sm_timer_start(sta_sm->timerid, WIFIMGR_SCAN_TIMEOUT);
		break;
	case WIFIMGR_CMD_CONNECT:
	case WIFIMGR_CMD_DISCONNECT:
		ret = sm_timer_start(sta_sm->timerid, WIFIMGR_EVENT_TIMEOUT);
		break;
	default:
		break;
	}

	return ret;
}

int sm_sta_timer_stop(struct wifimgr_state_machine *sta_sm, unsigned int evt_id)
{
	int ret = 0;

	switch (evt_id) {
	case WIFIMGR_EVT_SCAN_DONE:
	case WIFIMGR_EVT_CONNECT:
		ret = sm_timer_stop(sta_sm->timerid);
		break;
	case WIFIMGR_EVT_DISCONNECT:
		if (sta_sm->cur_cmd == WIFIMGR_CMD_DISCONNECT)
			ret = sm_timer_stop(sta_sm->timerid);
		break;
	default:
		break;
	}

	return ret;
}

static int sm_sta_timer_init(struct wifimgr_state_machine *sta_sm)
{
	return sm_timer_init(sta_sm, sm_sta_timeout);
}

#if 0
static int sm_sta_timer_release(struct wifimgr_state_machine *sta_sm)
{
	return sm_timer_release(sta_sm->timerid);
}
#endif

static void sm_ap_timeout(void *sival_ptr)
{
	struct wifimgr_state_machine *ap_sm =
	    (struct wifimgr_state_machine *)sival_ptr;
	struct wifimgr_ctrl_cbs *cbs = wifimgr_get_ctrl_cbs();
	unsigned int expected_evt;

	/* Notify the external caller */
	switch (ap_sm->cur_cmd) {
	case WIFIMGR_CMD_DEL_STATION:
		expected_evt = WIFIMGR_EVT_NEW_STATION;

		if (cbs && cbs->notify_del_station_timeout)
			cbs->notify_del_station_timeout();

		wifimgr_err("[%s] timeout!\n",
		       wifimgr_evt2str(expected_evt));
		break;
	default:
		break;
	}
}

int sm_ap_timer_start(struct wifimgr_state_machine *ap_sm, unsigned int cmd_id)
{
	int ret = 0;

	switch (cmd_id) {
	case WIFIMGR_CMD_DEL_STATION:
		ret = sm_timer_start(ap_sm->timerid, WIFIMGR_EVENT_TIMEOUT);
		break;
	default:
		break;
	}

	return ret;
}

int sm_ap_timer_stop(struct wifimgr_state_machine *ap_sm, unsigned int evt_id)
{
	int ret = 0;

	switch (evt_id) {
	case WIFIMGR_EVT_NEW_STATION:
		if (ap_sm->cur_cmd == WIFIMGR_CMD_DEL_STATION)
			ret = sm_timer_stop(ap_sm->timerid);
		break;
	default:
		break;
	}

	return ret;
}

static int sm_ap_timer_init(struct wifimgr_state_machine *ap_sm)
{
	return sm_timer_init(ap_sm, sm_ap_timeout);
}

#if 0
static int sm_ap_timer_release(struct wifimgr_state_machine *ap_sm)
{
	return sm_timer_release(ap_sm->timerid);
}
#endif

const char *wifimgr_cmd2str(int cmd)
{
	switch (cmd) {
	E2S(WIFIMGR_CMD_SET_STA_CONFIG)
	E2S(WIFIMGR_CMD_SET_AP_CONFIG)
	E2S(WIFIMGR_CMD_GET_STA_CONFIG)
	E2S(WIFIMGR_CMD_GET_AP_CONFIG)
	E2S(WIFIMGR_CMD_GET_STA_STATUS)
	E2S(WIFIMGR_CMD_GET_AP_STATUS)
	E2S(WIFIMGR_CMD_OPEN_STA)
	E2S(WIFIMGR_CMD_CLOSE_STA)
	E2S(WIFIMGR_CMD_SCAN)
	E2S(WIFIMGR_CMD_CONNECT)
	E2S(WIFIMGR_CMD_DISCONNECT)
	E2S(WIFIMGR_CMD_OPEN_AP)
	E2S(WIFIMGR_CMD_CLOSE_AP)
	E2S(WIFIMGR_CMD_START_AP)
	E2S(WIFIMGR_CMD_STOP_AP)
	E2S(WIFIMGR_CMD_DEL_STATION)
	default:
		return "WIFIMGR_CMD_UNKNOWN";
	}
}

const char *wifimgr_evt2str(int evt)
{
	switch (evt) {
	E2S(WIFIMGR_EVT_SCAN_RESULT)
	E2S(WIFIMGR_EVT_SCAN_DONE)
	E2S(WIFIMGR_EVT_CONNECT)
	E2S(WIFIMGR_EVT_DISCONNECT)
	E2S(WIFIMGR_EVT_NEW_STATION)
	default:
		return "WIFIMGR_EVT_UNKNOWN";
	}
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

bool is_sta_cmd(unsigned int cmd_id)
{
	return ((cmd_id >= WIFIMGR_CMD_GET_STA_STATUS)
		&& (cmd_id < WIFIMGR_CMD_GET_AP_STATUS));
}

bool is_sta_evt(unsigned int evt_id)
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

int sm_sta_query_cmd(struct wifimgr_state_machine *sta_sm, unsigned int cmd_id)
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

void sm_sta_step_cmd(struct wifimgr_state_machine *sta_sm, unsigned int cmd_id)
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

void sm_sta_step_evt(struct wifimgr_state_machine *sta_sm, unsigned int evt_id)
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

int sm_sta_init(struct wifimgr_state_machine *sta_sm)
{
	struct wifi_manager *mgr =
	    container_of(sta_sm, struct wifi_manager, sta_sm);
	struct cmd_processor *prcs = &mgr->prcs;
	int ret;

	sta_sm->state = sta_sm->old_state = WIFIMGR_SM_STA_NODEV;
	sem_init(&sta_sm->exclsem, 0, 1);

	/* Register default STA commands */
	command_processor_register_sender(prcs, WIFIMGR_CMD_SET_STA_CONFIG,
					  wifi_manager_set_sta_config,
					  &mgr->sta_conf);
	command_processor_register_sender(prcs, WIFIMGR_CMD_GET_STA_CONFIG,
					  wifi_manager_get_sta_config, mgr);
	command_processor_register_sender(prcs, WIFIMGR_CMD_GET_STA_STATUS,
					  wifi_manager_get_sta_status, mgr);
	command_processor_register_sender(prcs, WIFIMGR_CMD_OPEN_STA,
					  wifi_manager_open_station, mgr);

	ret = sm_sta_timer_init(sta_sm);
	if (ret < 0)
		wifimgr_err("failed to init WiFi STA timer!\n");

	return ret;
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

bool is_ap_cmd(unsigned int cmd_id)
{
	return ((cmd_id >= WIFIMGR_CMD_GET_AP_STATUS) && (cmd_id < WIFIMGR_CMD_MAX));
}

int sm_ap_query(struct wifimgr_state_machine *ap_sm)
{
	return ap_sm->state;
}

bool is_ap_evt(unsigned int evt_id)
{
	return ((evt_id >= WIFIMGR_EVT_NEW_STATION)
		&& (evt_id <= WIFIMGR_EVT_MAX));
}

static void sm_ap_step(struct wifimgr_state_machine *ap_sm,
		       unsigned int next_state)
{
	ap_sm->old_state = ap_sm->state;
	ap_sm->state = next_state;
	wifimgr_info("(%s) -> (%s)\n", ap_sts2str(ap_sm->old_state),
	       ap_sts2str(ap_sm->state));
}

void sm_ap_step_cmd(struct wifimgr_state_machine *ap_sm, unsigned int cmd_id)
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

int sm_ap_init(struct wifimgr_state_machine *ap_sm)
{
	struct wifi_manager *mgr =
	    container_of(ap_sm, struct wifi_manager, ap_sm);
	struct cmd_processor *prcs = &mgr->prcs;
	int ret;

	ap_sm->state = ap_sm->old_state = WIFIMGR_SM_AP_NODEV;
	sem_init(&ap_sm->exclsem, 0, 1);

	/* Register default AP commands */
	command_processor_register_sender(prcs, WIFIMGR_CMD_SET_AP_CONFIG,
					  wifi_manager_set_ap_config,
					  &mgr->ap_conf);
	command_processor_register_sender(prcs, WIFIMGR_CMD_GET_AP_CONFIG,
					  wifi_manager_get_ap_config, mgr);
	command_processor_register_sender(prcs, WIFIMGR_CMD_GET_AP_STATUS,
					  wifi_manager_get_ap_status, mgr);
	command_processor_register_sender(prcs, WIFIMGR_CMD_OPEN_AP,
					  wifi_manager_open_softap, mgr);

	ret = sm_ap_timer_init(ap_sm);
	if (ret < 0)
		wifimgr_err("failed to init WiFi AP timer!\n");

	return ret;
}

bool is_common_cmd(unsigned int cmd_id)
{
	return (cmd_id < WIFIMGR_CMD_GET_STA_STATUS);
}
