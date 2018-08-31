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

int wifimgr_ctrl_iface_set_conf(char *iface_name, char *ssid,
				       char *passphrase, unsigned char channel)
{
	struct wifimgr_config conf;
	unsigned int cmd_id = 0;

	memset(&conf, 0, sizeof(conf));
	strcpy(conf.ssid, ssid);
	strcpy(conf.passphrase, passphrase);

	if(!strcmp(iface_name, WIFIMGR_IFACE_STA_NAME)) {
		cmd_id = WIFIMGR_CMD_SET_STA_CONFIG;
	} else if(!strcmp(iface_name, WIFIMGR_IFACE_AP_NAME)) {
		cmd_id = WIFIMGR_CMD_SET_AP_CONFIG;
		/*conf.band = band;*/
		conf.channel = channel;
	} else
		return -1;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, &conf, sizeof(conf));
}

int wifimgr_ctrl_iface_get_conf(char *iface_name)
{
	unsigned int cmd_id = 0;

	if(!strcmp(iface_name, WIFIMGR_IFACE_STA_NAME))
		cmd_id = WIFIMGR_CMD_GET_STA_CONFIG;
	else if(!strcmp(iface_name, WIFIMGR_IFACE_AP_NAME))
		cmd_id = WIFIMGR_CMD_GET_AP_CONFIG;
	else
		return -1;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, NULL, 0);
}

int wifimgr_ctrl_iface_get_status(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_GET_STATUS, NULL, 0);
}

int wifimgr_ctrl_iface_open(char *iface_name)
{
	unsigned int cmd_id = 0;

	if(!strcmp(iface_name, WIFIMGR_IFACE_STA_NAME))
		cmd_id = WIFIMGR_CMD_OPEN_STA;
	else if(!strcmp(iface_name, WIFIMGR_IFACE_AP_NAME))
		cmd_id = WIFIMGR_CMD_OPEN_AP;
	else
		return -1;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, NULL, 0);
}

int wifimgr_ctrl_iface_close(char *iface_name)
{
	unsigned int cmd_id = 0;

	if(!strcmp(iface_name, WIFIMGR_IFACE_STA_NAME))
		cmd_id = WIFIMGR_CMD_CLOSE_STA;
	else if(!strcmp(iface_name, WIFIMGR_IFACE_AP_NAME))
		cmd_id = WIFIMGR_CMD_CLOSE_AP;
	else
		return -1;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, NULL, 0);
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

int wifimgr_ctrl_iface_start_ap(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_START_AP, NULL, 0);
}

int wifimgr_ctrl_iface_stop_ap(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_STOP_AP, NULL, 0);
}

int wifimgr_ctrl_iface_del_station(unsigned char *mac)
{
	char mac_addr[WIFIMGR_ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

	if(mac)
		strncpy(mac_addr, mac, WIFIMGR_ETH_ALEN);

	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_DEL_STATION, mac_addr, WIFIMGR_ETH_ALEN);
}

static const struct wifimgr_ctrl_ops wifimgr_ops = {
	.size = sizeof(struct wifimgr_ctrl_ops),
	.set_conf = wifimgr_ctrl_iface_set_conf,
	.get_conf = wifimgr_ctrl_iface_get_conf,
	.get_status = wifimgr_ctrl_iface_get_status,
	.open = wifimgr_ctrl_iface_open,
	.close = wifimgr_ctrl_iface_close,
	.scan = wifimgr_ctrl_iface_scan,
	.connect = wifimgr_ctrl_iface_connect,
	.disconnect = wifimgr_ctrl_iface_disconnect,
	.start_ap = wifimgr_ctrl_iface_start_ap,
	.stop_ap = wifimgr_ctrl_iface_stop_ap,
	.del_station = wifimgr_ctrl_iface_del_station,
};

const struct wifimgr_ctrl_ops *wifimgr_get_ctrl_ops(void)
{
	return &wifimgr_ops;
}

const struct wifimgr_ctrl_cbs *wifimgr_get_ctrl_cbs(void)
{
	return NULL;
}
