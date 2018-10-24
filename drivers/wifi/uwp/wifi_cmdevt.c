/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <logging/sys_log.h>
#include <string.h>
#include <sipc.h>
#include <sblock.h>
#include <net/wifimgr_drv.h>

#include "wifi_main.h"

#define RECV_BUF_SIZE (128)
static unsigned char recv_buf[RECV_BUF_SIZE];
static unsigned int recv_len;
static unsigned int seq = 1;
static struct k_sem cmd_sem;

static const u16_t CRC_table[] = {
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00,
	0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401,
	0x5000, 0x9C01, 0x8801, 0x4400,
};

/* 5G channel list. */
static const u16_t channels_5g_scan_table[] = {
	36, 40, 44, 48, 52,
	56, 60, 64, 100, 104,
	108, 112, 116, 120, 124,
	128, 132, 136, 140, 144,
	149, 153, 157, 161, 165
};

static u16_t CRC16(const u8_t *buf, u16_t len)
{
	u16_t CRC = 0xFFFF;
	u16_t i;
	u8_t ch_char;

	for (i = 0; i < len; i++) {
		ch_char = *buf++;
		CRC = CRC_table[(ch_char ^ CRC) & 15] ^ (CRC >> 4);
		CRC = CRC_table[((ch_char >> 4) ^ CRC) & 15] ^ (CRC >> 4);
	}
	return CRC;
}

static inline int check_cmdevt_len(int input_len, int expected_len)
{
	if (input_len != expected_len) {
		SYS_LOG_ERR("Invalid len %d, expected len %d",
				input_len, expected_len);
		return -1;
	}

	return 0;
}

int wifi_cmd_load_ini(const u8_t *data, u32_t len, u8_t sec_num)
{
	int ret = -1;
	u16_t crc = 0;
	int cmd_len;
	struct cmd_download_ini *cmd;

	/* Calculate total command length. */
	cmd_len = sizeof(*cmd) + len + sizeof(crc);

	cmd = k_malloc(cmd_len);
	if (cmd == NULL) {
		SYS_LOG_ERR("cmd is null");
		return ret;
	}

	/* Calc CRC value. */
	crc = CRC16(data, len);
	SYS_LOG_DBG("sec: %d, len: %d, CRC value: 0x%x",
		    sec_num, len, crc);

	memset(cmd, 0, cmd_len);
	cmd->sec_num = sec_num;

	/* Copy data after section num. */
	memcpy(cmd->data, data, len);
	/* Put CRC value at the tail of INI data. */
	memcpy(cmd->data + len, &crc, sizeof(crc));

	ret = wifi_cmd_send(WIFI_CMD_DOWNLOAD_INI, (char *)cmd,
			    cmd_len, NULL, 0);
	if (ret) {
		SYS_LOG_ERR("load ini fail");
		k_free(cmd);
		return ret;
	}

	k_free(cmd);

	return 0;
}

int wifi_cmd_scan(struct wifi_priv *priv)
{
	int ret = -1;
	int ssid_len = 0;
	int cmd_len = 0;
	struct cmd_scan *cmd;
	u16_t channels_5g_cnt = ARRAY_SIZE(channels_5g_scan_table);

	/* Calculate total command length. */
	cmd_len = sizeof(*cmd) +
		  ssid_len + sizeof(channels_5g_scan_table);

	cmd = k_malloc(cmd_len);
	if (cmd == NULL) {
		SYS_LOG_ERR("cmd is null");
		return ret;
	}

	memset(cmd, 0, cmd_len);

	cmd->channels = 0x3FFF;
	cmd->channels_5g_cnt = channels_5g_cnt;
	memcpy(cmd->channels_5g, channels_5g_scan_table,
	       sizeof(channels_5g_scan_table));

	ret = wifi_cmd_send(WIFI_CMD_SCAN, (char *)cmd,
			    cmd_len, NULL, NULL);
	if (ret) {
		SYS_LOG_ERR("scan cmd fail");
		k_free(cmd);
		return ret;
	}

	k_free(cmd);

	return 0;
}

int wifi_cmd_connect(struct wifi_priv *priv,
		     struct wifi_connect_req_params *params)
{
	int ret;
	struct cmd_connect cmd;

