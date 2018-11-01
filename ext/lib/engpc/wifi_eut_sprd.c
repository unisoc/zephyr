#include "wifi_eut_sprd.h"
#include "eut_opt.h"
#include "eng_sscanf.h"

#define CONFIG_SYSTEM_IWNPI
#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))
#define wifi_eut_debug
#define WIFI_DRIVER_MODULE_PATH "/lib/modules/sprdwl_ng.ko"

extern int iwnpi_main(int argc, char *argv[]);
extern int sprdwl_wlnpi_exec_status(char *buf, int len);
extern int sprdwl_iwnpi_ret_buf(char *buf, int len);

#define rsp_debug(rsp_)                               \
	do {                                                \
		ENG_LOG("%s(), rsp###:%s\n", __FUNCTION__, rsp_); \
	} while (0);

#define wifi_status_dump()                                             \
	do {                                                                 \
		ENG_LOG("%s(), eut_enter = %d, tx_start = %d, rx_start = %d\n",    \
				__FUNCTION__, g_wifi_data.eut_enter, g_wifi_data.tx_start, \
				g_wifi_data.rx_start);                                     \
	} while (0);

unsigned int g_wifi_tx_count = 0;

WIFI_ELEMENT g_wifi_data;

#define INIT_CDEC_VAL	100
unsigned char g_afc_cdec = INIT_CDEC_VAL;

struct tpc_para
{
	unsigned char channel;
	unsigned char chain;
	unsigned char power;
};
struct tpc_para g_tpc_para[12];

enum tpc_status
{
	TPC_DEFAULT = 1,
	TPC_NEED_SET_PARA = 2,
};

unsigned char g_tpc_status = TPC_DEFAULT;
char g_tpc_num = -1;

int  get_wcn_afc_cdec()
{
	return 	g_afc_cdec;
}
/********************************************************************
 *   name   get_wcn_hw_type
 *   ---------------------------
 *   description: get wcn hardware type
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   none
 *   ----------------------------------------------------
 *   return
 *   a enum type digit portray wcn hardware chip
 *   ------------------
 *   other:
 ********************************************************************/
static int get_wcn_hw_type(void)
{
	g_wifi_data.wcn_type = WCN_HW_MARLIN3;
	return g_wifi_data.wcn_type;
}

// new rate table for marlin3, it will be used for marlin2
static WIFI_RATE g_wifi_rate_table[] = {
 	{0, "DSSS-1"},    /*1M_Long*/      {1, "DSSS-2"},    /*2M_Long*/      {2, "DSSS-2S"},   /*2M_Short*/
	{3, "CCK-5.5"},   /*5.5M_Long*/    {4, "CCK-5.5S"},  /*5.5M_Short*/   {5, "CCK-11"},    /*11M_Long*/
	{6, "CCK-11S"},   /*11M_Short*/    {7, "OFDM-6"},    /*6M*/           {8, "OFDM-9"},    /*9M*/
	{9, "OFDM-12"},   /*12M*/          {10, "OFDM-18"},  /*18M*/          {11, "OFDM-24"},  /*24M*/
	{12, "OFDM-36"},  /*36M*/          {13, "OFDM-48"},  /*48M*/          {14, "OFDM-54"},  /*54M*/
	{15, "MCS-0"},    /*HT_MCS0*/      {16, "MCS-1"},    /*HT_MCS1*/      {17, "MCS-2"},    /*HT_MCS2*/
	{18, "MCS-3"},    /*HT_MCS3*/      {19, "MCS-4"},    /*HT_MCS4*/      {20, "MCS-5"},    /*HT_MCS5*/
	{21, "MCS-6"},    /*HT_MCS6*/      {22, "MCS-7"},    /*HT_MCS7*/      {23, "MCS-8"},    /*HT_MCS8*/
	{24, "MCS-9"},    /*HT_MCS9*/      {25, "MCS-10"},   /*HT_MCS10*/     {26, "MCS-11"},   /*HT_MCS11*/
	{27, "MCS-12"},   /*HT_MCS12*/     {28, "MCS-13"},   /*HT_MCS13*/     {29, "MCS-14"},   /*HT_MCS14*/
	{30, "MCS-15"},   /*HT_MCS15*/     {31, "MCS0_1SS"}, /*VHT_MCS0_1SS*/ {32, "MCS1_1SS"}, /*VHT_MCS1_1SS*/
	{33, "MCS2_1SS"}, /*VHT_MCS2_1SS*/ {34, "MCS3_1SS"}, /*VHT_MCS3_1SS*/ {35, "MCS4_1SS"}, /*VHT_MCS4_1SS*/
	{36, "MCS5_1SS"}, /*VHT_MCS5_1SS*/ {37, "MCS6_1SS"}, /*VHT_MCS6_1SS*/ {38, "MCS7_1SS"}, /*VHT_MCS7_1SS*/
	{39, "MCS8_1SS"}, /*VHT_MCS8_1SS*/ {40, "MCS9_1SS"}, /*VHT_MCS9_1SS*/ {41, "MCS0_2SS"}, /*VHT_MCS0_2SS*/
	{42, "MCS1_2SS"}, /*VHT_MCS1_2SS*/ {43, "MCS2_2SS"}, /*VHT_MCS2_2SS*/ {44, "MCS3_2SS"}, /*VHT_MCS3_2SS*/
	{45, "MCS4_2SS"}, /*VHT_MCS4_2SS*/ {46, "MCS5_2SS"}, /*VHT_MCS5_2SS*/ {47, "MCS6_2SS"}, /*VHT_MCS6_2SS*/
	{48, "MCS7_2SS"}, /*VHT_MCS7_2SS*/ {49, "MCS8_2SS"}, /*VHT_MCS8_2SS*/
	{50, "MCS9_2SS"}, /*VHT_MCS9_2SS*/ {-1, "null"}
};

static int mattch_rate_table_str(const char *name) {
	WIFI_RATE *rate_ptr = g_wifi_rate_table;

	while (rate_ptr->rate != -1) {
		if (strlen(name) - 1 == strlen(rate_ptr->name) && NULL != strstr(name, rate_ptr->name)) {
			return rate_ptr->rate;
		}
		rate_ptr++;
	}
	return -1;
}

static char *mattch_rate_table_index(const int rate) {
	WIFI_RATE *rate_ptr = g_wifi_rate_table;

	while (rate_ptr->rate != -1) {
		if (rate == rate_ptr->rate) {
			return rate_ptr->name;
		}
		rate_ptr++;
	}
	return NULL;
}

/********************************************************************
 *   name   get_iwnpi_ret_line
 *   ---------------------------
 *   description: get string result from iwnpi when after Chip's command
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   outstr      OUT         const char *    store string remove "ret: " and ":end"
 *   out_len     IN          const int       the max length of out_str
 *   flag        INT         const char *    find string included this flag,
 *                                           if flag = null, only get one data
 *   ----------------------------------------------------
 *   return
 *   if flag=null, return one digit from ret: %d:end
 *   if flag!=null, return value must > 0, stand for the length of string saved in *out_str
 *   ------------------
 *   other:
 ********************************************************************/
static int get_iwnpi_ret_line(const char *flag, const char *out_str, const int out_len) {
	//we can call iwnpi interface to get return value, no need operate file.
	char *str1 = NULL;
	char *str2 = NULL;
	int ret = WIFI_INT_INVALID_RET;
	unsigned long len = 0;
	char buf[TMP_BUF_SIZE] = {0};

	if (NULL != flag && NULL == out_str) {
		ENG_LOG("%s(), flag(%s) out str is NULL, return -100.\n", __FUNCTION__, flag);
		return WIFI_INT_INVALID_RET;
	}

#ifdef CONFIG_SYSTEM_IWNPI
	if(NULL != flag) {
		if (strcmp(flag, IWNPI_RET_STA_FLAG) == 0)
			sprdwl_wlnpi_exec_status(buf, TMP_BUF_SIZE);
		else
			sprdwl_iwnpi_ret_buf(buf, TMP_BUF_SIZE);
	} else {
		sprdwl_iwnpi_ret_buf(buf, TMP_BUF_SIZE);
}
#endif
	ENG_LOG("%s(), buf = %s\n", __FUNCTION__, buf);

	if (NULL == flag) {   //only for return one data, format is "ret: %d:end"
		eng_sscanf(buf, IWNPI_RET_ONE_DATA, &ret);
	} else if (NULL != flag && NULL != strstr(buf, flag)) {
		str1 = strstr(buf, IWNPI_RET_BEGIN);
		str2 = strstr(buf, IWNPI_RET_END);
		if ((NULL != str1) && (NULL != str2)) {
			len = (unsigned long)str2 - (unsigned long)str1 - strlen(IWNPI_RET_BEGIN);
			if (len > 0) {
				len = out_len > len ? len : out_len;
				memset(out_str, 0x00, out_len);
				memcpy(out_str, str1 + strlen(IWNPI_RET_BEGIN), len);
				ret = len;
			}
		}
	}

	memset(buf, 0x00, TMP_BUF_SIZE);
	ENG_LOG("leaving %s(), out_str = %s, ret = %d\n", __FUNCTION__, out_str, ret);
	return ret;
}

