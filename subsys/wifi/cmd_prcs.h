/*
 * @file
 * @brief Command processor header
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _WIFI_PROCESSOR_H_
#define _WIFI_PROCESSOR_H_

#define WIFIMGR_CMD_PROCESSOR		"wifimgr_cmd_processor"
#define WIFIMGR_CMD_PROCESSOR_PRIORITY	(1)
#define WIFIMGR_CMD_PROCESSOR_STACKSIZE	(4096)

#define WIFIMGR_CMD_MQUEUE	"wifimgr_cmd_mq"
#define WIFIMGR_CMD_MQUEUE_NR	(WIFIMGR_CMD_MAX - 1)
#define WIFIMGR_CMD_SENDER_NR	WIFIMGR_CMD_MAX

enum wifimgr_cmd {
	/*Common command */
	WIFIMGR_CMD_GET_STA_CONFIG,
	WIFIMGR_CMD_SET_STA_CONFIG,
	WIFIMGR_CMD_GET_AP_CONFIG,
	WIFIMGR_CMD_SET_AP_CONFIG,
	/*STA command */
	WIFIMGR_CMD_GET_STA_STATUS,
	WIFIMGR_CMD_OPEN_STA,
	WIFIMGR_CMD_CLOSE_STA,
	WIFIMGR_CMD_SCAN,
	WIFIMGR_CMD_CONNECT,
	WIFIMGR_CMD_DISCONNECT,
	/*AP command */
	WIFIMGR_CMD_GET_AP_STATUS,
	WIFIMGR_CMD_OPEN_AP,
	WIFIMGR_CMD_CLOSE_AP,
	WIFIMGR_CMD_START_AP,
	WIFIMGR_CMD_STOP_AP,
	WIFIMGR_CMD_DEL_STATION,

	WIFIMGR_CMD_MAX,
};

/* Function pointer prototype for commands */
typedef int (*cmd_func_t) (void *arg);

struct cmd_sender {
	cmd_func_t fn;
	void *arg;
};

struct cmd_processor {
	sem_t exclsem;		/* exclusive access to the struct */
	mqd_t mq;

	bool is_setup:1;
	bool is_started:1;
	pthread_t pid;

	struct cmd_sender cmd_pool[WIFIMGR_CMD_SENDER_NR];
};

/* Structure defining the messages passed to a processor thread */
struct cmd_message {
	wifimgr_snode_t cmd_node;
	unsigned int cmd_id;	/* Command ID */
	int reply;		/* Command reply */
	int buf_len;		/* Command message length in bytes */
	void *buf;		/* Command message pointer */
};

int wifimgr_send_cmd(unsigned int cmd_id, void *buf, int buf_len);

int cmd_processor_add_sender(struct cmd_processor *handle, unsigned int cmd_id,
			     cmd_func_t fn, void *arg);
int cmd_processor_remove_sender(struct cmd_processor *handle,
				unsigned int cmd_id);
int wifimgr_cmd_processor_init(struct cmd_processor *handle);

#endif