	memset(&cmd, 0, sizeof(cmd));

	if (params->ssid == NULL) {
		SYS_LOG_ERR("Invalid SSID.");
		return -1;
	}

	cmd.channel = params->channel;
	cmd.ssid_len = params->ssid_length;
	cmd.passphrase_len = params->psk_length;

	memcpy(cmd.ssid, params->ssid, cmd.ssid_len);
	memcpy(cmd.passphrase, params->psk, cmd.passphrase_len);

	ret = wifi_cmd_send(WIFI_CMD_CONNECT, (char *)&cmd,
			    sizeof(cmd), NULL, NULL);
	if (ret) {
		SYS_LOG_ERR("connect cmd fail");
		return ret;
	}

	return 0;
}

int wifi_cmd_disconnect(struct wifi_priv *priv)
{
	int ret;
	struct cmd_disconnect cmd;

	cmd.reason_code = 0;

	ret = wifi_cmd_send(WIFI_CMD_DISCONNECT, (char *)&cmd,
			    sizeof(cmd), NULL, NULL);
	if (ret) {
		SYS_LOG_ERR("disconnect cmd fail");
		return ret;
	}

	return 0;
}

int wifi_cmd_get_cp_info(struct wifi_priv *priv)
{
	struct cmd_get_cp_info cmd;
	int ret;
	int len;

	memset(&cmd, 0, sizeof(cmd));
	ret = wifi_cmd_send(WIFI_CMD_GET_CP_INFO, (char *)&cmd, sizeof(cmd),
			    (char *)&cmd, &len);
	if (ret) {
		SYS_LOG_ERR("get cp info fail");
		return ret;
	}

	priv->cp_version = cmd.version;
	if (priv->mode == WIFI_MODE_STA) {
		memcpy(priv->mac, cmd.mac, 6);
	} else if (priv->mode == WIFI_MODE_AP) {
		cmd.mac[4] ^= 0x80;
		memcpy(priv->mac, cmd.mac, 6);
	}

	return 0;
}

int wifi_cmd_open(struct wifi_priv *priv)
{
	struct cmd_start cmd;
	int ret;

	SYS_LOG_DBG("open mode %d", priv->mode);

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = priv->mode;
	memcpy(cmd.mac, priv->mac, 6);

	ret = wifi_cmd_send(WIFI_CMD_OPEN, (char *)&cmd, sizeof(cmd),
			    NULL, NULL);
	if (ret) {
		SYS_LOG_ERR("start mode %d fail", priv->mode);
		return ret;
	}
	SYS_LOG_DBG("open mode success.");

	return 0;
}

int wifi_cmd_close(struct wifi_priv *priv)
{
	struct cmd_stop cmd;
	int ret;

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = priv->mode;
	memcpy(cmd.mac, priv->mac, 6);

	ret = wifi_cmd_send(WIFI_CMD_CLOSE, (char *)&cmd, sizeof(cmd),
			    NULL, NULL);
	if (ret) {
		SYS_LOG_ERR("stop mode:%d fail", priv->mode);
		return ret;
	}

	return 0;
}

int wifi_cmd_start_ap(struct wifi_priv *priv,
		struct wifi_start_ap_req_params *params)
{
	struct cmd_start_ap cmd;
	int ret;

	SYS_LOG_DBG("start ap at channel: %d.", params->channel);
	memset(&cmd, 0, sizeof(cmd));

	/* memcpy(cmd.mac, priv->mac, 6); */
	if (params->ssid_length > 0) {
		memcpy(cmd.ssid, params->ssid, params->ssid_length);
		cmd.ssid_len = params->ssid_length;
		SYS_LOG_DBG("ssid: %s(%d).", cmd.ssid, cmd.ssid_len);
	}
	if (params->psk_length > 0) {
		memcpy(cmd.password, params->psk, params->psk_length);
		cmd.password_len = params->psk_length;
		SYS_LOG_DBG("psk: %s(%d).", cmd.password, cmd.password_len);
	}

	cmd.channel = params->channel;
	ret = wifi_cmd_send(WIFI_CMD_START_AP, (char *)&cmd,
			    sizeof(cmd), NULL, NULL);
	if (ret) {
		SYS_LOG_ERR("ap start fail");
		return ret;
	}
	SYS_LOG_DBG("start ap ok.");