/********************************************************************
 *   name   get_iwnpi_exec_status
 *   ---------------------------
 *   description: get iwnpi status
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   void
 *   ----------------------------------------------------
 *   return
 *   0: successfully
 *   !0: failed
 *   ------------------
 *   other:
 ********************************************************************/
static int get_iwnpi_exec_status(void)
{

	char buf[TMP_BUF_SIZE] = {0};
	int sta = -1;

	ENG_LOG("start to get exec status\n");
	if (get_iwnpi_ret_line(IWNPI_RET_STA_FLAG, buf, TMP_BUF_SIZE) > 0)
		eng_sscanf(buf, "status %d", &sta);
	ENG_LOG("leaving %s(), get status = %d", __FUNCTION__, sta);
	return sta;
}

/********************************************************************
 *   name   get_result_from_iwnpi
 *   ---------------------------
 *   description: get int type result of Chip's command
 *   ----------------------------
 *   para        IN/OUT      type            note
 *
 *   ----------------------------------------------------
 *   return
 *   -100:error
 *   other:return value
 *   ------------------
 *   other:
 ********************************************************************/
static inline int get_result_from_iwnpi(void) {
	return get_iwnpi_ret_line(NULL, NULL, 0);
}

static inline int wifi_enter_eut_mode(void) {
	ENG_LOG("%s eut_enter : %d\n",__FUNCTION__, g_wifi_data.eut_enter);
	return g_wifi_data.eut_enter;
}

/********************************************************************
 *   name   exec_cmd
 *   --------------------------------------------------
 *   description: send cmd to iwnpi
 *   --------------------------------------------------
 *   para        IN/OUT      type            note
 *   cmd         IN          const char *    execute cmd
 *   ----------------------------------------------------
 *   return
 *   0: successfully
 *   other: failed
 *   ---------------------------------------------------
 *   other:
 ********************************************************************/
static inline int exec_cmd(const char *cmd)
{
	ENG_LOG("%s : %s\n", __func__, cmd);
	return 0;
}

/********************************************************************
 *   name   exec_iwnpi_cmd
 *   --------------------------------------------------
 *   description: send cmd to iwnpi
 *   --------------------------------------------------
 *   para        IN/OUT      type            note
 *   cmd         IN          const char *    execute cmd
 *   ----------------------------------------------------
 *   return
 *   0: successfully
 *   other: failed
 *   ---------------------------------------------------
 *   other:
 ********************************************************************/
static inline int exec_iwnpi_cmd(int argc, char **argv)
{
	int index = 0;
	for(index = 0; index < argc; index++) {
		ENG_LOG("argv[%d] : %s \n", index, argv[index]);
	}

#ifdef CONFIG_SYSTEM_IWNPI
	return iwnpi_main(argc, argv);
#endif
}

/********************************************************************
 *   name   exec_iwnpi_cmd_for_status
 *   --------------------------------------------------
 *   description: send cmd to iwnpi, and return status
 *   --------------------------------------------------
 *   para        IN/OUT      type            note
 *   cmd         IN          const char *    execute cmd
 *   ----------------------------------------------------
 *   return
 *   0: successfully
 *   other: failed
 *   ---------------------------------------------------
 *   other:
 ********************************************************************/
static inline int exec_iwnpi_cmd_for_status(const char *cmd) {
	int ret = -1;
	return ((ret = exec_cmd(cmd)) == 0 ? get_iwnpi_exec_status() : ret);
}

/********************************************************************
 *   name   exec_iwnpi_cmd_for_status
 *   --------------------------------------------------
 *   description: send cmd to iwnpi, and return status
 *   --------------------------------------------------
 *   para        IN/OUT      type            note
 *   cmd         IN          const char *    execute cmd
 *   ----------------------------------------------------
 *   return
 *   0: successfully
 *   other: failed
 *   ---------------------------------------------------
 *   other: this function add for marlin3 module
 ********************************************************************/
static inline int exec_iwnpi_cmd_with_status(int argc, char **argv)
{
	int ret = -1;
	return ((ret = exec_iwnpi_cmd(argc, argv)) == 0 ? get_iwnpi_exec_status() : ret);
}

static inline int return_ok(const char *rsp) {
	strcpy(rsp, EUT_WIFI_OK);
	rsp_debug(rsp);
	return 0;
}

static inline int return_err(const char *rsp) {
	sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
	rsp_debug(rsp);
	return -1;
}

static int wifi_npi_start(char *rsp)
{
	int argc = 3;
	//just test iwnpi wlan0 start command

	char* cmd[] = {"iwnpi","wlan0", "start"};

	ENG_LOG("just call npi start\n");
	if (0 != exec_iwnpi_cmd_with_status(argc, cmd)) {
		ENG_LOG("exec iwnpi wlan0 start error");
		return -1;
	}
	return 0;
}

static int wifi_set_cbank_after_start(unsigned char cbank)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[argc];

	ENG_LOG("%s(), cbank:%d\n", __FUNCTION__, cbank);

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_cbank_reg";
	sprintf(cmd, "%d", cbank);
	argv[3] = cmd;
	return exec_iwnpi_cmd_with_status(argc, argv);
}

static int set_tpc_channel(unsigned char channel)
{
	char ch_1[8] = {0};
	char ch_2[8] = {0};
	int argc = 5;
	char *argv[argc];

	ENG_LOG("%s(), channel:%d\n", __func__, channel);

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_channel";
	sprintf(ch_1, "%d", channel);
	sprintf(ch_2, "%d", channel);
	argv[3] = ch_1;
	argv[4] = ch_2;
	return exec_iwnpi_cmd_with_status(argc, argv);
}

static int set_tpc_chain(unsigned char chain)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[argc];

	ENG_LOG("%s(), chain:%d\n", __func__, chain);

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_chain";
	sprintf(cmd, "%d", chain);
	argv[3] = cmd;
	return exec_iwnpi_cmd_with_status(argc, argv);
}

static int set_tpc_power(unsigned char power)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[argc];

	ENG_LOG("%s(), power:%d\n", __func__, power);

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_cal_txpower";
	sprintf(cmd, "%d", power);
	argv[3] = cmd;
	return exec_iwnpi_cmd_with_status(argc, argv);
}

int wifi_restore_tpc_result(void)
{
	/*restore tpc para*/
	int index;
	int ret = 0;

	for(index = 0; index < g_tpc_num; index++) {
		/*set channel*/
		ret = set_tpc_channel(g_tpc_para[index].channel);
		if (ret) {
			ENG_LOG("%s() set tpc channel err, ret : %d\n", __func__, ret);
			return ret;
		}
		/*set chain*/
		set_tpc_chain(g_tpc_para[index].chain);
		if (ret) {
			ENG_LOG("%s() set tpc channel err, ret : %d\n", __func__, ret);
			return ret;
		}
		/*set power*/
		set_tpc_power(g_tpc_para[index].power);
		if (ret) {
			ENG_LOG("%s() set tpc channel err, ret : %d\n", __func__, ret);
			return ret;
		}
	}
	return ret;
}

static int start_wifieut(char *rsp)
{
	int ret;
	int argc = 3;
	char *argv[3] = {"iwnpi", "wlan0", "enter_mode"};

	if (0 != exec_iwnpi_cmd(argc, argv)) {
		ENG_LOG("%s() enter mode fail\n", __func__);
		goto err;
	}

	ret = wifi_npi_start(rsp);
	if (ret)
		goto err;
	if (g_afc_cdec == INIT_CDEC_VAL) {
		ENG_LOG("%s() no need set cbank to cp\n", __func__);
	}

	if ((g_afc_cdec < 64) && (g_afc_cdec >= 0)) {
		ENG_LOG("%s() need set cbank to cp , g_afc_cdec value:%d \n", __func__, g_afc_cdec);
		if ( 0 != wifi_set_cbank_after_start(g_afc_cdec)) {
			ENG_LOG("%s() set cbank error\n", __func__);
			goto err;
		}
	}

	if (g_tpc_status == TPC_DEFAULT) {
		ENG_LOG("%s() no need set tpc para to cp\n", __func__);
	} else {
		ENG_LOG("%s() need set tpc para\n", __func__);
		if (wifi_restore_tpc_result()) {
			ENG_LOG("%s() set tpc para error\n", __func__);
			goto err;
		}
	}

	memset(&g_wifi_data, 0x00, sizeof(WIFI_ELEMENT));
	ENG_LOG("%s() start to set eut_enter status\n", __func__);
	g_wifi_data.eut_enter = 1;
	return return_ok(rsp);

err:
	ENG_LOG("%s() start to set eut_enter to zero\n", __func__);
	g_wifi_data.eut_enter = 0;
	return return_err(rsp);
}

