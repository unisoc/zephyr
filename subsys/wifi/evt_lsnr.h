/*
 * @file
 * @brief Event listener header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFI_LISTENER_H_
#define _WIFI_LISTENER_H_

#define WIFIMGR_EVT_LISTENER		"wifimgr_evt_listener"
#define WIFIMGR_EVT_LISTENER_PRIORITY	(1)
#define WIFIMGR_EVT_LISTENER_STACKSIZE	(4096)

#define WIFIMGR_EVT_MQUEUE	"wifimgr_evt_mq"
#define WIFIMGR_EVT_MQUEUE_NR	(WIFIMGR_EVT_MAX - 1)
#define WIFIMGR_EVT_RECEIVER_NR	WIFIMGR_EVT_MQUEUE_NR
#define WIFIMGR_FRM_RECEIVER_NR	(1)

enum event_type {
	/*STA*/
	WIFIMGR_EVT_CONNECT,
	WIFIMGR_EVT_DISCONNECT,
	WIFIMGR_EVT_SCAN_RESULT,
	WIFIMGR_EVT_SCAN_DONE,
	/*AP*/
	WIFIMGR_EVT_NEW_STATION,

	WIFIMGR_EVT_MAX,
};

/* CallBack function pointer prototype for notifing upper App */
typedef int (*evt_cb_t) (void *arg);
typedef int (*frm_cb_t) (void *arg);

struct evt_receiver {
	wifimgr_snode_t evt_node;
	unsigned short expected_evt;
	bool oneshot;
	evt_cb_t cb;
	void *arg;
};

struct frm_receiver {
	wifimgr_snode_t frm_node;
	bool oneshot;
	frm_cb_t cb;
	void *arg;
};

struct evt_listener {
	sem_t exclsem;		/* exclusive access to the struct */
	mqd_t mq;

	bool is_setup:1;
	bool is_started:1;
	pthread_t evt_pid;
	pthread_t frm_pid;

	wifimgr_slist_t evt_list;
	wifimgr_slist_t frm_list;
	wifimgr_slist_t free_evt_list;
	wifimgr_slist_t free_frm_list;

	struct evt_receiver evt_pool[WIFIMGR_EVT_RECEIVER_NR];
	struct frm_receiver frm_pool[WIFIMGR_FRM_RECEIVER_NR];
};

/* Structure defining the messages passed to a listening thread */
struct evt_message {
	unsigned int evt_id;	/* Event ID */
	int buf_len;		/* Event message length in bytes */
	void *buf;		/* Event message pointer */
};

int event_listener_add_receiver(struct evt_listener *handle,
				unsigned int evt_id,
				bool oneshot, evt_cb_t cb, void *arg);
int event_listener_remove_receiver(struct evt_listener *handle,
				   unsigned int evt_id);
int wifi_manager_event_listener_init(struct evt_listener *handle);

#endif
