#include <zephyr.h>
#include <kernel.h>
#include <init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <device.h>
#include <uart.h>
#include <logging/sys_log.h>


#include "eng_diag.h"
#include "engpc.h"
#include "eng_cmd4linuxhdlr.h"
#include "wifi_eut_sprd.h"
#include "eut_opt.h"

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))

static char eng_diag_buf[ENG_DIAG_SIZE];
static int eng_diag_len = 0;
static int s_cmd_index = -1;


struct eut_cmd eut_cmds[] = {
	{EUT_REQ_INDEX, ENG_EUT_REQ},  // sprd
	{EUT_INDEX, ENG_EUT},
	{GPSSEARCH_REQ_INDEX, ENG_GPSSEARCH_REQ},
	{GPSSEARCH_INDEX, ENG_GPSSEARCH},
	{WIFICH_REQ_INDEX, ENG_WIFICH_REQ},
	{WIFICH_INDEX, ENG_WIFICH},

	{WIFIRATIO_REQ_INDEX, ENG_WIFIRATIO_REQ},
	{WIFIRATIO_INDEX, ENG_WIFIRATIO},

	/* Tx Power Level */
	{WIFITXPWRLV_REQ_INDEX, ENG_WIFITXPWRLV_REQ},
	{WIFITXPWRLV_INDEX, ENG_WIFITXPWRLV},

	/* Tx FAC */
	{WIFITX_FACTOR_REQ_INDEX, ENG_WIFITX_FACTOR_REQ},
	{WIFITX_FACTOR_INDEX, ENG_WIFITX_FACTOR},

	/* TX Mode */
	{WIFITXMODE_REQ_INDEX, ENG_WIFITXMODE_REQ},
	{WIFITXMODE_INDEX, ENG_WIFITXMODE},

	/* TX Gain */
	{ENG_WIFITXGAININDEX_REQ_INDEX, ENG_WIFITXGAININDEX_REQ},  // sprd
	{ENG_WIFITXGAININDEX_INDEX, ENG_WIFITXGAININDEX},

	/* TX */
	{TX_REQ_INDEX, ENG_TX_REQ},
	{TX_INDEX, ENG_TX},

	/* RX */
	{RX_REQ_INDEX, ENG_RX_REQ},
	{RX_INDEX, ENG_RX},

	/* Mode, is not TX Mode*/
	{WIFIMODE_INDEX, ENG_WIFIMODE},

	{WIFIRX_PACKCOUNT_INDEX, ENG_WIFIRX_PACKCOUNT},
	{WIFICLRRXPACKCOUNT_INDEX, ENG_WIFI_CLRRXPACKCOUNT},
	{GPSPRNSTATE_REQ_INDEX, ENG_GPSPRNSTATE_REQ},
	{GPSSNR_REQ_INDEX, ENG_GPSSNR_REQ},
	{GPSPRN_INDEX, ENG_GPSPRN},
	{ENG_WIFIRATE_REQ_INDEX, ENG_WIFIRATE_REQ},
	{ENG_WIFIRATE_INDEX, ENG_WIFIRATE},
	{ENG_WIFIRSSI_REQ_INDEX, ENG_WIFIRSSI_REQ},

	/* LNA */
	{WIFILNA_REQ_INDEX, ENG_WIFILNA_REQ},
	{WIFILNA_INDEX, ENG_WIFILNA},

	/* Band */
	{WIFIBAND_REQ_INDEX, ENG_WIFIBAND_REQ},
	{WIFIBAND_INDEX, ENG_WIFIBAND},

	/* Band Width */
	{WIFIBANDWIDTH_REQ_INDEX, ENG_WIFIBANDWIDTH_REQ},
	{WIFIBANDWIDTH_INDEX, ENG_WIFIBANDWIDTH},

	/* Signal Band Width */
	{WIFISIGBANDWIDTH_REQ_INDEX, ENG_WIFISIGBANDWIDTH_REQ},
	{WIFISIGBANDWIDTH_INDEX, ENG_WIFISIGBANDWIDTH},