static int stop_wifieut(char *rsp)
{
	int ret;
	int argc = 3;
	char *argv[3] = {"iwnpi", "wlan0"};

	ENG_LOG("ADL entry %s(), line = %d", __func__, __LINE__);

	wifi_status_dump();
	if (1 == g_wifi_data.tx_start) {
		argv[2] = "tx_stop";
		if (0 != exec_iwnpi_cmd(argc, argv))
			goto err;
	}

	if (1 == g_wifi_data.rx_start) {
		argv[2] = "rx_stop";
		if (0 != exec_iwnpi_cmd(argc, argv))
			goto err;
	}

	argv[2] = "stop";
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	memset(&g_wifi_data, 0, sizeof(WIFI_ELEMENT));
	ENG_LOG("%s() exit eut, we will exit mode\n", __func__);
	argv[2] = "exit_mode";
	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	return return_ok(rsp);

err:
	return return_err(rsp);
}

int wifi_eut_set(int cmd, char *rsp)
{
	ENG_LOG("entry %s(), cmd = %d\n", __FUNCTION__, cmd);
	if (cmd == 1) {
		start_wifieut(rsp);
	} else if (cmd == 0) {
		stop_wifieut(rsp);
	} else {
		ENG_LOG("%s(), cmd don't support", __func__);
		sprintf(rsp, "%s%s", EUT_WIFI_ERROR, "error");
	}
	return 0;
}

int parse_para(char *cmd, int index)
{
	char *tmp, *pos;
	pos = cmd;
	unsigned channel, chain, power;
	char chan_str[5] = {0x00};
	char chain_str[5] = {0x00};
	char power_str[5] = {0x00};

	/* get channel */
	pos = cmd;
	tmp = strchr(pos, ',');
	if (!tmp) {
		printf("failed to get channel : %s\n", cmd);
		return -1;
	}

	memcpy(chan_str, pos, tmp -pos);
	channel =  atoi(chan_str);
	ENG_LOG ("the channel is : %d\n",channel);

	/* get chain */
	pos = tmp + 1;
	tmp = strchr(pos, ',');
	if (!tmp) {
		ENG_LOG("%s() failed to get chain: %s\n", __func__, cmd);
		return -1;
	}
	memcpy(chain_str, pos, tmp -pos);
	chain =  atoi(chain_str);
	ENG_LOG ("the chain is : %d\n",chain);

	/*get power*/
	pos = tmp + 1;
	memcpy(power_str, pos, strlen(pos));
	power =  atoi(power_str);
	ENG_LOG ("the power is : %d\n", power);

	g_tpc_para[index].channel = channel;
	g_tpc_para[index].chain = chain;
	g_tpc_para[index].power = power;
	return 0;
}

int save_tpc_para(char *buf,  int len)
{
	unsigned char num = 0;
	int index = 0, result = 0;
	char *pos;
	char cmd[20] = {0x00};

	pos = buf;
	for(index = 0; index < len; index++) {
		if (buf[index] == ';') {
			memset(cmd, 0x00, 20);
			result += 1;
			memcpy(cmd, pos, num);
			ENG_LOG("cmd is : %s\n", cmd);
			parse_para(cmd, result - 1);
			pos += num;
			pos += 1;
			/*reset num*/
			num = 0;
		} else {
			num += 1;
		}
	}

	g_tpc_num = result;
	return 0;
}

int wifi_set_tpc_para(char *buf, char *rsp)
{
	char *value;

	value = strchr(buf, ':');
	if (value == NULL) {
		ENG_LOG("%s() Invalid para received\n", __func__);
	} else {
		value += 1;
		ENG_LOG("%s() value is : %s, len is : %d\n", __func__, value, strlen(value));
		memset(g_tpc_para, 0x00, sizeof(g_tpc_para));
		save_tpc_para(value, strlen(value));
	}

	if (g_tpc_status == TPC_DEFAULT) {
		g_tpc_status = TPC_NEED_SET_PARA;
	}

	return return_ok(rsp);
}

int wifi_eut_get(char *rsp)
{
	wifi_enter_eut_mode();
	sprintf(rsp, "%s%d", EUT_WIFI_REQ, g_wifi_data.eut_enter);
	rsp_debug(rsp);
	return 0;
}

int wifi_rate_set(char *string, char *rsp)
{
	int rate = -1;
	int argc = 4;
	char cmd[8] = {0x00};
	char *argv[4];

	if (wifi_enter_eut_mode() == 0)
		goto err;
	rate = mattch_rate_table_str(string);
	ENG_LOG("%s(), %s rate:%d\n", __FUNCTION__, string, rate);
	if (-1 == rate) goto err;

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_rate";
	sprintf(cmd, "%d", rate);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	g_wifi_data.rate = rate;
	return return_ok(rsp);

err:
	return return_err(rsp);
}

int wifi_rate_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	char *str = NULL;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_rate"};

	ENG_LOG("%s()...\n", __FUNCTION__);
	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == ret) {
		ENG_LOG("iwnpi wlan0 get_rate err_code:%d", ret);
		goto err;
	}
	g_wifi_data.rate = ret;
	str = mattch_rate_table_index(g_wifi_data.rate);
	if (NULL == str) {
		ENG_LOG("%s(), don't mattch rate", __FUNCTION__);
		goto err;
	}

	sprintf(rsp, "%s%s", WIFI_RATE_REQ_RET, str);
	rsp_debug(rsp);
	return 0;

err:
	sprintf(rsp, "%s%s", WIFI_RATE_REQ_RET, "null");
	rsp_debug(rsp);
	return -1;
}

//marlin3: iwnpi wlan0 set_channel $(prim_ch) $(cen_ch) > $(TMP_FILE)
int wifi_channel_set_marlin3(char *ch_note1, char *ch_note2, char *rsp)
{
	int argc = 5;
	char *argv[argc];

	if (wifi_enter_eut_mode() == 0)
		goto err;

	int primary_channel = atoi(ch_note1);
	int center_channel = 0;
	if (ch_note2 != NULL)
		center_channel = atoi(ch_note2);

	if (primary_channel < 1) {
		ENG_LOG("Invalid channel : %d", center_channel);
		goto err;
	}

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_channel";
	argv[3] = ch_note1;
	if (center_channel == 0) {
		ENG_LOG("%s() should set center_channel with primary_channel\n", __func__);
		argv[4] = ch_note1;
	} else {
		argv[4] = ch_note2;
	}

	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	return return_ok(rsp);

err:
	return return_err(rsp);
}

int wifi_channel_get_marlin3(char *rsp)
{
	int ret = -1;
	int primary_channel = 0;
	int center_channel = 0;
	char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};
	int argc = 3;
	char *argv[3] = {"iwnpi", "wlan0", "get_channel"};

	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	memset(cmd, 0, WIFI_EUT_COMMAND_MAX_LEN);
	ret = get_iwnpi_ret_line(IWNPI_RET_CHN_FLAG, cmd, WIFI_EUT_COMMAND_MAX_LEN);
	if (ret <= 0) {
		ENG_LOG("%s(), get_channel run err[%d].\n", __FUNCTION__, ret);
		goto err;
	}
	eng_sscanf(cmd, "primary_channel:%d,center_channel:%d\n", &primary_channel, &center_channel);
	//  #ifdef __MARLIN3_USE_TWO_CHANNELS__
	if (primary_channel < 1 || center_channel < 1)
		goto err;
	sprintf(rsp, "%s%d,%d", WIFI_CHANNEL_REQ_RET, primary_channel, center_channel);
	//  #else /*__MARLIN3_USE_TWO_CHANNELS__*/
	//    chn = match_channel_table_by_chn(primary_channel, center_channel);
	//   ENG_LOG("%s(), get_channel-> %s [%d, %d].\n", __FUNCTION__, (chn != NULL ? chn->name : "null"), primary_channel, center_channel);
	//   if (chn == NULL)
	//     goto err;
	//   sprintf(rsp, "%s%s", WIFI_CHANNEL_REQ_RET, chn->name);
	// #endif  /*__MARLIN3_USE_TWO_CHANNELS__*/

	rsp_debug(rsp);
	return 0;

