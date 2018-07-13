#include <zephyr.h>
#include <logging/sys_log.h>
#include <string.h>
#include <sipc.h>
#include <sblock.h>

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


#if 0
int wifi_event_sta_connecting_handle(u8_t *pAd,struct event_sta_connecting_event *event)
{

	if (pAd == NULL || event == NULL){
		SYS_LOG_ERR("invalid parameter");
		return -1;
	}
	if (event->trans_header.type != EVENT_STA_CONNECTING_EVENT){
		SYS_LOG_ERR("event type is not EVENT_STA_CONNECTING_EVENT,we can do nothing");
		return -1;
	}

	switch (event->type){
		case EVENT_STA_AP_CONNECTED_SUCC:
			SYS_LOG_ERR("sta connected to ap(%s) success,start dhcpc now",pAd->sta_config.ssid);
			netif_set_link_up(&pAd->sta_ifnet);
			dhcp_start(&pAd->sta_ifnet);
			break;
		case EVENT_STA_AP_CONNECTED_FAIL:
			if (event->errcode == EVENT_STA_CONNECT_ERRCODE_PW_ERR)
				SYS_LOG_ERR("sta connect to ap(%s)fail,password error",pAd->sta_config.ssid);
			else if (event->errcode == EVENT_STA_CONNECT_ERRCODE_AP_OF_OUT_RANGE)
				SYS_LOG_ERR("sta connect to ap(%s)fail,ap is out of range",pAd->sta_config.ssid);
			break;
		case EVENT_STA_AP_CONNECTED_LOST:
			SYS_LOG_ERR("sta lost connection with ap(%s)",pAd->sta_config.ssid);
			break;
		default:
			SYS_LOG_ERR("unknow event->type=%d",event->type);
	}

	return 0;
}
int wifi_cmd_set_sta_connect_info(u8_t *pAd,char *ssid,char *key)
{
	struct cmd_sta_set_rootap_info data;
	int ret = 0;
	if (pAd == NULL ||ssid == NULL)
	{
		SYS_LOG_ERR("Invalid parameter,pAd=%p,ssid=%p",pAd,ssid);
		return -1;
	}

	memset(&data,0,sizeof(data));
	memcpy(data.rootap.ssid,ssid,(strlen(ssid)< 32?strlen(ssid):32));
	if (strlen(key) <sizeof(data.rootap.key))
		memcpy(data.rootap.key,key,strlen(key));
	ret = wifi_cmd_send(CMD_STA_SET_ROOTAPINFO,
			(char *)&data,sizeof(data),NULL,NULL);
	if (ret < 0){
		SYS_LOG_ERR("something wrong when set connection info");
		return -1;
	}

	return 0;
}
#endif

#if LWIP_IGMP
int wifi_cmd_add_mmac(u8_t *pAd,char *mac)
{
	struct cmd_sta_set_igmp_mac data;
	int ret = 0;

	memset(&data,0,sizeof(data));
	data.igmp_mac_set.action = IGMP_MAC_ACTION_ADD;
	memcpy(data.igmp_mac_set.mac,mac,6);
	ret = wifi_cmd_send(CMD_STA_SET_IGMP_MAC,(char *)&data,sizeof(data), NULL, NULL);
	if (ret < 0){
		SYS_LOG_ERR("set igmp add filter fail");
		return -1;
	}else{
		ninfo("set igmp add filter success");
		return 0;
	}
}
int wifi_cmd_del_mmac(u8_t *pAd,char *mac)
{
	struct cmd_sta_set_igmp_mac data;
	int ret = 0;

	memset(&data,0,sizeof(data));
	data.igmp_mac_set.action = IGMP_MAC_ACTION_REMOVE;
	memcpy(data.igmp_mac_set.mac,mac,6);
	ret = wifi_cmd_send(CMD_STA_SET_IGMP_MAC,(char *)&data,sizeof(data), NULL, NULL);
	if (ret < 0){
		SYS_LOG_ERR("set igmp delete filter fail");
		return -1;
	}else {
		ninfo("set igmp delete filter success");
		return 0;
	}
}
#endif

int wifi_cmd_start_ap(u8_t *pAd)
{
	struct wifi_cmd_start data;
	int ret = 0;

	memset(&data,0,sizeof(data));
	data.mode = WIFI_MODE_AP;
	wifi_get_mac((u8_t *)data.mac,1);
	ret = wifi_cmd_send(WIFI_CMD_OPEN,(char *)&data,sizeof(data),NULL,NULL);
	if (ret < 0){
		SYS_LOG_ERR("ap start fail");
		return -1;
	}

	return 0;
}

