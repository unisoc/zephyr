#include <zephyr.h>
#include <logging/sys_log.h>
#include <string.h>
#include <sipc.h>
#include <sblock.h>
#include <net/wifimgr_drv.h>

#include "wifi_main.h"

#define RECV_BUF_SIZE 128
static unsigned char recv_buf[RECV_BUF_SIZE];
static unsigned int recv_len;
static struct k_sem	cmd_sem;

static const u16_t CRC_table[] = {
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00,
	0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401,
	0x5000, 0x9C01, 0x8801, 0x4400,
};

static u16_t CRC16(u8_t * buf, u16_t len)
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

static struct cmd_download_ini ini;
int wifi_cmd_load_ini(u8_t * data, uint32_t len, u8_t sec_num)
{
	int ret = 0;
	u16_t CRC = 0;

	/*calc CRC value */
	CRC = CRC16(data, len);
	SYS_LOG_DBG("sec: %d, len: %d, CRC value: 0x%x",
			sec_num, len, CRC);

	memset(&ini, 0, sizeof(ini));
	ini.sec_num = sec_num;

	/*copy data after section num */
	memcpy(ini.data, data, len);
	/*put CRC value at the tail of INI data */
	memcpy((ini.data + len), &CRC, sizeof(CRC));

	ret = wifi_cmd_send(WIFI_CMD_DOWNLOAD_INI, (char *)&ini,
			sizeof(struct trans_hdr) + len + 4 + 2, NULL, 0);
			//sizeof(ini) + sizeof(CRC), NULL, 0);
	if (ret < 0) {
		SYS_LOG_ERR("load ini fail when call wifi_drv_cmd_send\n");
		return -1;
	}

	return 0;
}