err:
	sprintf(rsp, "%s%s", WIFI_CHANNEL_REQ_RET, "null");
	rsp_debug(rsp);
	return -1;
}

int wifi_channel_set(char *ch_note1, char *ch_note2, char *rsp)
{
	/*just call marlin3 set channel */
	return wifi_channel_set_marlin3(ch_note1, ch_note2, rsp);
}

int wifi_channel_get(char *rsp)
{
	if (wifi_channel_get_marlin3(rsp) == 0)
		return 0;
	else
		return return_err(rsp);
}

int wifi_txgainindex_set(int index, char *rsp)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[4] = {"iwnpi","wlan0","set_tx_power",""};

	if (wifi_enter_eut_mode() == 0)
		goto err;

	ENG_LOG("%s(), index:%d\n", __FUNCTION__, index);
	sprintf(cmd, "%d", index);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	g_wifi_data.txgainindex = index;
	return return_ok(rsp);

err:
	g_wifi_data.txgainindex = -1;
	return return_err(rsp);
}

int wifi_txgainindex_get(char *rsp)
{
	/* int ret = -1;
	   char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};
	   int level_a, level_b;

	   wifi_status_dump();
	   if (wifi_enter_eut_mode() == 0)
	   goto err;
	   sprintf(cmd, "iwnpi wlan0 get_tx_power > %s", TMP_FILE);
	   if (0 != exec_cmd(cmd))
	   goto err;

	   memset(cmd, 0, sizeof(cmd));
	   ret = get_iwnpi_ret_line(cmd, IWNPI_RET_BEGIN, IWNPI_RET_END);
	   if (ret <= 0) {
	   ENG_LOG("%s(), get_tx_power run err\n", __FUNCTION__);
	   goto err;
	   }
	   eng_sscanf(cmd, "level_a:%d,level_b:%d\n", &level_a, &level_b);
	   sprintf(rsp, "%s%d,%d", WIFI_TXGAININDEX_REQ_RET, level_a, level_b);
	   rsp_debug(rsp);
	   return 0;
err: */
	return return_err(rsp);
}

static int wifi_tx_start(char *rsp)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[4] = {"iwnpi","wlan0","set_tx_count",""};
	int tx_start_cnt = 3;
	char *tx_start_argv[3] = {"iwnpi", "wlan0","tx_start"};

	ENG_LOG("entry %s()\n", __FUNCTION__);

	wifi_status_dump();
	if (1 == g_wifi_data.tx_start) {
		ENG_LOG("ADL %s(), tx_start is aleady set to 1, goto ok.\n", __FUNCTION__);
		goto ok;
	}

	if (1 == g_wifi_data.rx_start) {
		ENG_LOG("ADL %s(), Rx_start is 1, goto err. RX_START is 1\n", __FUNCTION__);
		goto err;
	}
	/*we can not set tx on in cw mode*/
	if (1 == g_wifi_data.sin_wave_start) {
		ENG_LOG("ADL %s(), sin_wave_start is 1, goto err. SIN_WAVE_start is 1\n", __FUNCTION__);
		goto err;
	}

	sprintf(cmd, "%d", g_wifi_tx_count);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	/* reset to default, must be 0 */
	g_wifi_tx_count = 0;

	if (0 != exec_iwnpi_cmd_with_status(tx_start_cnt, tx_start_argv))
		goto err;

ok:
	g_wifi_data.tx_start = 1;
	return return_ok(rsp);

err:
	g_wifi_data.tx_start = 0;
	return return_err(rsp);
}

static int wifi_tx_stop(char *rsp)
{
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","tx_stop"};

	ENG_LOG("ADL entry %s()\n", __FUNCTION__);

	wifi_status_dump();

	if (0 == g_wifi_data.tx_start && 0 == g_wifi_data.sin_wave_start) {
		goto ok;
	}

	if (1 == g_wifi_data.rx_start) {
		goto err;
	}

	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

ok:
	g_wifi_data.tx_start = 0;
	g_wifi_data.sin_wave_start = 0;
	return return_ok(rsp);

err:
	g_wifi_data.tx_start = 0;
	g_wifi_data.sin_wave_start = 0;
	return return_err(rsp);
}

int wifi_tx_set(int command_code, int mode, int pktcnt, char *rsp)
{
	ENG_LOG("entry %s(), command_code = %d, mode = %d, pktcnt = %d\n",
		__FUNCTION__, command_code, mode, pktcnt);

	if (wifi_enter_eut_mode() == 0)
		goto err;

	if ((1 == command_code) && (0 == g_wifi_data.rx_start)) {
		if (0 == mode) {
			/* continues */
			ENG_LOG("%s(), set g_wifi_tx_count is 0\n", __FUNCTION__);
			g_wifi_tx_count = 0;
		} else if (1 == mode) {
			if (pktcnt > 0) {
				ENG_LOG("ADL %s(), set g_wifi_tx_count is %d \n",
					__FUNCTION__, pktcnt);
				g_wifi_tx_count = pktcnt;
			} else {
				ENG_LOG("%s(), pktcnt is ERROR, pktcnt = %d\n",
					__FUNCTION__, pktcnt);
				goto err;
			}
		}

		if (-1 == wifi_tx_start(rsp))
			goto err;
	} else if (0 == command_code) {
		wifi_tx_stop(rsp);
	} else {
		goto err;
	}

	return return_ok(rsp);

err:
	return return_err(rsp);
}

int wifi_tx_get(char *rsp)
{
	sprintf(rsp, "%s%d", EUT_WIFI_TX_REQ, g_wifi_data.tx_start);
	rsp_debug(rsp);
	return 0;
}

static int wifi_rx_start(char *rsp)
{
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","rx_start"};

	ENG_LOG("ADL entry %s(), rx_start = %d, tx_start = %d \n",
		__FUNCTION__, g_wifi_data.rx_start, g_wifi_data.tx_start);

	wifi_status_dump();
	if (1 == g_wifi_data.rx_start)
		goto ok;
	if (1 == g_wifi_data.tx_start)
		goto err;

	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

ok:
	g_wifi_data.rx_start = 1;
	return return_ok(rsp);

err:
	g_wifi_data.rx_start = 0;
	return return_err(rsp);
}

static int wifi_rx_stop(char *rsp)
{
	int argc = 3;
	char *argv[3] = {"iwnpi", "wlan0", "rx_stop"};

	ENG_LOG("ADL entry %s(), rx_start = %d, tx_start = %d \n", __func__,
			g_wifi_data.rx_start, g_wifi_data.tx_start);

	wifi_status_dump();
	if (0 == g_wifi_data.rx_start)
		goto ok;
	if (1 == g_wifi_data.tx_start)
		goto err;

	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

ok:
	g_wifi_data.rx_start = 0;
	return return_ok(rsp);

err:
	g_wifi_data.rx_start = 0;
	return return_err(rsp);
}

int wifi_rx_set(int command_code, char *rsp)
{
	ENG_LOG("ADL entry %s(), command_code = %d \n", __func__, command_code);

	if (wifi_enter_eut_mode() == 0)
		goto err;

	if ((command_code == 1) && (0 == g_wifi_data.tx_start)) {
		wifi_rx_start(rsp);
	} else if (command_code == 0) {
		wifi_rx_stop(rsp);
	} else {
		goto err;
	}
	return return_ok(rsp);

err:
	return return_err(rsp);
}

int wifi_rx_get(char *rsp)
{
	ENG_LOG("%s()...\n", __FUNCTION__);
	sprintf(rsp, "%s%d", EUT_WIFI_RX_REQ, g_wifi_data.rx_start);
	rsp_debug(rsp);
	return 0;
}

int wifi_rssi_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 0;
	char *argv[3] = {"iwnpi","wlan0","get_rssi"};

	wifi_status_dump();
	if (1 == g_wifi_data.tx_start) {
		goto err;
	}
	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == ret) {
		ENG_LOG("iwnpi wlan0 get_rssi cmd err_code:%d", ret);
		goto err;
	}

	sprintf(rsp, "%s%d", EUT_WIFI_RSSI_REQ_RET, ret);
	rsp_debug(rsp);
	return 0;
err:
	return return_err(rsp);
}