	/* Pkt Length */
	{WIFIPKTLEN_REQ_INDEX, ENG_WIFIPKTLEN_REQ},
	{WIFIPKTLEN_INDEX, ENG_WIFIPKTLEN},

	/* Preamble */
	{WIFIPREAMBLE_REQ_INDEX, ENG_WIFIPREAMBLE_REQ},
	{WIFIPREAMBLE_INDEX, ENG_WIFIPREAMBLE},

	/* Payload */
	{WIFIPAYLOAD_REQ_INDEX, ENG_WIFIPAYLOAD_REQ},
	{WIFIPAYLOAD_INDEX, ENG_WIFIPAYLOAD},

	/* Guard Interval */
	{WIFIGUARDINTERVAL_REQ_INDEX, ENG_GUARDINTERVAL_REQ},
	{WIFIGUARDINTERVAL_INDEX, ENG_GUARDINTERVAL},

	/* MAC Filter */
	{WIFIMACFILTER_REQ_INDEX, ENG_MACFILTER_REQ},
	{WIFIMACFILTER_INDEX, ENG_MACFILTER},  // sprd

	/* ANT chain */
	{WIFIANT_REQ_INDEX, ENG_WIFIANT_REQ},
	{WIFIANT_INDEX, ENG_WIFIANT},

	{WIFINETMODE_INDEX, ENG_WIFINETMODE},

	/* Decode mode */
	{WIFIDECODEMODE_REQ_INDEX, ENG_WIFIDECODEMODE_REQ},
	{WIFIDECODEMODE_INDEX, ENG_WIFIDECODEMODE},

	/*set cbank to register*/
	{WIFICBANK_INDEX, ENG_WIFICBANK},
	{WIFICDECEFUSE_INDEX,ENG_CDECEFUSE},
	{WIFICDECEFUSE_REQ_INDEX,ENG_CDECEFUSE_REQ},

	/* MAC Filter */
	{WIFIMACEFUSE_REQ_INDEX, ENG_MACEFUSE_REQ},
	{WIFIMACEFUSE_INDEX, ENG_MACEFUSE},  // sprd

	{WIFIANTINFO_REQ_INDEX, ENG_WIFIANTINFO_REQ},
};
void eng_dump(unsigned char *buf, int len, int col, int flag, char *keyword)
{
	int index = 0;
	for (index = 0; index < len; index++) {
		ENG_LOG("%02x ",buf[index]);
	}
	ENG_LOG("\n");
}

//handle for 7d7e.
int eng_diag_decode7d7e(unsigned char *buf, int len)
{
	int i, j, m = 0;
	unsigned char tmp;
	for (i = 0; i < len; i++) {
		if ((buf[i] == 0x7d) || (buf[i] == 0x7e)) {
			tmp = buf[i + 1] ^ 0x20;
			ENG_LOG("%s: tmp=%x, buf[%d]=%x", __FUNCTION__, tmp, i + 1, buf[i + 1]);
			buf[i] = tmp;
			for(j = i + 1; j < len; j ++){
				buf[j] = buf[j+1];
			}
			len--;
			m++;
			ENG_LOG("%s AFTER:", __FUNCTION__);
			/*
			   for(j=0; j<len; j++) {
			   ENG_LOG("%x,",buf[j]);
			   }*/
		}
	}
	ENG_LOG("%s: m=%d", __FUNCTION__, m);
	return m;
}

int eng_diag_encode7d7e(char *buf, int len, int *extra_len)
{
	int i, j;
	char tmp;

	ENG_LOG("%s: len=%d", __FUNCTION__, len);

	for (i = 0; i < len; i++) {
		if ((buf[i] == 0x7d) || (buf[i] == 0x7e)) {
			tmp = buf[i] ^ 0x20;
			ENG_LOG("%s: tmp=%x, buf[%d]=%x", __FUNCTION__, tmp, i, buf[i]);
			buf[i] = 0x7d;
			for (j = len; j > i + 1; j--) {
				buf[j] = buf[j - 1];
			}
			buf[i + 1] = tmp;
			len++;
			(*extra_len)++;

			ENG_LOG("%s: AFTER:[%d]", __FUNCTION__, len);
			for (j = 0; j < len; j++) {
				ENG_LOG("%x,", buf[j]);
			}
		}
	}

	return len;
}