int wifi_cmd_scan(struct wifi_priv *priv)
{
	int ret = 0;
	struct cmd_scan cmd;

	memset(&cmd,0,sizeof(cmd));
	cmd.channels = 0x3FFF;

	ret = wifi_cmd_send(WIFI_CMD_SCAN, (char *)&cmd,
			sizeof(cmd), NULL, NULL);
	if (ret < 0){
		SYS_LOG_ERR("scan cmd fail");
		return -1;
	}

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
	if (ret < 0){
		SYS_LOG_ERR("connect cmd fail");
		return -1;
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
	if (ret < 0){
		SYS_LOG_ERR("disconnect cmd fail");
		return -1;
	}

	return 0;
}

int wifi_cmd_get_cp_info(struct wifi_priv *priv)
{
	struct cmd_get_cp_info cmd;
	int ret = 0;
	int len;

	memset(&cmd, 0, sizeof(cmd));
	ret = wifi_cmd_send(WIFI_CMD_GET_CP_INFO, (char *)&cmd, sizeof(cmd),
			(char *)&cmd, &len);
	if (ret < 0){
		SYS_LOG_ERR("get cp info fail");
		return -1;
	}

	priv->cp_version = cmd.version;
	if (priv->mode == WIFI_MODE_STA)
		memcpy(priv->mac, cmd.mac, 6);
	else if (priv->mode == WIFI_MODE_AP) {
		cmd.mac[4] ^= 0x80;
		memcpy(priv->mac, cmd.mac, 6);
	}

	return 0;
}

int wifi_cmd_start(struct wifi_priv *priv)
{
	struct cmd_start cmd;
	int ret = 0;

	SYS_LOG_DBG("open mode %d", priv->mode);

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = priv->mode;
	memcpy(cmd.mac, priv->mac, 6);

	ret = wifi_cmd_send(WIFI_CMD_OPEN, (char *)&cmd, sizeof(cmd),
			NULL, NULL);
	if (ret < 0){
		SYS_LOG_ERR("start mode %d fail", priv->mode);
		return -1;
	}
	SYS_LOG_DBG("open mode success.");

	return 0;
}

int wifi_cmd_stop(struct wifi_priv *priv)
{
	struct cmd_stop cmd;
	int ret = 0;

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = priv->mode;
	memcpy(cmd.mac, priv->mac, 6);

	ret = wifi_cmd_send(WIFI_CMD_CLOSE, (char *)&cmd, sizeof(cmd),
			NULL, NULL);
	if (ret < 0){
		SYS_LOG_ERR("stop mode:%d fail", priv->mode);
		return -1;
	}

	return 0;
}

int wifi_cmd_start_ap(struct wifi_priv *priv, struct wifi_start_ap_req_params *params)
{
	struct cmd_start_ap cmd;
	int ret = 0;

	SYS_LOG_DBG("start ap at channel: %d.", params->channel);
	memset(&cmd, 0, sizeof(cmd));

	//memcpy(cmd.mac, priv->mac, 6);
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
	if (ret < 0){
		SYS_LOG_ERR("ap start fail");
		return -1;
	}
	SYS_LOG_DBG("start ap ok.");

	return 0;
}

int wifi_cmd_stop_ap(struct wifi_priv *priv)
{
	struct cmd_stop cmd;
	int ret = 0;

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = WIFI_MODE_AP;
	memcpy(cmd.mac, priv->mac, 6);
	ret = wifi_cmd_send(WIFI_CMD_CLOSE, (char *)&cmd, sizeof(cmd),
			NULL, NULL);
	if (ret < 0){
		SYS_LOG_ERR("ap stop fail");
		return -1;
	}

	return 0;
}


/*
 * return value is the real len sent to upper layer or -1 while error.
 */
static unsigned char npi_buf[128];
int wifi_cmd_npi_send(int ictx_id,char * t_buf,uint32_t t_len,char *r_buf,uint32_t *r_len)
{
	int ret = 0;

	/*FIXME add cmd header buffer.*/
	memcpy(npi_buf + sizeof(struct trans_hdr), t_buf, t_len);
	t_len += sizeof(struct trans_hdr);

	ret = wifi_cmd_send(WIFI_CMD_NPI_MSG, npi_buf, t_len,r_buf,r_len);
	if (ret < 0){
		SYS_LOG_ERR("npi_send_command fail");
		return -1;
	}

	if (r_buf != NULL && r_len != 0) {
		*r_len = *r_len - sizeof(struct trans_hdr);
		/* No need to copy trans_hdr */
		memcpy(r_buf, r_buf + sizeof(struct trans_hdr), *r_len);
	}

	return 0;
}

int wifi_cmd_npi_get_mac(int ictx_id,char * buf)
{
	return wifi_get_mac((u8_t *)buf,ictx_id);
}


int wifi_cmd_set_ip(struct wifi_priv *priv, u8_t *ip_addr, u8_t len)
{
	int ret = 0;
	struct cmd_set_ip cmd;

	memset(&cmd, 0, sizeof(cmd));
	if (len == IPV4_LEN) {
		/*
		 ** Temporarily supported 4-byte ipv4 address.
		 ** TODO: support ipv6 address, need to reserve more bytes.
		 */
		memcpy(cmd.ip, ip_addr, len);
		/* Store ipv4 address in wifi_priv. */
		memcpy(priv->ipv4_addr, ip_addr, len);
	} else {
		SYS_LOG_WRN("Currently only ipv4, 4 bytes.");
		return -1;
	}

	ret = wifi_cmd_send(WIFI_CMD_SET_IP, (char *)&cmd,
			sizeof(cmd), NULL, NULL);
	if (ret < 0) {
		SYS_LOG_ERR("set ip fail");
		return ret;
	}

	SYS_LOG_DBG("set ip ok.");

	return 0;
}

static struct wifi_scan_result result;
int wifi_evt_scan_result(struct wifi_priv *priv, char *data, int len)
{
	struct event_scan_result *event = 
		(struct event_scan_result *)data;

	memset(&result, 0, sizeof(result));
	
	result.security = WIFI_SECURITY_TYPE_NONE;
	memcpy(result.ssid, event->ssid, MAX_SSID_LEN);
	result.ssid_length = strlen(result.ssid);
	memcpy(result.bssid, event->bssid, ETH_ALEN);

	result.channel = event->channel;
	result.rssi = event->rssi;

	SYS_LOG_DBG("ssid: %s", event->ssid);

	if (priv->scan_cb) {
		priv->scan_cb(priv->iface, 0, &result);

		k_yield();
	}

	return 0;
}

int wifi_evt_scan_done(struct wifi_priv *priv)
{
	if (!priv->scan_cb) {
		return 0;
	}

	wifi_drv_iface_scan_done_cb(priv->iface, 0);
	priv->scan_cb = NULL;

	return 0;
}

int wifi_evt_connect(struct wifi_priv *priv, char *data, int len)
{
	struct event_connect *event =
		(struct event_connect *)data;
	wifi_drv_iface_connect_cb(priv->iface, event->status);

	return 0;
}

int wifi_evt_disconnect(struct wifi_priv *priv, char *data, int len)
{
	struct event_disconnect *event =
		(struct event_disconnect *)data;

	wifi_drv_iface_disconnect_cb(priv->iface,
			event->reason_code);


	return 0;
}

int wifi_cmdevt_process(struct wifi_priv *priv, char *data, int len)
{
	struct trans_hdr *hdr = (struct trans_hdr *)data;

	if(len > RECV_BUF_SIZE) {
		SYS_LOG_ERR("invalid data len %d.", len);
		return -1;
	}

	/* recieve command response */
	if(hdr->response == 1) {
		if(hdr->status != 0) {
			SYS_LOG_ERR("invalid cmd status: %i", hdr->status);
		}

		if (len > 0) {
			memcpy(recv_buf, data, len);
			recv_len = len;
		}

		k_sem_give(&cmd_sem);

		/*
		 * Release command wait semaphore, and switch current thread to
		 * command process thread. This rountine could prevent the send
		 * command timeout if there are many data recived from CP.
		 */
		k_yield();

		return 0;
	}

	SYS_LOG_DBG("Recieve event type 0x%x.", hdr->type);

	len -= sizeof(*hdr);

	/* Recieve Events */
	switch (hdr->type) {
		case WIFI_EVENT_SCAN_RESULT:
			wifi_evt_scan_result(priv, hdr->data, len);
			break;
		case WIFI_EVENT_SCAN_DONE:
			wifi_evt_scan_done(priv);
			break;
		case WIFI_EVENT_DISCONNECT:
			wifi_evt_disconnect(priv, hdr->data, len);
			break;
		case WIFI_EVENT_CONNECT:
			wifi_evt_connect(priv, hdr->data, len);
			break;
		default:
			break;
	}
	return 0;
}

static unsigned int seq = 1;
int wifi_cmd_send(u8_t cmd,char *data,int len,char * rbuf,int *rlen)
{
	int ret = 0;

	struct trans_hdr *hdr = (struct trans_hdr *)data;

	if (cmd > WIFI_CMD_MAX || cmd < WIFI_CMD_BEGIN){
		SYS_LOG_ERR("Invalid command %d ", cmd);
		return -1;
	}

	if (data != NULL && len ==0){
		SYS_LOG_ERR("data len Invalid,data=%p,len=%d",data,len);
		return -1;
	}

	hdr->len = len;
	hdr->type = cmd;
	hdr->seq = seq++;

	ret = wifi_tx_cmd(data, len);
	if (ret < 0){
		SYS_LOG_ERR("wifi_cmd_send fail");
		return -1;
	}

	ret = k_sem_take(&cmd_sem, 3000);
	if(ret) {
		SYS_LOG_ERR("wait cmd(%d) timeout.", cmd);
		return ret;
	}

	if(rbuf)
		memcpy(rbuf, recv_buf, recv_len);
	if(rlen)
		*rlen = recv_len;

	SYS_LOG_DBG("get command response success");
	return 0;
}


int wifi_cmdevt_init(void)
{
	k_sem_init(&cmd_sem, 0, 1);
	return 0;
}