int wifi_rxpktcnt_get(char *rsp)
{
	int ret = -1;
	RX_PKTCNT *cnt = NULL;
	char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0};
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_rx_ok"};

	ENG_LOG("ADL entry %s()", __func__);

	wifi_status_dump();
	if ((1 == g_wifi_data.tx_start)) {
		goto err;
	}

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	cnt = &(g_wifi_data.cnt);
	memset(cmd, 0x00, TMP_BUF_SIZE);
	ret = get_iwnpi_ret_line(IWNPI_RET_REG_FLAG, cmd, WIFI_EUT_COMMAND_MAX_LEN);
	ENG_LOG("ADL %s(), callED get_iwnpi_ret_line(), ret = %d", __func__, ret);
	if (ret <= 0) {
		ENG_LOG("%s, no iwnpi\n", __FUNCTION__);
		goto err;
	}
	eng_sscanf(cmd, "reg value: rx_end_count=%d rx_err_end_count=%d fcs_fail_count=%d ",
		   &(cnt->rx_end_count), &(cnt->rx_err_end_count),
		   &(cnt->fcs_fail_count));

	sprintf(rsp, "%s%d,%d,%d", EUT_WIFI_RXPKTCNT_REQ_RET,
		g_wifi_data.cnt.rx_end_count, g_wifi_data.cnt.rx_err_end_count,
		g_wifi_data.cnt.fcs_fail_count);
	rsp_debug(rsp);
	return 0;
err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_lna_set
 *   ---------------------------
 *   description: Set lna switch of WLAN Chip
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   lna_on_off  IN          int             lna's status
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 ********************************************************************/
int wifi_lna_set(int lna_on_off, char *rsp)
{
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0"};

	ENG_LOG("ADL entry %s(), lna_on_off = %d", __func__, lna_on_off);

	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (1 == lna_on_off) {
		argv[2] = "lna_on";
	} else if (0 == lna_on_off) {
		argv[2] = "lna_off";
	} else {
		ENG_LOG("%s(), lna_on_off is ERROR", __func__);
		goto err;
	}

	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_lna_get
 *   ---------------------------
 *   description: get Chip's LNA status by iwnpi command
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 ********************************************************************/
int wifi_lna_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","lna_status"};

	ENG_LOG("ADL entry %s()", __func__);
	wifi_status_dump();

	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == ret) {
		ENG_LOG("iwnpi wlan0 lna_status cmd, err_code = %d", ret);
		goto err;
	}

	sprintf(rsp, "%s%d", WIFI_LNA_STATUS_REQ_RET, ret);
	rsp_debug(rsp);
	return 0;
err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_band_set
 *   ---------------------------
 *   description: set Chip's band(2.4G/5G) by iwnpi command set_band
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   band        IN          wifi_band       band number.
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *   band:2.4G/5G
 *
 ********************************************************************/
int wifi_band_set(wifi_band band, char *rsp)
{
	ENG_LOG("ADL entry %s(),marlin3 isn't support set band", __FUNCTION__);
	return return_ok(rsp);
}


/********************************************************************
 *   name   wifi_band_get
 *   ---------------------------
 *   description: Get Chip's band(2.4G/5G) by iwnpi command from wifi RF
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *   band:2.4G/5G
 *
 ********************************************************************/
int wifi_band_get(char *rsp)
{
	if (WCN_HW_MARLIN3 == get_wcn_hw_type()) {
		ENG_LOG("ADL entry %s(),marlin3 isn't support get band", __FUNCTION__);
		goto err;
}

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_bandwidth_set
 *   ---------------------------
 *   description: set Chip's band width by iwnpi command set_cbw
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   band        IN          wifi_bandwidth  band width.
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *   20 40 80 160
 ********************************************************************/
int wifi_bandwidth_set(wifi_bandwidth band_width, char *rsp)
{
	char cmd[8] = {0x00};
	char argc = 4;
	char *argv[4];

	ENG_LOG("entry %s(), band_width = %d\n", __FUNCTION__, band_width);
	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (band_width > WIFI_BANDWIDTH_160MHZ) {
		ENG_LOG("%s(), band num is err, band_width = %d\n", __FUNCTION__, band_width);
		goto err;
	}

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_cbw";
	sprintf(cmd, "%d", band_width);
	argv[3] = cmd;

	if (0 != exec_iwnpi_cmd_with_status(argc, argv)) {
		ENG_LOG("exec set_cbw command error\n");
		goto err;
	}

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_bandwidth_get
 *   ---------------------------
 *   description: Get Chip's band width by iwnpi command from wifi RF
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *   band width:20 40 80 160
 *
 ********************************************************************/
int wifi_bandwidth_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_cbw"};

	ENG_LOG("ADL entry %s()", __func__);

	if (0 != exec_iwnpi_cmd(argc, argv)) {
		goto err;
	}

	ret = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == ret) {
		ENG_LOG("iwnpi wlan0 get_band cmd, err_code = %d", ret);
		goto err;
	}
	snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_BANDWIDTH_REQ_RET,
			ret);
	rsp_debug(rsp);
	return 0;

err:
	return return_err(rsp);
}