int is_ap_at_cmd_need_to_handle(char *buf, int len)
{
	char *ptr = NULL;

	if (NULL == buf) {
		ENG_LOG("%s,null pointer", __FUNCTION__);
		return 0;
	}

	ptr = buf + 1 + sizeof(MSG_HEAD_T);

	if (-1 != (s_cmd_index = eng_at2linux(ptr))) {
		ENG_LOG("s_cmd_index : %d\n", s_cmd_index);
		return 1;
	} else {
		return 0;
	}
}

int get_sub_str(const char *buf, char **revdata, char a, char *delim,
		unsigned char count, unsigned char substr_max_len)
{
	int len;
	char *start = NULL;
	char *substr = NULL;
	const char *end = buf;

	start = strchr(buf, a);
	substr = strstr(buf, delim);

	if (!substr) {
		return 0;
	}

	while (end && *end != '\0') {
		end++;
	}

	if ((NULL != start) && (NULL != end)) {
		start++;
		substr++;
		len = substr - start - 1;

		/* get cmd name */
		memcpy(revdata[0], start, len);

		ENG_LOG("command name : %s \n",revdata[0]);

		/*there are some problem when use strtok, so parse string directly*/
		int start_pos = 0, pos = 0, tmp = 1;
		int length = strlen(substr);
		ENG_LOG("length : %d, substr : %s\n",length, substr);

		for(pos = 0; pos < length; pos++) {
			if(substr[pos] == ',') {
				memcpy(revdata[tmp], substr + start_pos, pos - start_pos);
				ENG_LOG("value : revdata[%d] : %s\n",tmp, revdata[tmp]);
				tmp++;
				start_pos = pos + 1;
			}
			else if(pos == length -1) {
				memcpy(revdata[tmp], substr + start_pos, pos - start_pos + 1);
				ENG_LOG("value : revdata[%d] : %s\n",tmp, revdata[tmp]);
			}
		}
		/* get sub str by delimeter, no need use strtok*/
		//   tokenPtr = strtok(substr, delim);
		//   while (NULL != tokenPtr && index < count) {
		//     strncpy(revdata[index++], tokenPtr, substr_max_len);

		/* next */
		//    tokenPtr = strtok(NULL, delim);
		//   }
	}

	return 0;
}

int get_cmd_index(char *buf)
{
	int index = -1;
	int i;
	ENG_LOG("%s(), buf = %s \n", __FUNCTION__, buf);
	for (i = 0; i < (int)NUM_ELEMS(eut_cmds); i++) {
		if (strncmp(buf, eut_cmds[i].name, strlen(eut_cmds[i].name)) == 0
		    && (strlen(buf) == strlen(eut_cmds[i].name))) {
			ENG_LOG(
					"i=%d, eut_cmds[i].index= "
					"%d, eut_cmds[i].name = %s\n",
					i, eut_cmds[i].index, eut_cmds[i].name);
			index = eut_cmds[i].index;
			break;
		}
	}
	return index;
}



