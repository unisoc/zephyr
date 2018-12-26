/*
 * @file
 * @brief Command processor related functions
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

K_THREAD_STACK_ARRAY_DEFINE(cmd_stacks, 1, WIFIMGR_CMD_PROCESSOR_STACKSIZE);

int wifimgr_send_cmd(unsigned int cmd_id, void *buf, int buf_len)
{
	struct mq_des *mq;
	struct mq_attr attr;
	struct cmd_message msg;
	struct timespec ts;
	int prio;
	int ret;

	attr.mq_maxmsg = WIFIMGR_CMD_MQUEUE_NR;
	attr.mq_msgsize = sizeof(msg);
	attr.mq_flags = 0;
	mq = mq_open(WIFIMGR_CMD_MQUEUE, O_RDWR | O_CREAT, 0666, &attr);
	if (!mq) {
		wifimgr_err("failed to open command queue %s!\n",
			    WIFIMGR_CMD_MQUEUE);
		return -errno;
	}

	msg.cmd_id = cmd_id;
	msg.reply = 0;
	msg.buf_len = buf_len;
	msg.buf = NULL;
	if (buf_len) {
		msg.buf = malloc(buf_len);
		memcpy(msg.buf, buf, buf_len);
	}

	ret = mq_send(mq, (const char *)&msg, sizeof(msg), 0);
	if (ret < 0) {
		wifimgr_err("failed to send [%s]: %d, errno %d!\n",
			    wifimgr_cmd2str(msg.cmd_id), ret, errno);
		ret = -errno;
	} else {
		wifimgr_dbg("send [%s], buf: 0x%08x\n",
			    wifimgr_cmd2str(msg.cmd_id), *(int *)msg.buf);

		ret = clock_gettime(CLOCK_MONOTONIC, &ts);
		if (ret)
			wifimgr_err("failed to get clock time: %d!\n", ret);
		ts.tv_sec += WIFIMGR_CMD_TIMEOUT;
		ret =
		    mq_timedreceive(mq, (char *)&msg, sizeof(msg), &prio, &ts);
		if (ret != sizeof(struct cmd_message)) {
			wifimgr_err
			    ("failed to get command reply: %d, errno %d!\n",
			     ret, errno);
			if (errno == ETIME)
				wifimgr_err("[%s] timeout!\n",
					    wifimgr_cmd2str(msg.cmd_id));
			ret = -errno;
		} else {
			wifimgr_dbg("recv [%s] reply: %d\n",
				    wifimgr_cmd2str(msg.cmd_id), msg.reply);
			ret = msg.reply;
			if (ret)
				wifimgr_err("failed to exec [%s]: %d!\n",
					    wifimgr_cmd2str(msg.cmd_id), ret);
		}
	}

	free(msg.buf);
	mq_close(mq);

	return ret;
}

int cmd_processor_add_sender(struct cmd_processor *handle, unsigned int cmd_id,
			     cmd_func_t fn, void *arg)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct cmd_sender *sndr = &prcs->cmd_pool[cmd_id];

	if (!prcs || !fn)
		return -EINVAL;

	sndr->fn = fn;
	sndr->arg = arg;

	return 0;
}

int cmd_processor_remove_sender(struct cmd_processor *handle,
				unsigned int cmd_id)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct cmd_sender *sndr = &prcs->cmd_pool[cmd_id];

	if (!prcs)
		return -EINVAL;

	sndr->fn = NULL;
	sndr->arg = NULL;

	return 0;
}

static void cmd_processor_post_process(void *handle,
				       struct cmd_message *msg, int reply)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	int ret;

	msg->reply = reply;
	ret =
	    mq_send(prcs->mq, (const char *)msg, sizeof(struct cmd_message), 0);
	if (ret < 0) {
		wifimgr_err("failed to send [%s] reply: %d, errno %d!\n",
			    wifimgr_cmd2str(msg->cmd_id), ret, errno);
	} else {
		wifimgr_dbg("send [%s] reply: %d\n",
			    wifimgr_cmd2str(msg->cmd_id), msg->reply);
	}
}

static void *cmd_processor(void *handle)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct cmd_sender *sndr;
	struct wifi_manager *mgr =
	    container_of(prcs, struct wifi_manager, prcs);
	struct cmd_message msg;
	int prio;
	int ret;

	wifimgr_dbg("starting %s, pid=%p\n", __func__, pthread_self());

	if (!prcs)
		return NULL;

	while (prcs->is_started) {
		/* Wait for commands */
		ret = mq_receive(prcs->mq, (char *)&msg, sizeof(msg), &prio);
		if (ret != sizeof(struct cmd_message)) {
			wifimgr_err("failed to get command: %d, errno %d!\n",
				    ret, errno);
			continue;
		} else if (msg.reply) {
			/* Drop command reply when receiving it */
			wifimgr_err("recv [%s] reply: %d? drop it\n",
				    wifimgr_cmd2str(msg.cmd_id), msg.reply);
			continue;
		}

		wifimgr_dbg("recv [%s], buf: 0x%08x\n",
			    wifimgr_cmd2str(msg.cmd_id), *(int *)msg.buf);

		/* Ask state machine whether the command could be executed */
		ret = wifimgr_sm_query_cmd(mgr, msg.cmd_id);
		if (ret) {
			cmd_processor_post_process(prcs, &msg, ret);

			if (ret == -EBUSY)
				wifimgr_err("Busy! try again later\n");
			continue;
		}

		/* Initialize driver interface for the first time running */
		ret = wifimgr_low_level_init(mgr, msg.cmd_id);
		if (ret == -ENODEV) {
			cmd_processor_post_process(prcs, &msg, ret);
			wifimgr_err("No such device!\n");
			continue;
		}

		sem_wait(&prcs->exclsem);

		sndr = &prcs->cmd_pool[msg.cmd_id];
		if (sndr->fn) {
			if (msg.buf_len)
				memcpy(sndr->arg, msg.buf, msg.buf_len);

			/* Call command function */
			ret = sndr->fn(sndr->arg);
			if (!ret) {
				/*Step to next state when successful sending */
				wifimgr_sm_step_cmd(mgr, msg.cmd_id);
				/*Start timer then */
				wifimgr_sm_start_timer(mgr, msg.cmd_id);
			} else {
				/*Remain current state when failed sending */
				wifimgr_err("failed to exec [%s]! remains %s\n",
					    wifimgr_cmd2str(msg.cmd_id),
					    wifimgr_sts2str_cmd(mgr,
								msg.cmd_id));
			}
		} else {
			wifimgr_err("[%s] not allowed under %s!\n",
				    wifimgr_cmd2str(msg.cmd_id),
				    wifimgr_sts2str_cmd(mgr, msg.cmd_id));
			ret = -EPERM;
		}

		sem_post(&prcs->exclsem);
		cmd_processor_post_process(prcs, &msg, ret);
	}

	return NULL;
}