int wifi_sigbandwidth_set(int band_width, char *rsp)
{
	char cmd[8] = {0x00};
	char argc = 4;
	char *argv[4];

	ENG_LOG("entry %s(), band_width = %d\n", __FUNCTION__, band_width);
	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (band_width > 3) {
		ENG_LOG("%s(), band num is err, band_width = %d\n", __FUNCTION__, band_width);
		goto err;
	}

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_sbw";
	sprintf(cmd, "%d", band_width);
	argv[3] = cmd;

	if (0 != exec_iwnpi_cmd_with_status(argc, argv)) {
		ENG_LOG("exec set_cbw command error\n");
		goto err;
	}

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_bandwidth_get
 *   ---------------------------
 *   description: Get Chip's band width by iwnpi command from wifi RF
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *   band width:20 40 80 160
 *
 ********************************************************************/
int wifi_sigbandwidth_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_sbw"};

	ENG_LOG("ADL entry %s()", __FUNCTION__);

	if (0 != exec_iwnpi_cmd(argc, argv)) {
		goto err;
	}

	ret = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == ret) {
		ENG_LOG("iwnpi wlan0 get_band cmd, err_code = %d", ret);
		goto err;
	}
	snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_BANDWIDTH_REQ_RET,
			ret);
	rsp_debug(rsp);
	return 0;

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_tx_power_level_set
 *   ---------------------------
 *   description: set Chip's tx_power by iwnpi command set_tx_power
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   tx_power    IN          int             tx power
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_tx_power_level_set(int tx_power, char *rsp)
{
	char cmd[8] = {0x00};
	int argc = 4;
	char *argv[4] = {"iwnpi","wlan0","set_tx_power",""};

	ENG_LOG("entry %s(), tx_power = %d\n", __FUNCTION__, tx_power);
	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (tx_power < WIFI_TX_POWER_MIN_VALUE ||
	    tx_power > WIFI_TX_POWER_MAX_VALUE) {
		ENG_LOG("%s(), tx_power's value is invalid, tx_power = %d\n",
			__FUNCTION__, tx_power);
		goto err;
	}
	sprintf(cmd,"%d",tx_power);
	argv[3] = cmd;

	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_tx_power_level_get
 *   ---------------------------
 *   description: get Chip's tx power leaving by iwnpi command get_tx_power
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_tx_power_level_get(char *rsp)
{
	int tssi = WIFI_INT_INVALID_RET;
	char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_tx_power"};

	ENG_LOG("ADL entry %s()", __func__);
	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	tssi = get_result_from_iwnpi();
	if (tssi == WIFI_INT_INVALID_RET) {
		int level_a = 0, level_b = 0;
		memset(cmd, 0x00, WIFI_EUT_COMMAND_MAX_LEN);
		if (get_iwnpi_ret_line(IWNPI_RET_PWR_FLAG, cmd, WIFI_EUT_COMMAND_MAX_LEN) <= 0)
			goto err;
		eng_sscanf(cmd, "level_a:%d,level_b:%d", &level_a, &level_b);
		snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%d",
				WIFI_TX_POWER_LEVEL_REQ_RET, level_a, level_b);
		rsp_debug(rsp);
		return 0;
	}
	snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
			WIFI_TX_POWER_LEVEL_REQ_RET, tssi);
	rsp_debug(rsp);
	return 0;

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_pkt_length_set
 *   ---------------------------
 *   description: set Chip's pkt length by iwnpi command set_pkt_len
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   pkt_len     IN          int             pkt length
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_pkt_length_set(int pkt_len, char *rsp) {
	int ret = -1;
	char cmd[8] = {0x00};
	int argc = 4;
	char *argv[4] = {"iwnpi", "wlan0", "set_pkt_len"};

	ENG_LOG("ADL entry %s(), pkt_len = %d", __func__, pkt_len);
	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (pkt_len < WIFI_PKT_LENGTH_MIN_VALUE ||
	    pkt_len > WIFI_PKT_LENGTH_MAX_VALUE) {
		ENG_LOG("%s(), tx_power's value is invalid, pkt_len = %d",
			__func__, pkt_len);
		goto err;
	}

	sprintf(cmd, "%d", pkt_len);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv)) {
		ENG_LOG("set packet len error\n");
		goto err;
	}

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_pkt_length_get
 *   ---------------------------
 *   description: get Chip's pkt length by iwnpi command get_pkt_len
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_pkt_length_get(char *rsp)
{
	int pkt_length = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_pkt_len"};

	ENG_LOG("ADL entry %s()", __func__);

	if (0 != exec_iwnpi_cmd(argc, argv)) {
		goto err;
	}

	pkt_length = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == pkt_length) {
		ENG_LOG("iwnpi wlan0 get_pkt_len cmd, err_code = %d", pkt_length);
		goto err;
	}

	snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_PKT_LENGTH_REQ_RET,
			pkt_length);
	rsp_debug(rsp);
	return 0;

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_tx_mode_set
 *   ---------------------------
 *   description: set Chip's tx mode by iwnpi command set_tx_mode
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   tx_mode     IN          wifi_tx_mode    tx mode
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_tx_mode_set(wifi_tx_mode tx_mode, char *rsp)
{
	char cmd[8] = {0x00};
	int argc = 3;
	char *argv[3] = {"iwnpi", "wlan0", ""};

	ENG_LOG("entry %s(), tx_mode = %d", __FUNCTION__, tx_mode);
	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (tx_mode > WIFI_TX_MODE_LOCAL_LEAKAGE) {
		ENG_LOG("%s(), tx_power's value is invalid, tx_mode = %d",
			__FUNCTION__, tx_mode);
		goto err;
	}

	/*if tx mode is CW,must be tx stop when tx start is 1, rather, call sin_wave
	 */
	if (WIFI_TX_MODE_CW == tx_mode) {
		ENG_LOG("%s(), tx_mode is CW, tx_start = %d", __FUNCTION__,
			g_wifi_data.tx_start);
		if (1 == g_wifi_data.tx_start) {
			ENG_LOG("tx start already on, need stop tx first\n");
			argv[2] = "tx_stop";

			if (0 != exec_iwnpi_cmd_with_status(argc, argv))
				goto err;

			ENG_LOG("ADL %s(), set tx_start to 0", __func__);
			g_wifi_data.tx_start = 0;
		}
		ENG_LOG("start to send sin_wave\n");
		argv[2] = "sin_wave";

		if (0 != exec_iwnpi_cmd_with_status(argc, argv))
			goto err;

		g_wifi_data.sin_wave_start = 1;
		ENG_LOG("%s(), set sin_wave_start is 1.", __FUNCTION__);

	} else { /* other mode, send value to CP2 */
		ENG_LOG("%s(), tx_mode is other mode, mode = %d", __FUNCTION__, tx_mode);
		int cmd_cnt = 4;
		char *cmd_str[4];
		cmd_str[0] = "iwnpi";
		cmd_str[1] = "wlan0";
		cmd_str[2] = "set_tx_mode";
		sprintf(cmd, "%d",tx_mode);
		cmd_str[3] = cmd;

		if (0 != exec_iwnpi_cmd_with_status(cmd_cnt, cmd_str))
			goto err;
	}

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_tx_mode_get
 *   ---------------------------
 *   description: get Chip's tx mode by iwnpi command tx_mode
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_tx_mode_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_tx_mode"};

	ENG_LOG("ADL entry %s()", __func__);

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == ret) {
		ENG_LOG("iwnpi wlan0 get_tx_mode cmd, err_code = %d", ret);
		goto err;
	}
	snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_TX_MODE_REQ_RET,
			ret);
	rsp_debug(rsp);
	return 0;

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_preamble_set
 *   ---------------------------
 *   description: set Chip's preamble type by iwnpi command set_preamble
 *   ----------------------------
 *   para            IN/OUT      type                    note
 *   preamble_type   IN          wifi_preamble_type      tx power
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_preamble_set(wifi_preamble_type preamble_type, char *rsp)
{
	char cmd[8] = {0x00};
	int argc = 4;
	char *argv[4] = {"iwnpi", "wlan0", "set_preamble"};
	ENG_LOG("ADL entry %s(), preamble_type = %d", __func__, preamble_type);
	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (preamble_type > PREAMBLE_TYPE_80211N_GREEN_FIELD) {
		ENG_LOG("%s(), preamble's value is invalid, preamble_type = %d", __func__,
				preamble_type);
		goto err;
	}

	sprintf(cmd, "%d", preamble_type);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_preamble_get
 *   ---------------------------
 *   description: get Chip's preamble type by iwnpi command get_preamble
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_preamble_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_preamble"};

	ENG_LOG("ADL entry %s()", __func__);

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == ret) {
		ENG_LOG("iwnpi wlan0 get_tx_mode cmd, err_code = %d", ret);
		goto err;
	}
	snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_PREAMBLE_REQ_RET, ret);
	rsp_debug(rsp);
	return 0;

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_payload_set
 *   ---------------------------
 *   description: set Chip's payload by iwnpi command set_payload
 *   ----------------------------
 *   para            IN/OUT      type                    note
 *   payload         IN          wifi_payload            payload
 *   rsp             OUT         char *                  response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_payload_set(wifi_payload payload, char *rsp)
{
	char cmd[8] = {0x00};
	int argc = 4;
	char *argv[4] = {"iwnpi", "wlan0", "set_payload"};

	ENG_LOG("ADL entry %s(), payload = %d", __func__, payload);
	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (payload > PAYLOAD_RANDOM) {
		ENG_LOG("%s(), payload's value is invalid, payload = %d", __func__,
				payload);
		goto err;
	}

	sprintf(cmd, "%d", payload);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_payload_get
 *   ---------------------------
 *   description: get Chip's payload by iwnpi command get_payload
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_payload_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_payload"};

	ENG_LOG("ADL entry %s()", __func__);

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == ret) {
		ENG_LOG("iwnpi wlan0 get_tx_mode cmd, err_code = %d", ret);
		goto err;
	}
	snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d", WIFI_PAYLOAD_REQ_RET, ret);
	rsp_debug(rsp);
	return 0;

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_guardinterval_set
 *   ---------------------------
 *   description: set Chip's guard interval by iwnpi command set_gi
 *   ----------------------------
 *   para            IN/OUT      type                    note
 *   gi              IN          wifi_guard_interval     guard interval
 *   rsp             OUT         char *                  response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_guardinterval_set(wifi_guard_interval gi, char *rsp)
{
	char cmd[8] = {0x00};
	char argc = 4;
	char *argv[4] = {"iwnpi", "wlan0", "set_gi"};

	ENG_LOG("ADL entry %s(), gi = %d", __FUNCTION__, gi);

	if (gi > GUARD_INTERVAL_MAX_VALUE) {
		ENG_LOG("%s(), guard interval's value is invalid, gi = %d", __func__, gi);
		goto err;
	}

	sprintf(cmd,"%d", gi);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv)) {
		ENG_LOG("%s set gi error\n", __FUNCTION__);
		goto err;
	}

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_guardinterval_get
 *   ---------------------------
 *   description: get Chip's guard interval by iwnpi command get_gi
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_guardinterval_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_gi"};

	ENG_LOG("ADL entry %s()", __func__);

	if (0 != exec_iwnpi_cmd(argc, argv)) {
		goto err;
	}

	ret = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == ret) {
		ENG_LOG("iwnpi wlan0 get_gi cmd, err_code = %d", ret);
		goto err;
	}

	snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
		 WIFI_GUARDINTERVAL_REQ_RET, ret);
	rsp_debug(rsp);
	return 0;

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_mac_filter_set
 *   ---------------------------
 *   description: set Chip's mac filter by iwnpi command set_macfilter
 *   ----------------------------
 *   para            IN/OUT      type                    note
 *   on_off          IN          int                     on or off mac filter
 *function
 *   mac             IN          const char*             mac addr as string
 *format
 *   rsp             OUT         char *                  response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_mac_filter_set(int on_off, const char *mac, char *rsp)
{
	char cmd[WIFI_EUT_COMMAND_MAX_LEN] = {0x00};
	char mac_str[WIFI_MAC_STR_MAX_LEN + 1] = {0x00};

	//check marlin3 iwnpi code, set_mac_filter is not implement
	ENG_LOG("Marlin3 is not support set mac filter\n");
	return return_ok(rsp);

	ENG_LOG("ADL entry %s(), on_off = %d, mac = %s", __func__, on_off, mac);
	if (wifi_enter_eut_mode() == 0)
		goto err;

	if (on_off < 0 || on_off > 1) {
		ENG_LOG("%s(), on_off's value is invalid, on_off = %d", __func__, on_off);
		goto err;
	}

	/* parese mac as string if on_off is 1 */
	if (1 == on_off) {
		unsigned char len = 0;

		if (mac &&(strlen(mac) < (WIFI_MAC_STR_MAX_LEN + 2 + strlen("\r\n")))) {
			ENG_LOG("%s(), mac's length is invalid, len = %lu", __func__,
					strlen(mac));
			goto err;
		}

		if (mac && *mac == '\"') {
			/* skip first \" */
			mac++;
		}

		while (mac && *mac != '\"') {
			mac_str[len++] = *mac;
			mac++;
		}

		/* first, execute iwnpi wlan0 set_macfilter to set enable or disable */
		snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_macfilter %d > %s",
				on_off, TMP_FILE);
		if (0 != exec_iwnpi_cmd_for_status(cmd))
			goto err;

		/* second, execute iwnpi wlan0 set_mac set MAC assress */
		memset(cmd, 0x00, WIFI_EUT_COMMAND_MAX_LEN);
		snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_mac %s > %s",
				mac_str, TMP_FILE);
		if (0 != exec_cmd(cmd))
			goto err;

	} else if (0 == on_off) {
		snprintf(cmd, WIFI_EUT_COMMAND_MAX_LEN, "iwnpi wlan0 set_macfilter %d > %s",
				on_off, TMP_FILE);
		if (0 != exec_iwnpi_cmd_for_status(cmd))
			goto err;
	}

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
*   name   wifi_mac_filter_get
*   ---------------------------
*   description: get Chip's mac filter switch and mac addr if on, by iwnpi
*command get_macfilter
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_mac_filter_get(char *rsp)
{
	int ret = -1;
	int on_off = 0;
	int argc = 3, get_mac_argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_macfilter"};
	char *get_mac_argv[3] = {"iwnpi","wlan0","get_mac"};

	/* inquery mac filter switch */
	if (0 != exec_iwnpi_cmd(argc, argv)) {
		goto err;
	}

	on_off = get_result_from_iwnpi();
	if (WIFI_INT_INVALID_RET == ret) {
		ENG_LOG("%s() iwnpi wlan0 get mac filter cmd, err_code = %d", __func__, on_off);
		goto err;
	}

	if (1 == on_off) {
		char mac_str[WIFI_MAC_STR_MAX_LEN] = {0x00};

		/* the mac filter switch is on, get MAC address by iwnpi get_mac command */
		if (0 != exec_iwnpi_cmd(get_mac_argc, get_mac_argv)) {
			goto err;
		}

		ret = get_iwnpi_ret_line(IWNPI_RET_MAC_FLAG, mac_str, WIFI_MAC_STR_MAX_LEN);
		if (ret <= 0) {
			ENG_LOG("%s() iwnpi wlan0 get mac failed, err_code = %d", __func__, ret);
			goto err;
		}

		snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%s",
			 WIFI_MAC_FILTER_REQ_RET, on_off, mac_str);
	} else if (0 == on_off) {
		snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
			 WIFI_MAC_FILTER_REQ_RET, on_off);
	} else {
		ENG_LOG("%s(), on_off's value is invalid, on_off = %d", __func__, on_off);
		goto err;
	}

	return 0;

