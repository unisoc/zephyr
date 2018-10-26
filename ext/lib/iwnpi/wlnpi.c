/*
 * Authors:<jessie.xin@spreadtrum.com>
 * Owner:
 */

#include "wlnpi.h"

#define WLNPI_WLAN0_NAME            ("wlan0")

wlnpi_t g_wlnpi = {0};

char iwnpi_exec_status_buf[IWNPI_EXEC_STATUS_BUF_LEN] = {0x00};
struct device *dev;

#define WIFI_FIRMWARE 1
extern int npi_wifi_iface_init(struct device **dev);
extern int npi_open_station(struct device *dev);
extern int npi_close_station(struct device *dev);

extern int wifi_cmd_npi_send(int ictx_id, char *t_buf, uint32_t t_len, char *r_buf, uint32_t *r_len);
extern int wifi_cmd_npi_get_mac(int ictx_id, char * buf);
extern int iwnpi_hex_dump(unsigned char *name, unsigned short nLen, unsigned char *pData, unsigned short len);

#define SUCCESS (0)
#define ARG_ERROR  (-1)
#define GET_RESULT_ERROR  (-2)
#define EXEC_ERROR (-3)
#define MALLOC_ERROR (-4)
#define ERROR (-6)
#define CMD_CAN_NOT_EXEC_ERROR (-7)
#define NOT_SUPPORT_ERROR (-8)

void show_status(int status)
{
	char *err_str = NULL;
	switch(status) {
	case SUCCESS:
		err_str = "SUCCESS";
		break;
	case ARG_ERROR:
		err_str = "ARG_ ERROR";
		break;
	case GET_RESULT_ERROR:
		err_str = "GET_RESULT_ ERROR";
		break;
	case EXEC_ERROR:
		err_str = "EXEC_ERROR";
		break;
	case MALLOC_ERROR:
		err_str = "MALLOC_ERROR";
		break;
	case CMD_CAN_NOT_EXEC_ERROR:
		err_str = "MALLOC_ERROR";
		break;
	case NOT_SUPPORT_ERROR:
		err_str = "NOT_SUPPORT_ERROR";
		break;
	default:
		err_str = "UNKNOWN_ERROR";
		break;
	}

	ENG_LOG("error ccode %d : %s \n", status, err_str);
}
int sprdwl_wlnpi_exec_status(char *buf, int len)
{
	memcpy(buf, iwnpi_exec_status_buf, len);
	ENG_LOG("iwnpi exec buf : %s\n", iwnpi_exec_status_buf);
	memset(iwnpi_exec_status_buf, 0x00, IWNPI_EXEC_STATUS_BUF_LEN);
	return 0;
}

static int sprdwl_npi_cmd_handler(wlnpi_t *wlnpi, unsigned char *s_buf, int s_len, unsigned char *r_buf, unsigned int *r_len)
{
	WLNPI_CMD_HDR_T *hdr = NULL;
	char dbgstr[64] = { 0 };
	int ictx_id = 0;
	int ret = 0;
	int recv_len = 0;

	ENG_LOG("enter %s\n", __func__);
	ictx_id = wlnpi->ictx_id;

	sprintf(dbgstr, "[iwnpi][SEND][%d]:", s_len);
	hdr = (WLNPI_CMD_HDR_T *)s_buf;
	ENG_LOG("%s type is %d, subtype %d\n", dbgstr, hdr->type, hdr->subtype);
	//iwnpi_hex_dump((unsigned char *)"s_buf:\n", strlen("s_buf:\n"), (unsigned char *)s_buf, s_len);

	ret = wifi_cmd_npi_send(ictx_id, (char *)s_buf, s_len, (char *)r_buf, r_len);
	if (ret < 0)
	{
		ENG_LOG("npi command send or recv error!\n");
		return ret;
	}

	recv_len = *r_len;
	sprintf(dbgstr, "[iwnpi][RECV][%d]:", recv_len);
	hdr = (WLNPI_CMD_HDR_T *)r_buf;
	ENG_LOG("%s type is %d, subtype %d\n", dbgstr, hdr->type, hdr->subtype);

	return ret;
}

static int enter_mode(int argc, char *argv[])
{
	int ret;

	ENG_LOG("need init wifi iface before test\n");
	ret = npi_wifi_iface_init(&dev);
	if (ret) {
		ENG_LOG("init wifi iface failed\n");
		return ret;
	}
	ENG_LOG("init wifi iface & enter mode success\n");

	/* open sta first */
	ret = npi_open_station(dev);
	if (ret) {
		ENG_LOG("open STA failed\n");
		return -1;
	}

	return ret;
}

static int exit_mode(void)
{
	int ret;
	ENG_LOG("close STA before exit mode\n");
	ret = npi_close_station(dev);
	if (ret) {
		ENG_LOG("close STA failed\n");
		return ret;
	}
	return ret;
}

static int sprdwl_npi_get_info_handler(wlnpi_t *wlnpi, unsigned char *s_buf, int s_len, unsigned char *r_buf, unsigned int *r_len)
{
	int ictx_id = 0;
	int ret = 0;

	ENG_LOG("enter %s\n", __func__);
	ictx_id = wlnpi->ictx_id;
	ret = wifi_cmd_npi_get_mac(ictx_id, (char *)r_buf);
	ENG_LOG("ret from wifi drv is %d\n", ret);

	return ret;
}

static int wlnpi_handle_special_cmd(struct wlnpi_cmd_t *cmd)
{
	ENG_LOG("ADL entry %s(), cmd = %s\n", __func__, cmd->name);

	if (0 == strcmp(cmd->name, "conn_status"))
	{
		ENG_LOG("not connected AP\n");
		ENG_LOG("ADL leval %s(), return 1\n", __func__);

		return 1;
	}

	ENG_LOG("ADL levaling %s()\n", __func__);
	return 0;
}