int wifimgr_cmd_processor_init(struct cmd_processor *handle)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct mq_attr attr;
	pthread_attr_t tattr;
	struct sched_param sparam;
	int ret;

	if (!prcs)
		return -EINVAL;

	/* Fill in attributes for message queue */
	attr.mq_maxmsg = WIFIMGR_CMD_MQUEUE_NR;
	attr.mq_msgsize = sizeof(struct cmd_message);
	attr.mq_flags = 0;

	/* open message queue of command sender */
	prcs->mq = mq_open(WIFIMGR_CMD_MQUEUE, O_RDWR | O_CREAT, 0666, &attr);
	if (!prcs->mq) {
		wifimgr_err("failed to open command queue %s!\n",
			    WIFIMGR_CMD_MQUEUE);
		return -errno;
	}

	sem_init(&prcs->exclsem, 0, 1);
	prcs->is_setup = true;
	prcs->is_started = true;

	/* Starts internal threads to process commands */
	pthread_attr_init(&tattr);
	sparam.priority = WIFIMGR_CMD_PROCESSOR_PRIORITY;
	pthread_attr_setschedparam(&tattr, &sparam);
	pthread_attr_setstack(&tattr, &cmd_stacks[0][0],
			      WIFIMGR_CMD_PROCESSOR_STACKSIZE);
	pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);

	ret = pthread_create(&prcs->pid, &tattr, cmd_processor, prcs);
	if (ret) {
		wifimgr_err("failed to start %s!\n", WIFIMGR_CMD_PROCESSOR);
		prcs->is_setup = false;
		mq_close(prcs->mq);
		return ret;
	}
	wifimgr_dbg("started %s, pid=%p\n", WIFIMGR_CMD_PROCESSOR, prcs->pid);

	return 0;
}