	return 0;
}

int wifi_cmd_stop_ap(struct wifi_priv *priv)
{
	struct cmd_stop cmd;
	int ret;

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = WIFI_MODE_AP;
	memcpy(cmd.mac, priv->mac, 6);
	ret = wifi_cmd_send(WIFI_CMD_CLOSE, (char *)&cmd, sizeof(cmd),
			    NULL, NULL);
	if (ret) {
		SYS_LOG_ERR("ap stop fail");
		return ret;
	}

	return 0;
}


/*
 * @param r_buf: address of return value
 * @param r_len: length of return value
 */
int wifi_cmd_npi_send(int ictx_id, char *t_buf,
		u32_t t_len, char *r_buf, u32_t *r_len)
{
	struct cmd_npi *cmd;
	int ret = -1;
	int cmd_len;

	/* Calculate total command length. */
	cmd_len = sizeof(*cmd) + t_len;

	cmd = k_malloc(cmd_len);
	if (cmd == NULL) {
		SYS_LOG_ERR("cmd is null");
		return ret;
	}

	memset(cmd, 0, cmd_len);

	memcpy(cmd->data, t_buf, t_len);

	ret = wifi_cmd_send(WIFI_CMD_NPI_MSG, (char *)cmd,
			cmd_len, r_buf, r_len);
	if (ret) {
		SYS_LOG_ERR("npi_send_command fail");
		k_free(cmd);
		return ret;
	}

	if (r_buf && r_len) {
		*r_len = *r_len - sizeof(*cmd);
		/* No need to copy trans_hdr. */
		memcpy(r_buf, r_buf + sizeof(*cmd), *r_len);
	}

	k_free(cmd);

	return 0;
}

int wifi_cmd_npi_get_mac(int ictx_id, char *buf)
{
	return wifi_get_mac((u8_t *)buf, ictx_id);
}


int wifi_cmd_set_ip(struct wifi_priv *priv, u8_t *ip_addr, u8_t len)
{
	int ret = -1;
	struct cmd_set_ip cmd;

	memset(&cmd, 0, sizeof(cmd));
	if (len == IPV4_LEN) {
		/*
		 * Temporarily supported 4-byte ipv4 address.
		 * TODO: support ipv6 address, need to reserve more bytes.
		 */
		memcpy(cmd.ip, ip_addr, len);
		/* Store ipv4 address in wifi_priv. */
		memcpy(priv->ipv4_addr, ip_addr, len);
	} else {
		SYS_LOG_WRN("Currently only ipv4, 4 bytes.");
		return ret;
	}

	ret = wifi_cmd_send(WIFI_CMD_SET_IP, (char *)&cmd,
			    sizeof(cmd), NULL, NULL);
	if (ret) {
		SYS_LOG_ERR("set ip fail");
		return ret;
	}

	SYS_LOG_DBG("set ip ok.");

	return 0;
}

int wifi_evt_scan_result(struct wifi_priv *priv, char *data, int len)
{
	struct event_scan_result *event =
		(struct event_scan_result *)data;
	struct wifi_scan_result scan_result;

	memset(&scan_result, 0, sizeof(scan_result));

	if (check_cmdevt_len(len, sizeof(struct event_scan_result))) {
		return -1;
	}

	scan_result.security = WIFI_SECURITY_TYPE_NONE;
	memcpy(scan_result.ssid, event->ssid, MAX_SSID_LEN);
	scan_result.ssid_length = strlen(scan_result.ssid);
	memcpy(scan_result.bssid, event->bssid, ETH_ALEN);

	scan_result.channel = event->channel;
	scan_result.rssi = event->rssi;

	SYS_LOG_DBG("ssid: %s", event->ssid);

	wifi_drv_iface_scan_result_cb(priv->iface, 0, &scan_result);

	k_yield();

	return 0;
}

int wifi_evt_scan_done(struct wifi_priv *priv, char *data, int len)
{
	struct event_scan_done *event =
		(struct event_scan_done *)data;

	if (check_cmdevt_len(len, sizeof(struct event_scan_done))) {
		return -1;
	}

	wifi_drv_iface_scan_done_cb(priv->iface, event->status);

	return 0;
}