static int get_drv_info(wlnpi_t *wlnpi, struct wlnpi_cmd_t *cmd)
{
	int ret;
	unsigned char  s_buf[4]  ={0};
	unsigned short s_len     = 4;
	unsigned char  r_buf[32] ={0};
	unsigned int r_len     = 0;
	ENG_LOG("enter %s\n", __func__);

	memset(s_buf, 0xFF, 4);
	wlnpi->npi_cmd_id = WLAN_NL_CMD_GET_INFO;
	ret = sprdwl_npi_get_info_handler(wlnpi, s_buf, s_len, r_buf, &r_len);
	if (ret < 0)
	{
		ENG_LOG("%s iwnpi get driver info fail\n", __func__);
		return ret;
	}

	wlnpi->npi_cmd_id = WLAN_NL_CMD_NPI;
	memcpy(&(wlnpi->mac[0]), r_buf, 6 );
	if(wlnpi->mac == NULL)
	{
		if (1 == wlnpi_handle_special_cmd(cmd))
		{
			ENG_LOG("ADL levaling %s(), return -2\n", __func__);
			return -2;
		}

		ENG_LOG("get drv info err\n");
		return -1;
	}
	ENG_LOG("ADL levaling %s(), addr is %02x:%02x:%02x:%02x:%02x:%02x :end\n",
		__func__, wlnpi->mac[0], wlnpi->mac[1], wlnpi->mac[2],wlnpi->mac[3],
		wlnpi->mac[4], wlnpi->mac[5]);
	return 0;
}

extern void do_help(void);

int iwnpi_main(int argc, char **argv)
{
	int ret;
	int s_len = 0;
	unsigned int       r_len = 256;
	unsigned char        s_buf[256] = {0};
	unsigned char        r_buf[256] = {0};
	struct wlnpi_cmd_t  *cmd = NULL;
	WLNPI_CMD_HDR_T     *msg_send = NULL;
	WLNPI_CMD_HDR_R     *msg_recv = NULL;
	wlnpi_t             *wlnpi = &g_wlnpi;
	int                 status = 0;

	argc--;
	argv++;

	if (0 == strcmp(argv[0], WLNPI_WLAN0_NAME))
	{
		wlnpi->ictx_id = STA_IDX;
		/* skip "wlan0" */
		argc--;
		argv++;
	}

	if(argc < 1 || (0 == strcmp(argv[0], "help")))
	{
		do_help();
		return -1;
	}

	if(0 == strcmp(argv[0], "enter_mode"))
	{
		return enter_mode(0, NULL);
	}

	if(0 == strcmp(argv[0], "exit_mode"))
	{
		return exit_mode();
	}

	cmd = match_cmd_table(argv[0]);
	if(NULL == cmd)
	{
		ENG_LOG("[%s][not match]\n", argv[0]);
		return -1;
	}

	wlnpi->npi_cmd_id = WLAN_NL_CMD_NPI;
	
	ret = get_drv_info(wlnpi, cmd);
	if(0 != ret)
		goto EXIT;

	argc--;
	argv++;
	if(NULL == cmd->parse)
	{
		ENG_LOG("func null\n");
		ret = -1;
		goto EXIT;
	}

	ret = cmd->parse(argc, argv, s_buf + sizeof(WLNPI_CMD_HDR_T), &s_len);
	if(0 != ret)
	{
		ENG_LOG("%s\n", cmd->help);
		goto EXIT;
	}
	msg_send = (WLNPI_CMD_HDR_T *)s_buf;
	msg_send->type = HOST_TO_MARLIN_CMD;
	msg_send->subtype = cmd->id;
	msg_send->len = s_len;
	s_len = s_len + sizeof(WLNPI_CMD_HDR_T);

	ret = sprdwl_npi_cmd_handler(wlnpi, s_buf, s_len, r_buf, &r_len);
	if (ret < 0)
	{
		ENG_LOG("%s iwnpi cmd send or recv error\n", __func__);
		return ret;
	}

	msg_recv = (WLNPI_CMD_HDR_R *)r_buf;

	ENG_LOG("msg_recv->type = %d, cmd->id = %d, subtype = %d, r_len = %d\n",
		msg_recv->type, cmd->id, msg_recv->subtype, r_len);
	//iwnpi_hex_dump((unsigned char *)"r_buf:\n", strlen("r_buf:\n"), (unsigned char *)r_buf, r_len);
	if( (MARLIN_TO_HOST_REPLY != msg_recv->type) || (cmd->id != msg_recv->subtype) || r_len < sizeof(WLNPI_CMD_HDR_R) )
	{
		ENG_LOG("communication error\n");
		ENG_LOG("msg_recv->type = %d, cmd->id = %d, subtype = %d, r_len = %d\n",
			msg_recv->type, cmd->id, msg_recv->subtype, r_len);
		goto EXIT;
	}

	status = msg_recv->status;
	if(-100 == status)
	{
		ENG_LOG("marlin not realize\n");
		goto EXIT;
	} else {
		show_status(status);
	}
	snprintf(iwnpi_exec_status_buf, IWNPI_EXEC_STATUS_BUF_LEN, "ret: status %d :end", status);
	ENG_LOG("ret: status %d :end\n", status);
	ENG_LOG("%s\n", iwnpi_exec_status_buf);
	cmd->show(cmd, r_buf + sizeof(WLNPI_CMD_HDR_R), r_len - sizeof(WLNPI_CMD_HDR_R));

EXIT:
	return 0;
}