static struct wifi_cmd_download_ini ini;
int wifi_cmd_load_ini(u8_t * data, uint32_t len, u8_t sec_num)
{
	int ret = 0;
	u16_t CRC = 0;

	/*calc CRC value */
	CRC = CRC16(data, len);
	SYS_LOG_INF("sec: %d, len: %d, CRC value: 0x%x",
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

#define TEST_SSID	"test_ssid"
void result_test(struct wifi_priv *priv)
{
	static struct wifi_scan_result result;
	memset(&result, 0, sizeof(result));

	result.security = WIFI_SECURITY_TYPE_NONE;
	memcpy(result.ssid, TEST_SSID, MAX_SSID_LEN);
	result.ssid_length = strlen(result.ssid);
	result.channel = 1;
	result.rssi = 0xb8;

	if (priv->scan_cb) {
		priv->scan_cb(priv->iface, 0, &result);
	}
}

int wifi_cmd_scan(struct wifi_priv *priv)
{
	int ret = 0;
	struct wifi_cmd_scan cmd;

	memset(&cmd,0,sizeof(cmd));
	cmd.channels = 0x1;

	ret = wifi_cmd_send(WIFI_CMD_SCAN, (char *)&cmd,
			sizeof(cmd), NULL, NULL);
	if (ret < 0){
		SYS_LOG_ERR("scan cmd fail");
		return -1;
	}
	
	result_test(priv);

	return 0;
}

int wifi_cmd_connect(struct wifi_priv *priv, 
		struct wifi_connect_req_params *params)
{
	int ret;
	struct wifi_cmd_connect cmd;

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
	struct wifi_cmd_disconnect cmd;

	cmd.reason_code = 0;

	ret = wifi_cmd_send(WIFI_CMD_DISCONNECT, (char *)&cmd,
			sizeof(cmd), NULL, NULL);
	if (ret < 0){
		SYS_LOG_ERR("disconnect cmd fail");
		return -1;
	}

	return 0;
}

int wifi_cmd_start_sta(struct wifi_priv *priv)
{
	struct wifi_cmd_start cmd;
	int ret = 0;

	memset(&cmd, 0, sizeof(cmd));
	cmd.mode = WIFI_MODE_STA;
	wifi_get_mac((u8_t *)cmd.mac, 0);
	ret = wifi_cmd_send(WIFI_CMD_OPEN, (char *)&cmd, sizeof(cmd),
			NULL, NULL);
	if (ret < 0){
		SYS_LOG_ERR("sta start fail");
		return -1;
	}

	return 0;
}

int wifi_cmd_start_apsta(u8_t *pAd)
{
	struct wifi_cmd_start data;
	int ret = 0;

	memset(&data,0,sizeof(data));
	data.mode= WIFI_MODE_APSTA;
	wifi_get_mac((u8_t *)data.mac,1);
	ret = wifi_cmd_send(WIFI_CMD_OPEN,(char *)&data,sizeof(data),NULL,NULL);
	if (ret < 0){
		SYS_LOG_ERR("apsta start fail");
		return -1;
	}

	return 0;
}

#if 0
int wifi_cmd_config_sta(u8_t *pAd,struct apinfo *config)
{
	int ret = 0;
	struct cmd_sta_set_rootap_info data;

	memset(&data,0,sizeof(data));
	memcpy(&data.rootap, config, sizeof(*config));
	ret = wifi_cmd_send(CMD_STA_SET_ROOTAPINFO,(char *)&data,sizeof(data),NULL,NULL);
	if (ret < 0){
		SYS_LOG_ERR("config sta fail");
		return -1;
	}

	return 0;
}

int wifi_cmd_config_ap(u8_t *pAd,struct apinfo *config)
{
	int ret = 0;
	struct cmd_ap_set_ap_info data;

	memset(&data,0,sizeof(data));
	memcpy(&data.apinfo, config, sizeof(*config));
	ret = wifi_cmd_send(CMD_AP_SET_APINFO,(char *)&data,sizeof(data),NULL,NULL);
	if (ret < 0){
		SYS_LOG_ERR("config ap fail");
		return -1;
	}

	return 0;
}
#endif

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

static struct wifi_scan_result result;
int wifi_evt_scan_result(struct wifi_priv *priv, char *data, int len)
{
	struct wifi_event_scan_result *event = 
		(struct wifi_event_scan_result *)data;

	memset(&result, 0, sizeof(result));
	
	result.security = WIFI_SECURITY_TYPE_NONE;
	memcpy(result.ssid, event->ssid, MAX_SSID_LEN);
	result.ssid_length = strlen(result.ssid);

	result.channel = event->channel;
	result.rssi = event->rssi;

	SYS_LOG_INF("ssid: %s", event->ssid);

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

	priv->scan_cb(priv->iface, 0, 0);
	priv->scan_cb = NULL;

	return 0;
}

int wifi_evt_connect(struct wifi_priv *priv, char *data, int len)
{
	struct wifi_event_connect *event = 
		(struct wifi_event_connect *)data;

	wifi_mgmt_raise_connect_result_event(priv->iface, event->status);

	return 0;
}

int wifi_evt_disconnect(struct wifi_priv *priv, char *data, int len)
{
	struct wifi_event_disconnect *event = 
		(struct wifi_event_disconnect *)data;

	wifi_mgmt_raise_connect_result_event(priv->iface,
			event->reason_code ? -EIO: 0);

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

	SYS_LOG_INF("Recieve event type 0x%x.", hdr->type);

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
			SYS_LOG_ERR("disconnected");
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
		SYS_LOG_ERR("wait cmd(%d) timeout.\n", cmd);
		return ret;
	}

	if(rbuf)
		memcpy(rbuf, recv_buf, recv_len);
	if(rlen)
		*rlen = recv_len;

	SYS_LOG_INF("get command response success");
	return 0;
}


int wifi_cmdevt_init(void)
{
	k_sem_init(&cmd_sem, 0, 1);
	return 0;
}