int wifi_evt_connect(struct wifi_priv *priv, char *data, int len)
{
	struct event_connect *event =
		(struct event_connect *)data;

	if (check_cmdevt_len(len, sizeof(struct event_connect))) {
		return -1;
	}

	wifi_drv_iface_connect_cb(priv->iface, event->status);

	return 0;
}

int wifi_evt_disconnect(struct wifi_priv *priv, char *data, int len)
{
	struct event_disconnect *event =
		(struct event_disconnect *)data;

	if (check_cmdevt_len(len, sizeof(struct event_disconnect))) {
		return -1;
	}

	wifi_drv_iface_disconnect_cb(priv->iface,
				     event->reason_code);

	return 0;
}

int wifi_evt_new_sta(struct wifi_priv *priv, char *data, int len)
{
	struct event_new_station *event =
		(struct event_new_station *)data;

	if (check_cmdevt_len(len, sizeof(struct event_new_station))) {
		return -1;
	}

	wifi_drv_iface_new_station(priv->iface,
				   event->is_connect, event->mac);
	return 0;
}

int wifi_cmdevt_process(struct wifi_priv *priv, char *data, int len)
{
	struct trans_hdr *hdr = (struct trans_hdr *)data;

	if (len > RECV_BUF_SIZE) {
		SYS_LOG_ERR("invalid data len %d.", len);
		return -1;
	}

	/* Receive command response. */
	if (hdr->response == 1) {
		if (len > 0) {
			memcpy(recv_buf, data, len);
			recv_len = len;
		}

		k_sem_give(&cmd_sem);

		/*
		 * Release command wait semaphore, and switch current thread to
		 * command process thread. This routine could prevent the send
		 * command timeout if there are many data recived from CP.
		 */
		k_yield();

		return 0;
	}

	SYS_LOG_DBG("Receive event type 0x%x.", hdr->type);

	len -= sizeof(*hdr);

	/* Receive Events */
	switch (hdr->type) {
	case WIFI_EVENT_SCAN_RESULT:
		wifi_evt_scan_result(priv, hdr->data, len);
		break;
	case WIFI_EVENT_SCAN_DONE:
		wifi_evt_scan_done(priv, hdr->data, len);
		break;
	case WIFI_EVENT_DISCONNECT:
		wifi_evt_disconnect(priv, hdr->data, len);
		break;
	case WIFI_EVENT_CONNECT:
		wifi_evt_connect(priv, hdr->data, len);
		break;
	case WIFI_EVENT_NEW_STATION:
		wifi_evt_new_sta(priv, hdr->data, len);
		break;
	default:
		break;
	}
	return 0;
}

int wifi_cmd_send(u8_t cmd, char *data, int len, char *rbuf, int *rlen)
{
	int ret = 0;

	struct trans_hdr *hdr = (struct trans_hdr *)data;

	if (cmd > WIFI_CMD_MAX || cmd < WIFI_CMD_BEGIN) {
		SYS_LOG_ERR("Invalid command %d ", cmd);
		return -1;
	}

	if (data != NULL && len == 0) {
		SYS_LOG_ERR("data len Invalid,data=%p,len=%d", data, len);
		return -1;
	}

	hdr->len = len;
	hdr->type = cmd;
	hdr->seq = seq++;

	ret = wifi_tx_cmd(data, len);
	if (ret < 0) {
		SYS_LOG_ERR("wifi send cmd fail");
		return -1;
	}

	ret = k_sem_take(&cmd_sem, 3000);
	if (ret) {
		SYS_LOG_ERR("wait cmd(%d) timeout.", cmd);
		return ret;
	}

	hdr = (struct trans_hdr *)recv_buf;
	if (hdr->status != 0) {
		SYS_LOG_ERR("invalid cmd status: %i", hdr->status);
		return hdr->status;
	}

	if (rbuf) {
		memcpy(rbuf, recv_buf, recv_len);
	}
	if (rlen) {
		*rlen = recv_len;
	}

	SYS_LOG_DBG("get command response success");
	return 0;
}


int wifi_cmdevt_init(void)
{
	k_sem_init(&cmd_sem, 0, 1);
	return 0;
}
