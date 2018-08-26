/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifimgr.h"

static int wifimgr_ctrl_iface_send_cmd(unsigned int cmd_id, void *buf,
				       int buf_len)
{
	struct mq_des *mq;
	struct mq_attr attr;
	struct cmd_message msg;
	int ret;

	attr.mq_maxmsg = WIFIMGR_CMD_MQUEUE_NR;
	attr.mq_msgsize = sizeof(struct cmd_message);
	attr.mq_flags = 0;
	mq = mq_open(WIFIMGR_CMD_MQUEUE, O_WRONLY | O_CREAT, 0666, &attr);
	if (!mq) {
		syslog(LOG_ERR, "failed to open command queue %s!\n",
		       WIFIMGR_CMD_MQUEUE);
		return -errno;
	}

	msg.cmd_id = cmd_id;
	msg.buf_len = buf_len;
	msg.buf = NULL;
	if (buf_len) {
		msg.buf = malloc(buf_len);
		memcpy(msg.buf, buf, buf_len);
	}

	ret = mq_send(mq, (const char *)&msg, sizeof(msg), 0);
	if (ret < 0)
		syslog(LOG_ERR, "failed to send [%s]: %d, errno %d!\n",
		       wifimgr_cmd2str(msg.cmd_id), ret, errno);
	else
		syslog(LOG_DEBUG, "send [%s], buf: 0x%08x\n",
		       wifimgr_cmd2str(msg.cmd_id), *(int *)msg.buf);

	mq_close(mq);

	return ret;
}

int wifimgr_ctrl_iface_set_conf(char *ssid, char *bssid,
				       char *passphrase, unsigned char band,
				       unsigned char channel)
{
	struct wifimgr_sta_config sta_conf;

	memset(&sta_conf, 0, sizeof(sta_conf));
	strcpy(sta_conf.ssid, ssid);
	strcpy(sta_conf.bssid, bssid);
	strcpy(sta_conf.passphrase, passphrase);
	sta_conf.band = band;
	sta_conf.channel = channel;

	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_SET_STA_CONFIG,
					   &sta_conf, sizeof(sta_conf));
}

int wifimgr_ctrl_iface_get_conf(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_GET_STA_CONFIG, NULL, 0);
}

int wifimgr_ctrl_iface_get_status(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_GET_STATUS, NULL, 0);
}

int wifimgr_ctrl_iface_open_sta(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_OPEN_STA, NULL, 0);
}

int wifimgr_ctrl_iface_close_sta(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_CLOSE_STA, NULL, 0);
}

int wifimgr_ctrl_iface_scan(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_SCAN, NULL, 0);
}

int wifimgr_ctrl_iface_connect(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_CONNECT, NULL, 0);
}

int wifimgr_ctrl_iface_disconnect(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_DISCONNECT, NULL, 0);
}

int wifimgr_ctrl_iface_open_ap(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_OPEN_AP, NULL, 0);
}

int wifimgr_ctrl_iface_close_ap(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_CLOSE_AP, NULL, 0);
}

static const struct wifimgr_ctrl_ops wifimgr_ops = {
	.size = sizeof(struct wifimgr_ctrl_ops),
	.set_conf = wifimgr_ctrl_iface_set_conf,
	.get_conf = wifimgr_ctrl_iface_get_conf,
	.get_status = wifimgr_ctrl_iface_get_status,
	.open_sta = wifimgr_ctrl_iface_open_sta,
	.close_sta = wifimgr_ctrl_iface_close_sta,
	.scan = wifimgr_ctrl_iface_scan,
	.connect = wifimgr_ctrl_iface_connect,
	.disconnect = wifimgr_ctrl_iface_disconnect,
};

const struct wifimgr_ctrl_ops *wifimgr_get_ctrl_ops(void)
{
	return &wifimgr_ops;
}

const struct wifimgr_ctrl_cbs *wifimgr_get_ctrl_cbs(void)
{
	return NULL;
}