int eng_atdiag_wifi_euthdlr(char *buf, int len, char *rsp, int module_index)
{
	// spreadtrum
	char args0[32 + 1] = {0x00};
	char args1[32 + 1] = {0x00};
	char args2[32 + 1] = {0x00};
	char args3[32 + 1] = {0x00};

	char *data[4] = {args0, args1, args2, args3};
	int cmd_index = -1;
	char *tmp = buf;
	char *start = buf;

	ENG_LOG("buf: 0x%x.\n", buf);
	ENG_LOG("module=%d, buf = %s \n", module_index, buf);
	tmp = strchr(buf, '?');
	ENG_LOG("tmp = 0x%x.\n", tmp);
	if (tmp != NULL) {
		ENG_LOG("it is a query command\n");
		start = strchr(buf, '=');
		if (start == NULL) {
			ENG_LOG("can not find = charator in buf\n");
			strcpy(rsp, "can not match the at command");
			return -1;
		}
		start++;
		strncpy(args0, start, tmp - start + 1);
		ENG_LOG("command name : args0 : %s \n", args0);
	} else {
		ENG_LOG("it is a set command\n");
		get_sub_str(buf, data, '=', ",", 4, 32);
		ENG_LOG("set command name:%s\n", args0);
	}

	cmd_index = get_cmd_index(args0);
	ENG_LOG(
			"%s(), args0 = %s, args1 = %s, args2 = %s, args3 = %s cmd_index = "
			"%d, module_index = %d\n",
			__FUNCTION__, args0, args1, args2, args3, cmd_index, module_index);

	switch (cmd_index) {
		case EUT_REQ_INDEX:
			if (module_index == WIFI_MODULE_INDEX) {
				ENG_LOG("case WIFIEUT_INDEX 1");
				wifi_eut_get(rsp);
			}

			break;

		case EUT_INDEX:
			if (module_index == WIFI_MODULE_INDEX) {
				ENG_LOG("case WIFIEUT_INDEX 2");
				wifi_eut_set(atoi(data[1]), rsp);
			}

			break;
		case WIFICH_REQ_INDEX:
			wifi_channel_get(rsp);
			break;
		case WIFICH_INDEX:
			ENG_LOG("case WIFICH_INDEX   %d", WIFICH_INDEX);
			wifi_channel_set(data[1], data[2], rsp);
			break;
		case WIFIMODE_INDEX:
			// wifi_eutops.set_wifi_mode(data[1],rsp);
			break;
		case WIFIRATIO_INDEX:
			ENG_LOG("case WIFIRATIO_INDEX   %d", WIFIRATIO_INDEX);
			// wifi_eutops.set_wifi_ratio(atof(data[1]),rsp);
			break;
		case WIFITX_FACTOR_INDEX:
			// wifi_eutops.set_wifi_tx_factor(atol(data[1]),rsp);
			break;

		case TX_INDEX: {
			ENG_LOG("ADL %s(), case TX_INDEX, module_index = %d\n", __FUNCTION__,
				module_index);

			if (module_index == WIFI_MODULE_INDEX) {
				ENG_LOG("%s(), case TX_INDEX, call wifi_tx_set()\n", __FUNCTION__);
				wifi_tx_set(atoi(data[1]), atoi(data[2]), atoi(data[3]), rsp);
			} else {
				ENG_LOG("%s(), case TX_INDEX, module_index is ERROR\n", __FUNCTION__);
			}
		} break;

		case RX_INDEX: {
			ENG_LOG("ADL %s(), case RX_INDEX, module_index = %d", __FUNCTION__,
				module_index);

			if (module_index == WIFI_MODULE_INDEX) {
				ENG_LOG("ADL %s(), case RX_INDEX, call wifi_rx_set()", __FUNCTION__);
				wifi_rx_set(atoi(data[1]), rsp);
			} else {
				ENG_LOG("ADL %s(), case RX_INDEX, module_index is ERROR", __FUNCTION__);
			}
		} break;

		case TX_REQ_INDEX: {
			ENG_LOG("ADL %s(), case TX_REQ_INDEX, module_index = %d", __FUNCTION__,
				module_index);

			if (WIFI_MODULE_INDEX == module_index) {
				ENG_LOG("ADL %s(), case TX_REQ_INDEX, call wifi_tx_get()", __FUNCTION__);
				wifi_tx_get(rsp);
			} else {
				ENG_LOG("ADL %s(), case TX_REQ_INDEX, module_index is ERROR", __FUNCTION__);
			}
		} break;

		case RX_REQ_INDEX: {
			ENG_LOG("ADL %s(), case RX_REQ_INDEX, module_index = %d", __FUNCTION__,
				module_index);

			if (WIFI_MODULE_INDEX == module_index) {
				ENG_LOG("ADL %s(), case RX_REQ_INDEX, call wifi_rx_get()", __FUNCTION__);
				wifi_rx_get(rsp);
			} else {
				ENG_LOG("ADL %s(), case RX_REQ_INDEX, module_index is ERROR", __FUNCTION__);
			}
		} break;

		case WIFITX_FACTOR_REQ_INDEX:
			// wifi_eutops.wifi_tx_factor_req(rsp);

			break;
		case WIFIRATIO_REQ_INDEX:
			// wifi_eutops.wifi_ratio_req(rsp);

			break;
		case WIFIRX_PACKCOUNT_INDEX:
			wifi_rxpktcnt_get(rsp);
			break;
		case WIFICLRRXPACKCOUNT_INDEX:
			// wifi_eutops.wifi_clr_rxpackcount(rsp);
			break;

		//-----------------------------------------------------
		case ENG_WIFIRATE_INDEX:
			ENG_LOG("%s(), case:ENG_WIFIRATE_INDEX\n", __FUNCTION__);
			wifi_rate_set(data[1], rsp);
			break;
		case ENG_WIFIRATE_REQ_INDEX:
			ENG_LOG("%s(), case:ENG_WIFIRATE_REQ_INDEX\n", __FUNCTION__);
			wifi_rate_get(rsp);
			break;
		case ENG_WIFITXGAININDEX_INDEX:
			ENG_LOG("%s(), case:ENG_WIFITXGAININDEX_INDEX\n", __FUNCTION__);
			wifi_txgainindex_set(atoi(data[1]), rsp);
			break;
		case ENG_WIFITXGAININDEX_REQ_INDEX:
			ENG_LOG("%s(), case:ENG_WIFITXGAININDEX_REQ_INDEX\n", __FUNCTION__);
			wifi_txgainindex_get(rsp);
			break;
		case ENG_WIFIRSSI_REQ_INDEX:
			ENG_LOG("%s(), case:ENG_WIFIRSSI_REQ_INDEX\n", __FUNCTION__);
			wifi_rssi_get(rsp);
			break;

		/* lna */
		case WIFILNA_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFILNA_REQ_INDEX", __func__);
			wifi_lna_get(rsp);
			} break;

		case WIFILNA_INDEX: {
			ENG_LOG("%s(), case:WIFILNA_INDEX", __func__);
			wifi_lna_set(atoi(data[1]), rsp);
		} break;

		/* Band */
		case WIFIBAND_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIBAND_REQ_INDEX", __func__);
			wifi_band_get(rsp);
		} break;

		case WIFIBAND_INDEX: {
			ENG_LOG("%s(), case:WIFIBAND_INDEX", __func__);
			wifi_band_set((wifi_band)atoi(data[1]), rsp);
		} break;

		/* Band Width */
		case WIFIBANDWIDTH_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIBANDWIDTH_REQ_INDEX", __func__);
			wifi_bandwidth_get(rsp);
		} break;

		case WIFIBANDWIDTH_INDEX: {
			ENG_LOG("%s(), case:WIFIBANDWIDTH_INDEX", __func__);
			wifi_bandwidth_set((wifi_bandwidth)atoi(data[1]), rsp);
		} break;

		/* Signal Band Width */
		case WIFISIGBANDWIDTH_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIBANDWIDTH_REQ_INDEX", __FUNCTION__);
			wifi_sigbandwidth_get(rsp);
		} break;

		case WIFISIGBANDWIDTH_INDEX: {
			ENG_LOG("%s(), case:WIFIBANDWIDTH_INDEX", __FUNCTION__);
			wifi_sigbandwidth_set((wifi_bandwidth)atoi(data[1]), rsp);
		} break;

		/* Tx Power Level */
		case WIFITXPWRLV_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFITXPWRLV_REQ_INDEX", __func__);
			wifi_tx_power_level_get(rsp);
		} break;

		case WIFITXPWRLV_INDEX: {
			ENG_LOG("%s(), case:WIFITXPWRLV_INDEX", __func__);
			wifi_tx_power_level_set(atoi(data[1]), rsp);
		} break;

		/* Pkt Length */
		case WIFIPKTLEN_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIPKTLEN_REQ_INDEX", __func__);
			wifi_pkt_length_get(rsp);
		} break;

		case WIFIPKTLEN_INDEX: {
			ENG_LOG("%s(), case:WIFIPKTLEN_INDEX", __func__);
			wifi_pkt_length_set(atoi(data[1]), rsp);
		} break;

		/* TX Mode */
		case WIFITXMODE_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFITXMODE_REQ_INDEX", __func__);
			wifi_tx_mode_get(rsp);
		} break;

		case WIFITXMODE_INDEX: {
			ENG_LOG("%s(), case:WIFIPKTLEN_INDEX", __func__);
			wifi_tx_mode_set((wifi_tx_mode)atoi(data[1]), rsp);
		} break;

		/* Preamble */
		case WIFIPREAMBLE_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIPREAMBLE_REQ_INDEX", __func__);
			wifi_preamble_get(rsp);
		} break;

		case WIFIPREAMBLE_INDEX: {
			ENG_LOG("%s(), case:WIFIPREAMBLE_INDEX", __func__);
			wifi_preamble_set((wifi_tx_mode)atoi(data[1]), rsp);
		} break;

		/* Payload */
		case WIFIPAYLOAD_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIPAYLOAD_REQ_INDEX", __func__);
			wifi_payload_get(rsp);
		} break;

		case WIFIPAYLOAD_INDEX: {
			ENG_LOG("%s(), case:WIFIPAYLOAD_INDEX", __func__);
			wifi_payload_set((wifi_payload)atoi(data[1]), rsp);
		} break;

		/* Payload */
		case WIFIGUARDINTERVAL_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIGUARDINTERVAL_REQ_INDEX", __func__);
			wifi_guardinterval_get(rsp);
		} break;

		case WIFIGUARDINTERVAL_INDEX: {
			ENG_LOG("%s(), case:WIFIGUARDINTERVAL_INDEX", __func__);
			wifi_guardinterval_set((wifi_guard_interval)atoi(data[1]), rsp);
		} break;

		/* MAC Filter */
		case WIFIMACFILTER_INDEX: {
			ENG_LOG("%s(), case:WIFIMACFILTER_INDEX", __func__);
			wifi_mac_filter_set(atoi(data[1]), data[2], rsp);
		} break;

		case WIFIMACEFUSE_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIMACFILTER_REQ_INDEX", __func__);
			wifi_mac_efuse_get(rsp);
		} break;

		case WIFIMACEFUSE_INDEX: {
			ENG_LOG("%s(), case:WIFIMACFILTER_INDEX", __func__);
			wifi_mac_efuse_set(data[1], data[2], rsp);
		} break;

		/* ANT chain */
		case WIFIANT_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIANT_REQ_INDEX", __func__);
			wifi_ant_get(rsp);
		} break;

		/* get ant info */
		case WIFIANTINFO_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIANTINFO_REQ_INDEX", __func__);
			wifi_ant_info(rsp);
		} break;

		case WIFIANT_INDEX: {
			ENG_LOG("%s(), case:WIFIANT_INDEX\n", __func__);
			wifi_ant_set(atoi(data[1]), rsp);
		} break;

		/*wifi set cdec to efuse*/
		case WIFICDECEFUSE_INDEX: {
			ENG_LOG("%s(), case:WIFICDECEFUSE_INDEX\n", __func__);
			wifi_cdec_efuse_set(atoi(data[1]), rsp);
		} break;
		/*wifi get cdec from efuse*/
		case WIFICDECEFUSE_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFICDECEFUSE_REQ_INDEX\n", __func__);
			wifi_cdec_efuse_get(rsp);
		} break;

		/* CBANK */
		case WIFICBANK_INDEX: {
			ENG_LOG("%s(), case:WIFICBANK_INDEX\n", __func__);
			wifi_cbank_set(atoi(data[1]), rsp);
		} break;

		/* NETMODE */
		case WIFINETMODE_INDEX: {
			ENG_LOG("%s(), case:WIFINETMODE_INDEX\n", __FUNCTION__);
			wifi_netmode_set(atoi(data[1]), rsp);
		} break;

		/* Decode mode */
		case WIFIDECODEMODE_REQ_INDEX: {
			ENG_LOG("%s(), case:WIFIDECODEMODE_REQ_INDEX", __func__);
			wifi_decode_mode_get(rsp);
		} break;

		case WIFIDECODEMODE_INDEX: {
			ENG_LOG("%s(), case:WIFIDECODEMODE_INDEX", __func__);
			wifi_decode_mode_set(atoi(data[1]), rsp);
		} break;

		//-----------------------------------------------------
		default:
			strcpy(rsp, "can not match the at command");
			return 0;
	}

	// @alvin:
	// Here: I think it will response to pc directly outside
	// this function and should not send to modem again.
	ENG_LOG(" eng_atdiag_rsp   %s \n", rsp);

	return 0;
}