err:
	return return_err(rsp);
}

/********************************************************************
*   name   wifi_mac_efuse_get
*   ---------------------------
*   description: get Chip's mac from efuse by iwnpi
*
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_mac_efuse_get(char *rsp)
{
	int ret = -1;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_mac_efuse"};
	char mac_str[WIFI_MAC_EFUSE_MAX_LEN] = {0x00};

	ENG_LOG("ADL entry %s()", __FUNCTION__);

	/* inquery mac filter switch */
	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_iwnpi_ret_line("MAC", mac_str, WIFI_MAC_EFUSE_MAX_LEN);
	if (ret <= 0) {
		ENG_LOG("iwnpi wlan0 get mac cmd, err_code = %d", ret);
		goto err;
    	}

	snprintf(rsp, WIFI_EUT_COMMAND_RSP_MAX_LEN, "%s%s", WIFI_MAC_EFUSE_REQ_RET, mac_str);
	rsp_debug(rsp);
	return 0;

err:
  return return_err(rsp);
}

int wifi_mac_efuse_set(const char *wifi_mac, const char* bt_mac, char *rsp)
{
	char wifi_cmd[20] = {0};
	char bt_cmd[20] = {0};
	int argc = 5;
	char *argv[argc];
	int index = 0;
	char *pos1 = wifi_cmd;
	char *pos2 = bt_cmd;
	const char *tmp1 = wifi_mac;
	const char *tmp2 = bt_mac;

	if (wifi_enter_eut_mode() == 0)
		goto err;

	/* add : for wifi mac addr */
	for (index = 0; index < 6; index++) {
		memcpy(pos1, tmp1, 2);
		pos1 += 2;
		tmp1 +=2;
		if (index < 5) {
			*pos1 = ':';
			pos1++;
		}
	}

	/* add : for bt mac addr */
	for (index = 0; index < 6; index++) {
		memcpy(pos2, tmp2, 2);
		pos2 += 2;
		tmp2 +=2;
		if (index < 5) {
			*pos2 = ':';
			pos2++;
		}
	}
	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_mac_efuse";
	argv[3] = wifi_cmd;
	argv[4] = bt_cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	return return_ok(rsp);
err:
	return return_err(rsp);
}

int wifi_cdec_efuse_set(int cdec, char *rsp)
{
	char cmd[8] = {0};
	int argc = 5;
	char *argv[argc];
	if (wifi_enter_eut_mode() == 0)
		goto err;
	ENG_LOG("%s(), cdec:%d\n", __FUNCTION__, cdec);

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_efuse";
	argv[3] = "11";
	sprintf(cmd, "%02x", cdec);
	argv[4] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		 goto err;

	return return_ok(rsp);
err:
	g_wifi_data.chain = 0;
	return return_err(rsp);
}

#define CDEC_MASK 0x3f
int wifi_cdec_efuse_get(char *rsp)
{
	unsigned int argc = 4;
	char *argv[argc];
	int index = 2,ret;
	unsigned char cdec = 0;
	unsigned char tmp[4] = {0};
	char efuse_str[20] = {0};

	if (wifi_enter_eut_mode() == 0)
		goto err;

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "get_efuse";
	argv[3] = "11";
	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;
	ret = get_iwnpi_ret_line("efuse", efuse_str, 20);
	if (ret < 0) {
		ENG_LOG("%s get return line error\n", __FUNCTION__);
		goto err;
	}

	ENG_LOG("efuse_str : %s\n", efuse_str);
	eng_sscanf(efuse_str,"efuse:%02x%02x%02x%02x", &tmp[0], &tmp[1], &tmp[2], &tmp[3]);

	/* low 3 byte contain cdec info */
	for (index = 2; index >= 0; index--) {
		ENG_LOG("%02x\n",tmp[index]);
		unsigned char j = tmp[index];
		unsigned char flag = j >> 6;
		ENG_LOG("flag : %d\n",flag);
		if (flag == 0x2) {
			cdec = tmp[index] & CDEC_MASK;
			ENG_LOG("cdec is : %d\n", cdec);
			break;
		}
	}
	if (cdec > 64 || cdec < 0) {
		ENG_LOG("cdec is not invalid : %d\n",cdec);
		goto err;
	}
	ENG_LOG("cdec : %d\n", cdec);
	//return cdec value;
	sprintf(rsp, "%s%d", WIFI_CDEC_EFUSE_REQ_RET, cdec);
	rsp_debug(rsp);
	return 0;
err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_ant_get
 *   ---------------------------
 *   description: get Chip's chain by iwnpi *command get_chain
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_ant_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_chain"};

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_result_from_iwnpi();
	if (ret < 0) {
		ENG_LOG("iwnpi wlan0 get_chain cmd, err_code:%d", ret);
		goto err;
	}

	g_wifi_data.chain = (WIFI_CHAIN)ret;
	sprintf(rsp, "%s%d", WIFI_ANT_REQ_RET, ret);
	rsp_debug(rsp);
	return 0;

err:
	g_wifi_data.chain = 0;
	return return_err(rsp);
}

