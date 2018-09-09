/*
 * @file
 * @brief Command processor related functions
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifimgr.h"

K_THREAD_STACK_ARRAY_DEFINE(cmd_stacks, 1, WIFIMGR_CMD_PROCESSOR_STACKSIZE);

int command_processor_register_sender(struct cmd_processor *handle,
				      unsigned int cmd_id, cmd_func_t fn,
				      void *arg)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct cmd_sender *sndr = &prcs->cmd_pool[cmd_id];

	if (!prcs || !fn)
		return -EINVAL;

	/*sem_wait(&prcs->exclsem); */
	sndr->fn = fn;
	sndr->arg = arg;
	/*sem_post(&prcs->exclsem); */

	return 0;
}

int command_processor_unregister_sender(struct cmd_processor *handle,
					unsigned int cmd_id)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct cmd_sender *sndr = &prcs->cmd_pool[cmd_id];

	if (!prcs)
		return -EINVAL;

	/*sem_wait(&prcs->exclsem); */
	sndr->fn = NULL;
	sndr->arg = NULL;
	/*sem_post(&prcs->exclsem); */

	return 0;
}

static void command_processor_post_process(void *handle,
					   struct cmd_message *msg, int reply)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	int ret;

	msg->reply = reply;
	ret =
	    mq_send(prcs->mq, (const char *)msg, sizeof(struct cmd_message), 0);
	if (ret < 0)
		syslog(LOG_ERR,
		       "failed to send [%s] reply: %d, errno %d!\n",
		       wifimgr_cmd2str(msg->cmd_id), ret, errno);
	else
		syslog(LOG_DEBUG, "send [%s] reply: %d\n",
		       wifimgr_cmd2str(msg->cmd_id), msg->reply);
}

static void *command_processor(void *handle)
{
	struct cmd_processor *prcs = (struct cmd_processor *)handle;
	struct cmd_sender *sndr;
	struct wifi_manager *mgr =
	    container_of(prcs, struct wifi_manager, prcs);
	struct cmd_message msg;
	int prio;
	int ret;

	syslog(LOG_INFO, "starting %s, pid=%p\n", __func__, pthread_self());

	if (!prcs)
		return NULL;

	while (prcs->is_started) {
		/* Wait for commands */
		ret = mq_receive(prcs->mq, (char *)&msg, sizeof(msg), &prio);
		if (ret != sizeof(struct cmd_message)) {
			syslog(LOG_ERR,
			       "failed to get command: %d, errno %d!\n", ret,
			       errno);
			continue;
		} else if (msg.reply) {
			/* Drop command reply when receiving it */
			syslog(LOG_ERR, "recv [%s] reply: %d? drop it\n",
			       wifimgr_cmd2str(msg.cmd_id), msg.reply);
			continue;
		}

		syslog(LOG_DEBUG, "recv [%s], buf: 0x%08x\n",
		       wifimgr_cmd2str(msg.cmd_id), *(int *)msg.buf);

		/* Ask state machine whether the command could be executed */
		ret = wifi_manager_sm_query_cmd(mgr, msg.cmd_id);
		if (ret) {
			command_processor_post_process(prcs, &msg, ret);

			if (ret == -EBUSY)
				syslog(LOG_ERR, "Busy! try again later\n");
			continue;
		}

		/* Initialize driver interface for the first time running */
		ret = wifi_manager_low_level_init(mgr, msg.cmd_id);
		if (ret == -ENODEV) {
			command_processor_post_process(prcs, &msg, ret);
			syslog(LOG_ERR, "No such device!\n");
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
				wifi_manager_sm_step_cmd(mgr, msg.cmd_id);
				/*Start timer then */
				wifi_manager_sm_start_timer(mgr, msg.cmd_id);
			} else {
				/*Remain current state when failed sending */
				syslog(LOG_ERR,
				       "failed to exec [%s]! remains %s\n",
				       wifimgr_cmd2str(msg.cmd_id),
				       wifimgr_sts2str_cmd(mgr, msg.cmd_id));
			}
		} else {
			syslog(LOG_ERR, "[%s] not allowed under %s!\n",
			       wifimgr_cmd2str(msg.cmd_id),
			       wifimgr_sts2str_cmd(mgr, msg.cmd_id));
			ret = -EPERM;
		}

		sem_post(&prcs->exclsem);
		command_processor_post_process(prcs, &msg, ret);
	}

	return NULL;
}

int wifi_manager_command_processor_init(struct cmd_processor *handle)
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
		syslog(LOG_ERR, "failed to open command queue %s!\n",
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

	ret = pthread_create(&prcs->pid, &tattr, command_processor, prcs);
	if (ret) {
		syslog(LOG_ERR, "failed to start %s!\n", WIFIMGR_CMD_PROCESSOR);
		prcs->is_setup = false;
		mq_close(prcs->mq);
		return ret;
	}
	syslog(LOG_INFO, "started %s, pid=%p\n", WIFIMGR_CMD_PROCESSOR,
	       prcs->pid);

	return 0;
}