int eng_diag_parse(char *buf, int len, int *num)
{
	int ret = CMD_COMMON;
	MSG_HEAD_T *head_ptr = NULL;
	// remove the start 0x7e and the last 0x7e
	*num = eng_diag_decode7d7e(
				   (unsigned char *)(buf + 1), (len - 2));
	head_ptr = (MSG_HEAD_T *)(buf + 1);
	ENG_LOG("start %s\n", __func__);
	ENG_LOG("%s: cmd=0x%x; subcmd=0x%x\n", __FUNCTION__, head_ptr->type,
		head_ptr->subtype);
	// ENG_LOG("%s: cmd is:%s \n", __FUNCTION__, (buf + DIAG_HEADER_LENGTH + 1));

	switch (head_ptr->type) {
		case DIAG_CMD_AT:
			if (is_ap_at_cmd_need_to_handle(buf, len)) {
				ret = CMD_USER_APCMD;
			} else {
				ret = CMD_COMMON;
			}
			break;
		default:
			ENG_LOG("%s: Default\n", __FUNCTION__);
			ret = CMD_COMMON;
			break;
	}
	ENG_LOG("end %s\n", __func__);
	return ret;
}

int eng_diag_write2pc(struct device *uart, char* diag_data, int r_cnt)
{
	int offset = 0;  // reset the offset
	int w_cnt = 0;

	ENG_LOG("%s() rcnt is : %d\n", r_cnt);
	do {
		w_cnt = uart_fifo_fill(uart, diag_data + offset, r_cnt);

		if (w_cnt < 0) {
			if (errno == EBUSY) {
				ENG_LOG("write ebusy\n");
				k_sleep(59000);
			} else {
				ENG_LOG("eng_vdiag_r no log data write:%d\n", w_cnt);
			}
		}
		ENG_LOG("write success, wcnt : %d\n",w_cnt);
		r_cnt -= w_cnt;
		offset += w_cnt;
		ENG_LOG("check rcnt : %d\n", r_cnt);
	} while (r_cnt > 0);

	ENG_LOG("write finished, return offset : %d\n",offset);
	return offset;
}