/********************************************************************
*   name   wifi_ant_info
*   ---------------------------
*   description: get Chip's chain by iwnpi *command get_rf_config
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_ant_info(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_rf_config"};
	char ant_info[16] = {0x00};
	int rf_2g,rf_5g;

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_iwnpi_ret_line("ANTINFO", ant_info, 16);
	printf("ant info is : %s\n", ant_info);
	if (ret <= 0) {
		ENG_LOG("iwnpi wlan0 get mac cmd, err_code = %d", ret);
		goto err;
	}
	eng_sscanf(ant_info, "ANTINFO=%d,%d", &rf_2g, &rf_5g);

	printf("chan1 : %d, chan2 : %d\n", rf_2g, rf_5g);
	sprintf(rsp, "%s%d,%d", WIFI_ANTINFO_REQ_RET, rf_2g, rf_5g);
	rsp_debug(rsp);
	return 0;

err:
	g_wifi_data.chain = 0;
	return return_err(rsp);
}

int wifi_efuse_info(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_efuse_info"};
	char efuse_info[32] = {0x00};
	int cdec,mac,tpc;

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_iwnpi_ret_line("EFUSEINFO", efuse_info, 16);
	ENG_LOG("%s efuse info is : %s\n",__func__, efuse_info);
	if (ret <= 0) {
		ENG_LOG("%s() iwnpi wlan0 get efuse info, err_code = %d", __func__, ret);
		goto err;
	}
	eng_sscanf(efuse_info, "EFUSEINFO=%d,%d,%d", &cdec, &mac, &tpc);

	ENG_LOG("CDEC: %d, MAC: %d, TPC: %d\n", cdec, mac, tpc);
	sprintf(rsp, "%s%d,%d,%d", WIFI_EFUSEINFO_REQ_RET, cdec, mac, tpc);
	rsp_debug(rsp);
	return 0;

err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_ant_set
 *   ---------------------------
 *   description: set Chip's chain by iwnpi command set_chain
 *   ----------------------------
 *   para            IN/OUT      type                    note
 *   chain           IN          int                     wifi chain
 *   rsp             OUT         char *                  response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_ant_set(int chain, char *rsp)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[argc];
	if (wifi_enter_eut_mode() == 0)
		goto err;
	ENG_LOG("%s(), chain:%d\n", __FUNCTION__, chain);

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_chain";
	sprintf(cmd, "%d", chain);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	g_wifi_data.chain = chain;
	return return_ok(rsp);
err:
	g_wifi_data.chain = 0;
	return return_err(rsp);
}

/********************************************************************
*   name   wifi_cbank_set
*   ---------------------------
*   description: set Chip's cbank by iwnpi command set_cbank_ret
*   ----------------------------
*   para            IN/OUT      type                    note
*   cbank           IN          int                     cbank [0--63]
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_cbank_set(int cbank, char *rsp)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[argc];
	if (wifi_enter_eut_mode() == 0)
		goto err;
	ENG_LOG("%s(), cbank:%d\n", __FUNCTION__, cbank);

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_cbank_reg";
	sprintf(cmd, "%d", cbank);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;
	g_afc_cdec = cbank;
	ENG_LOG("%s(), g_afc_cdec:%d\n", __FUNCTION__, g_afc_cdec);

	return return_ok(rsp);
err:
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_netmode_set
 *   ---------------------------
 *   description: set netmode, marlin3 not support this command
 *   ----------------------------
 *
 ********************************************************************/
int wifi_netmode_set(int chain, char *rsp)
{
	ENG_LOG("marlin3 not support this command,so return ok\n");
	return return_ok(rsp);
}
/********************************************************************
 *   name   wifi_decode_mode_get
 *   ---------------------------
 *   description: get Chip's decode mode by iwnpi *command get_fec
 *   ----------------------------
 *   para        IN/OUT      type            note
 *   rsp         OUT         char *          response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_decode_mode_get(char *rsp)
{
	int ret = WIFI_INT_INVALID_RET;
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0","get_fec"};

	if (0 != exec_iwnpi_cmd(argc, argv))
		goto err;

	ret = get_result_from_iwnpi();
	if (ret < 0) {
		ENG_LOG("iwnpi wlan0 get_fec cmd  err_code:%d", ret);
		goto err;
	}

	g_wifi_data.decode_mode = (WIFI_DECODEMODE) ret;
	sprintf(rsp, "%s%d", WIFI_DECODEMODE_REQ_RET, ret);
	rsp_debug(rsp);
	return 0;
err:
	g_wifi_data.decode_mode = DM_BCC;
	return return_err(rsp);
}

/********************************************************************
 *   name   wifi_decode_mode_set
 *   ---------------------------
 *   description: set Chip's decode mode by iwnpi command set_fec
 *   ----------------------------
 *   para            IN/OUT      type                    note
 *   dec_mode        IN          int                     wifi decode mode
 *   rsp             OUT         char *                  response result
 *   ----------------------------------------------------
 *   return
 *   0:exec successful
 *   -1:error
 *   ------------------
 *   other:
 *
 *
 ********************************************************************/
int wifi_decode_mode_set(int dec_mode, char *rsp)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[argc];

	if (wifi_enter_eut_mode() == 0)
		goto err;

	ENG_LOG("%s(), dec_mode:%d\n", __FUNCTION__, dec_mode);

	if (dec_mode > DM_LDPC || dec_mode < DM_BCC)
		goto err;

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_fec";
	sprintf(cmd, "%d", dec_mode);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	g_wifi_data.decode_mode = (WIFI_DECODEMODE)dec_mode;
	return return_ok(rsp);

err:
	g_wifi_data.decode_mode = DM_BCC;
	return return_err(rsp);
}

/********************************************************************
*   name   wifi_cal_txpower_set
*   ---------------------------
*   description: set Chip's cal tx power by iwnpi cmd set_cal_txpower
*   ----------------------------
*   para            IN/OUT      type                    note
*   cal_txpower     IN          int                     wifi cal tx power
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_cal_txpower_set(int cal_txpower, char *rsp)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[argc];

	if (wifi_enter_eut_mode() == 0)
		goto err;

	ENG_LOG("%s(), cal_txpower:%d\n", __FUNCTION__, cal_txpower);

	if (cal_txpower < 0)
		goto err;

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_cal_txpower";
	sprintf(cmd, "%d", cal_txpower);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
*   name   wifi_cal_txpower_efuse_en
*   ---------------------------
*   description: Set cal tx power efuse en by iwnpi cmd cal_txpower_efuse_en
*   ----------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wifi_cal_txpower_efuse_en(char *rsp)
{
	int argc = 3;
	char *argv[3] = {"iwnpi","wlan0"};

	ENG_LOG("ADL entry %s()", __func__);

	if (wifi_enter_eut_mode() == 0)
		goto err;

	argv[2] = "cal_txpower_efuse_en";

	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
	goto err;

	return return_ok(rsp);

err:
	return return_err(rsp);
}

/********************************************************************
*   name   wifi_set_tpc_mode
*   ---------------------------
*   description: set wifi tpc mode by iwnpi cmd set_tpc_mode
*   ----------------------------
*   para            IN/OUT      type                    note
*   tpc_mode	    IN          int                     wifi tpc mode
*   rsp             OUT         char *                  response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int wifi_set_tpc_mode(int tpc_mode, char *rsp)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[argc];

	if (wifi_enter_eut_mode() == 0)
		goto err;

	ENG_LOG("%s(), tpc_mode:%d\n", __FUNCTION__, tpc_mode);

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_tpc_mode";
	sprintf(cmd, "%d", tpc_mode);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	return return_ok(rsp);

err:
	return return_err(rsp);
}

int wifi_set_tssi(int tssi, char *rsp)
{
	char cmd[8] = {0};
	int argc = 4;
	char *argv[argc];

	if (wifi_enter_eut_mode() == 0)
	goto err;

	argv[0] = "iwnpi";
	argv[1] = "wlan0";
	argv[2] = "set_tssi";
	sprintf(cmd, "%d", tssi);
	argv[3] = cmd;
	if (0 != exec_iwnpi_cmd_with_status(argc, argv))
		goto err;

	return return_ok(rsp);

err:
	return return_err(rsp);
}
/********************************end of the file*************************************/