int eng_diag_apcmd_hdlr(unsigned char *buf, int len, char *rsp)
{
	int rlen = 0, i = 0;
	char *ptr = NULL;

	ptr = buf + 1 + sizeof(MSG_HEAD_T);

	while (*(ptr + i) != 0x7e) {
		i++;
	}

	*(ptr + i - 1) = '\0';

	ENG_LOG("%s: s_cmd_index: %d \n", __FUNCTION__, s_cmd_index);
	eng_linuxcmd_hdlr(s_cmd_index, ptr, rsp);

	rlen = strlen(rsp);

	ENG_LOG("%s:rlen:%d; %s", __FUNCTION__, rlen, rsp);

	return rlen;
}

int eng_diag_user_handle(struct device *uart, int type, char *buf, int len)
{
	int rlen = 0;
	int extra_len = 0;
	int ret;
	MSG_HEAD_T head;
	char rsp[512];
	int adc_rsp[8];

	ENG_LOG("%s start\n", __func__);
	memset(rsp, 0, sizeof(rsp));
	memset(adc_rsp, 0, sizeof(adc_rsp));

	ENG_LOG("%s: type=%d\n", __FUNCTION__, type);

	switch (type) {
		case CMD_USER_APCMD:
			// For compatible with pc tool: MOBILETEST,
			// send a empty diag framer first.
			//{
			//  char emptyDiag[] = {0x7e, 0x00, 0x00, 0x00, 0x00,
			//                      0x08, 0x00, 0xd5, 0x00, 0x7e};
			//  write(get_ser_diag_fd(), emptyDiag, sizeof(emptyDiag));
			//}
			//just handle at command.
			rlen = eng_diag_apcmd_hdlr(buf, len, rsp);
			break;
		default:
			break;
	}

	memcpy((char *)&head, buf + 1, sizeof(MSG_HEAD_T));
	head.len = sizeof(MSG_HEAD_T) + rlen - extra_len;
	ENG_LOG("%s: head.len=%d\n", __FUNCTION__, head.len);
	eng_diag_buf[0] = 0x7e;

	if (type == CMD_USER_APCMD) {

		head.seq_num = 0;
		head.type = 0x9c;
		head.subtype = 0x00;
	}

	memcpy(eng_diag_buf + 1, &head, sizeof(MSG_HEAD_T));
	memcpy(eng_diag_buf + sizeof(MSG_HEAD_T) + 1, rsp, sizeof(rsp));

	eng_diag_buf[head.len + extra_len + 1] = 0x7e;
	eng_diag_len = head.len + extra_len + 2;
	ENG_LOG("%s: eng_diag_write2pc eng_diag_buf=%s;eng_diag_len:%d !\n",
		__FUNCTION__, eng_diag_buf, eng_diag_len);
	eng_diag_encode7d7e((char *)(eng_diag_buf+1), (eng_diag_len-2), &eng_diag_len);
	eng_diag_buf[eng_diag_len-1] = 0x7e;

	ret = eng_diag_write2pc(uart, eng_diag_buf, eng_diag_len);
	if (ret <= 0) {
		ENG_LOG("%s: eng_diag_write2pc ret=%d !\n", __FUNCTION__, ret);
	}

	ENG_LOG("%s all finished \n",__func__);
	return ret;
}

int eng_diag(struct device *uart, char *buf, int len)
{
	int ret = 0;
	int type, num;
	int retry_time = 0;
	int ret_val = 0;
	char rsp[128];
	MSG_HEAD_T head;
	memset(rsp, 0, sizeof(rsp));

	//parse eng diag type.
	type = eng_diag_parse(buf, len, &num);

	ENG_LOG("%s:write type=%d,num=%d\n", __FUNCTION__, type, num);

	memset(eng_diag_buf, 0, sizeof(eng_diag_buf));

	ret_val = eng_diag_user_handle(uart, type, buf, len - num);
	ENG_LOG("%s:ret_val=%d\n", __FUNCTION__, ret_val);

	if (ret_val) {
		ENG_LOG("start to write OK test tool\n");
		eng_diag_buf[0] = 0x7e;

		sprintf(rsp, "%s", "\r\nOK\r\n");
		head.len = sizeof(MSG_HEAD_T) + strlen("\r\nOK\r\n");
		ENG_LOG("%s: head.len=%d\n", __FUNCTION__, head.len);
		head.seq_num = 0;
		head.type = 0x9c;
		head.subtype = 0x00;
		memcpy(eng_diag_buf + 1, &head, sizeof(MSG_HEAD_T));
		memcpy(eng_diag_buf + sizeof(MSG_HEAD_T) + 1, rsp, strlen(rsp));
		eng_diag_buf[head.len + 1] = 0x7e;
		eng_diag_len = head.len + 2;

		retry_time = 0;  // reset retry time counter
	}

	ret = eng_diag_write2pc(uart, eng_diag_buf, eng_diag_len);
	if (ret <= 0) {
		ENG_LOG("%s: eng_diag_write2pc ret=%d !\n", __FUNCTION__, ret);
	}

	ENG_LOG("%s: ret=%d\n", __FUNCTION__, ret);

	return ret;
}
