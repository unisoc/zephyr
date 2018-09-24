/******************************************************************************
 *
 *  Copyright (C) 2018 Spreadtrum Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
#include <zephyr/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <misc/printk.h>
#include <zephyr.h>

#include <stdarg.h>
#include <ctype.h>

#include "bt_npi.h"
#include "bt_eng.h"
#include "../../../../drivers/bluetooth/unisoc/uki_utlis.h"

#define RESPONSE_DUMP(param) BTD("%s, response dump: %s\n", __FUNCTION__, param);
#define CASE_RETURN_STR(const) case const: return #const;

static BTEUT_TX_ELEMENT g_bteut_tx = {
	.pattern = BT_EUT_TX_PATTERN_DEAFULT_VALUE,
	.channel = BT_EUT_TX_CHANNEL_DEAFULT_VALUE,
	.pkttype = BT_EUT_TX_PKTTYPE_DEAFULT_VALUE,
	.pktlen = BT_EUT_PKTLEN_DEAFULT_VALUE,
	.phy = BTEUT_LE_LEGACY_PHY,
	.txpwr.power_type = BT_EUT_POWER_TYPE_DEAFULT_VALUE,
	.txpwr.power_value = BT_EUT_POWER_VALUE_DEAFULT_VALUE
};
static BTEUT_RX_ELEMENT g_bteut_rx = {
	.pattern = BT_EUT_RX_PATTERN_DEAFULT_VALUE,
	.channel = BT_EUT_RX_CHANNEL_DEAFULT_VALUE,
	.pkttype = BT_EUT_RX_PKTTYPE_DEAFULT_VALUE,
	.phy = BTEUT_LE_LEGACY_PHY,
	.modulation_index = BTEUT_STANDARD_INDEX,
	.rxgain.mode = BT_EUT_RX_RXGAIN_DEAFULT_VALUE,
	.addr = BT_EUT_RX_ADDR_DEAFULT_VALUE
};

struct bt_eut_cmd bt_eut_cmds[] = {
	/* BT TXCH*/
	{ BT_TXCH_REQ_INDEX, ENG_BT_TXCH_REQ },
	{ BT_TXCH_INDEX, ENG_BT_TXCH },

	/* BT RXCH*/
	{ BT_RXCH_REQ_INDEX, ENG_BT_RXCH_REQ },
	{ BT_RXCH_INDEX, ENG_BT_RXCH },

	/* BT TX PATTERN */
	{ BT_TXPATTERN_REQ_INDEX, ENG_BT_TXPATTERN_REQ },
	{ BT_TXPATTERN_INDEX, ENG_BT_TXPATTERN },

	/* BT RX PATTERN */
	{ BT_RXPATTERN_REQ_INDEX, ENG_BT_RXPATTERN_REQ },
	{ BT_RXPATTERN_INDEX, ENG_BT_RXPATTERN },

	/* TXPKTTYPE */
	{ BT_TXPKTTYPE_REQ_INDEX, ENG_BT_TXPKTTYPE_REQ },
	{ BT_TXPKTTYPE_INDEX, ENG_BT_TXPKTTYPE },

	/* RXPKTTYPE */
	{ BT_RXPKTTYPE_REQ_INDEX, ENG_BT_RXPKTTYPE_REQ },
	{ BT_RXPKTTYPE_INDEX, ENG_BT_RXPKTTYPE },

	/* TXPKTLEN */
	{ BT_TXPKTLEN_REQ_INDEX, ENG_BT_TXPKTLEN_REQ },
	{ BT_TXPKTLEN_INDEX, ENG_BT_TXPKTLEN },

	/* TXPWR */
	{ BT_TXPWR_REQ_INDEX, ENG_BT_TXPWR_REQ },
	{ BT_TXPWR_INDEX, ENG_BT_TXPWR },

	/* RX Gain */
	{ BT_RXGAIN_REQ_INDEX, ENG_BT_RXGAIN_REQ },
	{ BT_RXGAIN_INDEX, ENG_BT_RXGAIN },

	/* ADDRESS */
	{ BT_ADDRESS_REQ_INDEX, ENG_BT_ADDRESS_REQ },
	{ BT_ADDRESS_INDEX, ENG_BT_ADDRESS },

	/* RXDATA */
	{ BT_RXDATA_REQ_INDEX, ENG_BT_RXDATA_REQ },

	/* TESTMODE */
	{ BT_TESTMODE_REQ_INDEX, ENG_BT_TESTMODE_REQ },
	{ BT_TESTMODE_INDEX, ENG_BT_TESTMODE },

	/* TX */
	{BT_TX_REQ_INDEX, ENG_BT_TX_REQ},
	{BT_TX_INDEX, ENG_BT_TX},

	/* RX */
	{BT_RX_REQ_INDEX, ENG_BT_RX_REQ},
	{BT_RX_INDEX, ENG_BT_RX},

	/* TXPHYTYPE */
	{ BT_TXPHYTYPE_REQ_INDEX, ENG_BT_TXPHYTYPE_REQ },
	{ BT_TXPHYTYPE_INDEX, ENG_BT_TXPHYTYPE },

	/* RXPHYTYPE */
	{ BT_RXPHYTYPE_REQ_INDEX, ENG_BT_RXPHYTYPE_REQ },
	{ BT_RXPHYTYPE_INDEX, ENG_BT_RXPHYTYPE },

	/* RXMODINDEX */
	{ BT_RXMODINDEX_REQ_INDEX, ENG_BT_RXMODINDEX_REQ },
	{ BT_RXMODINDEXE_INDEX, ENG_BT_RXMODINDEX },

	/* RFPATH */
	{ BT_RFPATH_REQ_INDEX, ENG_BT_RFPATH_REQ },
	{ BT_RFPATH_INDEX, ENG_BT_RFPATH },

};

static bteut_txrx_status g_bteut_txrx_status = BTEUT_TXRX_STATUS_OFF;
static bteut_testmode g_bteut_testmode = BTEUT_TESTMODE_LEAVE;

/* Set to 1 when the Bluedroid stack is enabled */
// static unsigned char g_bteut_bt_enabled = 0;

/* current is Classic or BLE */
static bteut_bt_mode g_bt_mode = BTEUT_BT_MODE_OFF;
static bteut_eut_running g_bteut_runing = BTEUT_EUT_RUNNING_OFF;

static int bteut_rf_path = BTEUT_RFPATH_UNKNOWN;

static char *le_test_legacy_list[] = {"marlin", "marlin2", "pike2", "sharkle", "sharkl3", "sharklep", NULL};
extern char *uki_strtok_r(char *str, const char *delim, char **saveptr);


int bt_testmode_set(bteut_bt_mode bt_mode, bteut_testmode testmode, char *rsp);
int bt_testmode_get(bteut_bt_mode bt_mode, char *rsp);

int bt_address_set(const char *addr, char *rsp);
int bt_address_get(char *rsp);

int bt_channel_set(bteut_cmd_type cmd_type, int ch, char *rsp);
int bt_channel_get(bteut_cmd_type cmd_type, char *rsp);

int bt_pattern_set(bteut_cmd_type cmd_type, int pattern, char *rsp);
int bt_pattern_get(bteut_cmd_type cmd_type, char *rsp);

int bt_pkttype_set(bteut_cmd_type cmd_type, int pkttype, char *rsp);
int bt_pkttype_get(bteut_cmd_type cmd_type, char *rsp);

int bt_txpktlen_set(unsigned int pktlen, char *rsp);
int bt_txpktlen_get(char *rsp);

int bt_txpwr_set(bteut_txpwr_type txpwr_type, unsigned int value, char *rsp);
int bt_txpwr_get(char *rsp);

int bt_rxgain_set(bteut_gain_mode mode, unsigned int value, char *rsp);
int bt_rxgain_get(char *rsp);

int bt_tx_set(int on_off, int tx_mode, unsigned int pktcnt, char *rsp);
int bt_tx_get(char *rsp);

int bt_rx_set(int on_off, char *rsp);
int bt_rx_get(char *rsp);

int bt_rxdata_get(char *rsp);

int bt_phy_set(bteut_cmd_type cmd_type, int phy, char *rsp);
int bt_phy_get(bteut_cmd_type cmd_type, char *rsp);

int bt_rx_mod_index_set(int index, char *rsp);
int bt_rx_mod_index_get(char *rsp);

int bt_prf_path_set(int index, char *rsp);
int bt_prf_path_get(char *rsp);


/**************************Function Definition***************************/
static int skip_atoi(const char **s)
{
	int i = 0;

	while (isdigit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

/**
 * skip_spaces - Removes leading whitespace from @str.
 * @str: The string to be stripped.
 *
 * Returns a pointer to the first non-whitespace character in @str.
 */
static char *skip_spaces(const char *str)
{
	while (isspace(*str))
		++str;
	return (char *)str;
}

/**
 * vsscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	format of buffer
 * @args:	arguments
 */
static int uki_vsscanf(const char *buf, const char *fmt, va_list args)
{
	const char *str = buf;
	char *next;
	char digit;
	int num = 0;
	uint8_t qualifier;
	uint8_t base;
	int16_t field_width;
	int is_sign;

	while (*fmt && *str) {
		/* skip any white space in format */
		/* white space in format matchs any amount of
		 * white space, including none, in the input.
		 */
		if (isspace(*fmt)) {
			fmt = skip_spaces(++fmt);
			str = skip_spaces(str);
		}

		/* anything that is not a conversion must match exactly */
		if (*fmt != '%' && *fmt) {
			if (*fmt++ != *str++)
				break;
			continue;
		}

		if (!*fmt)
			break;
		++fmt;

		/* skip this conversion.
		 * advance both strings to next white space
		 */
		if (*fmt == '*') {
			while (!isspace(*fmt) && *fmt != '%' && *fmt)
				fmt++;
			while (!isspace(*str) && *str)
				str++;
			continue;
		}

		/* get field width */
		field_width = -1;
		if (isdigit(*fmt))
			field_width = skip_atoi(&fmt);

		/* get conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || tolower(*fmt) == 'l' ||
		    tolower(*fmt) == 'z') {
			qualifier = *fmt++;
			if (qualifier == *fmt) {
				if (qualifier == 'h') {
					qualifier = 'H';
					fmt++;
				} else if (qualifier == 'l') {
					qualifier = 'L';
					fmt++;
				}
			}
		}

		if (!*fmt || !*str)
			break;

		base = 10;
		is_sign = 0;

		switch (*fmt++) {
		case 'c':
		{
			char *s = (char *)va_arg(args, char*);
			if (field_width == -1)
				field_width = 1;
			do {
				*s++ = *str++;
			} while (--field_width > 0 && *str);
			num++;
		}
		continue;
		case 's':
		{
			char *s = (char *)va_arg(args, char *);
			if (field_width == -1)
				field_width = SHRT_MAX;
			/* first, skip leading white space in buffer */
			str = skip_spaces(str);

			/* now copy until next white space */
			while (*str && !isspace(*str) && field_width--)
				*s++ = *str++;
			*s = '\0';
			num++;
		}
		continue;
		case 'n':
			/* return number of characters read so far */
		{
			int *i = (int *)va_arg(args, int*);
			*i = str - buf;
		}
		continue;
		case 'o':
			base = 8;
			break;
		case 'x':
		case 'X':
			base = 16;
			break;
		case 'i':
			base = 0;
		case 'd':
			is_sign = 1;
		case 'u':
			break;
		case '%':
			/* looking for '%' in str */
			if (*str++ != '%')
				return num;
			continue;
		default:
			/* invalid format; stop here */
			return num;
		}

		/* have some sort of integer conversion.
		 * first, skip white space in buffer.
		 */
		str = skip_spaces(str);

		digit = *str;
		if (is_sign && digit == '-')
			digit = *(str + 1);

		if (!digit
		    || (base == 16 && !isxdigit(digit))
		    || (base == 10 && !isdigit(digit))
		    || (base == 8 && (!isdigit(digit) || digit > '7'))
		    || (base == 0 && !isdigit(digit)))
			break;

		switch (qualifier) {
		case 'H':	/* that's 'hh' in format */
			if (is_sign) {
				signed char *s = (signed char *)va_arg(args, signed char *);
				*s = (signed char)strtol(str, &next, base);
			} else {
				unsigned char *s = (unsigned char *)va_arg(args, unsigned char *);
				*s = (unsigned char)strtoul(str, &next, base);
			}
			break;
		case 'h':
			if (is_sign) {
				short *s = (short *)va_arg(args, short *);
				*s = (short)strtol(str, &next, base);
			} else {
				unsigned short *s = (unsigned short *)va_arg(args, unsigned short *);
				*s = (unsigned short)strtoul(str, &next, base);
			}
			break;
		case 'l':
			if (is_sign) {
				long *l = (long *)va_arg(args, long *);
				*l = strtol(str, &next, base);
			} else {
				unsigned long *l = (unsigned long *)va_arg(args, unsigned long *);
				*l = strtoul(str, &next, base);
			}
			break;
#if 0
		case 'L':
			if (is_sign) {
				long long *l = (long long *)va_arg(args, long long *);
				*l = simple_strtoll(str, &next, base);
			} else {
				unsigned long long *l = (unsigned long long *)va_arg(args, unsigned long long *);
				*l = simple_strtoull(str, &next, base);
			}
			break;
#endif
		case 'Z':
		case 'z':
		{
			size_t *s = (size_t *)va_arg(args, size_t *);
			*s = (size_t)strtoul(str, &next, base);
		}
		break;
		default:
			if (is_sign) {
				int *i = (int *)va_arg(args, int *);
				*i = (int)strtol(str, &next, base);
			} else {
				unsigned int *i = (unsigned int *)va_arg(args, unsigned int*);
				*i = (unsigned int)strtoul(str, &next, base);
			}
			break;
		}
		num++;

		if (!next)
			break;
		str = next;
	}

	/*
	 * Now we've come all the way through so either the input string or the
	 * format ended. In the former case, there can be a %n at the current
	 * position in the format that needs to be filled.
	 */
	if (*fmt == '%' && *(fmt + 1) == 'n') {
		int *p = (int *)va_arg(args, int *);
		*p = str - buf;
	}

	return num;
}

/**
 * sscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	formatting of buffer
 * @...:	resulting arguments
 */
int uki_sscanf(const char *buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = uki_vsscanf(buf, fmt, args);
	va_end(args);

	return i;
}

static void bt_build_err_resp(char *resp) {
	snprintf(resp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%s",
			(BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_ERROR : EUT_BT_ERROR), "error");
}

static int le_test_mode_inuse(void)
{
	int i = 0;
	char property_buf[128] = "marlin3";

	for (;; i++) {
		if (le_test_legacy_list[i] == NULL)
			break;
		if (!strcmp(le_test_legacy_list[i], property_buf)) {
			BTD("legacy device found: %s\n", le_test_legacy_list[i]);
			return 0;
		}
	}

	return 1;
}

static int get_sprd_sub_str(const char *buf, char **revdata, char a, char *delim, unsigned char count,
			unsigned char substr_max_len) {
	int len;
	char *start = NULL, *substr = NULL, *end = (char *)buf;

	start = strchr(buf, a);
	substr = strstr(buf, delim);

	if (!substr) {
		/* if current1 not exist, return this function.*/
		return 0;
	}

	while (end && *end != '\0') {
		end++;
	}

	if ((NULL != start) && (NULL != end)) {
		char *tokenPtr = NULL;
		char *outer_ptr = NULL;
		unsigned int index = 1; /*must be inited by 1, because data[0] is command name */

		start++;
		substr++;
		len = substr - start - 1;

		/* get cmd name */
		memcpy(revdata[0], start, len);

		/* get sub str by delimeter */
		tokenPtr = uki_strtok_r(substr, delim, &outer_ptr);
		while (NULL != tokenPtr && index < count) {
			strncpy(revdata[index++], tokenPtr, substr_max_len);

			/* next */
			tokenPtr = uki_strtok_r(NULL, delim, &outer_ptr);
		}
	}

	return 0;
}

static int get_sprd_cmd_index(char *buf) {
	int i = 0;
	int index = -1;
	char *start = NULL;
	char *cur = NULL;
	int str_len = 0;
	char name_str[64 + 1] = { 0x00 };

	start = strchr(buf, '=');
	if (NULL == start) {
		BTE("expected '=', but argument is: %s\n", buf);
		return -1;
	}

	/* search ',' '?' '\r' */
	/* skip '=' */
	start++;

	if (NULL != (cur = strchr(buf, '?'))) {
		cur++; /* include '?' to name_str */
		str_len = cur - start;
		strncpy(name_str, (char *)start, str_len);
	} else if (NULL != (cur = strchr(buf, ',')) || NULL != (cur = strchr(buf, '\r'))) {
		str_len = cur - start;
		strncpy(name_str, (char *)start, str_len);
	} else {
		BTE("expected paramters, but argument is: %s\n", buf);
		return -1;
	}

	for (i = 0; i < (int)NUM_ELEMS(bt_eut_cmds); i++) {
		if (0 == strcmp(name_str, bt_eut_cmds[i].name)) {
			index = bt_eut_cmds[i].index;
			break;
		}
	}

	return index;
}

static const char* dump_module_index(int module_index) {
	switch (module_index) {
		case EUT_BT_MODULE_INDEX: return "Classic";
		case EUT_WIFI_MODULE_INDEX: return "WIFI";
		case EUT_GPS_MODULE_INDEX: return "GPS";
		case EUT_BLE_MODULE_INDEX: return "BLE";
	default:
		return "unknown module index";
	}
}

static const char* dump_cmd_index(int cmd_index) {
	switch (cmd_index) {
		CASE_RETURN_STR(BT_TX_REQ_INDEX)
		CASE_RETURN_STR(BT_TX_INDEX)
		CASE_RETURN_STR(BT_RX_REQ_INDEX)
		CASE_RETURN_STR(BT_RX_INDEX)
		CASE_RETURN_STR(BT_TXCH_REQ_INDEX)
		CASE_RETURN_STR(BT_TXCH_INDEX)
		CASE_RETURN_STR(BT_RXCH_REQ_INDEX)
		CASE_RETURN_STR(BT_RXCH_INDEX)
		CASE_RETURN_STR(BT_TXPATTERN_REQ_INDEX)
		CASE_RETURN_STR(BT_TXPATTERN_INDEX)
		CASE_RETURN_STR(BT_RXPATTERN_REQ_INDEX)
		CASE_RETURN_STR(BT_RXPATTERN_INDEX)
		CASE_RETURN_STR(BT_TXPKTTYPE_REQ_INDEX)
		CASE_RETURN_STR(BT_TXPKTTYPE_INDEX)
		CASE_RETURN_STR(BT_RXPKTTYPE_REQ_INDEX)
		CASE_RETURN_STR(BT_RXPKTTYPE_INDEX)
		CASE_RETURN_STR(BT_TXPKTLEN_REQ_INDEX)
		CASE_RETURN_STR(BT_TXPKTLEN_INDEX)
		CASE_RETURN_STR(BT_TXPWR_REQ_INDEX)
		CASE_RETURN_STR(BT_TXPWR_INDEX)
		CASE_RETURN_STR(BT_RXGAIN_REQ_INDEX)
		CASE_RETURN_STR(BT_RXGAIN_INDEX)
		CASE_RETURN_STR(BT_ADDRESS_REQ_INDEX)
		CASE_RETURN_STR(BT_ADDRESS_INDEX)
		CASE_RETURN_STR(BT_RXDATA_REQ_INDEX)
		CASE_RETURN_STR(BT_TESTMODE_REQ_INDEX)
		CASE_RETURN_STR(BT_TESTMODE_INDEX)

	default:
		return "unknown cmd";
	}
}

static int cmdline_handle(char *dst, char *src)
{
	int ret = strlen(src);

	if (ret <= 0)
		return -1;

	memcpy(dst, src, ret);

	dst[ret] = 0;

	if (dst[ret - 1] == '\n') {
		dst[ret - 1] = 0;
		ret--;
	}

	if (dst[ret - 1] == '\r') {
		dst[ret - 1] = 0;
		ret--;
	}
	return ret;
}

void bt_npi_parse(int module_index, char *buf, char *rsp) {
	// spreadtrum
	char args0[33] = {0}, args1[33] = {0}, args2[33] = {0}, args3[33] = {0};
	char *data[4] = {args0, args1, args2, args3};
	static char cmdline[1024];
	int cmd_index = -1;

	if (cmdline_handle(cmdline, buf) <= 0) {
		BTD("Got Cmdline Error: %s\n", buf);
		SNPRINT(rsp, "cmdline error");
		return;
	}

	BTD("%s, cmdline: %s\n", dump_module_index(module_index), cmdline);

	if (EUT_BT_MODULE_INDEX != module_index && EUT_BLE_MODULE_INDEX != module_index) {
		BTD("Unknow Module Index for Bluetooth NPI Test, module_index: %d\n", module_index);
		SNPRINT(rsp, "can not match TheClassic/BLE Test Founction");
		return;
	}

	get_sprd_sub_str(cmdline, data, '=', ",", 4, 32);
	cmd_index = get_sprd_cmd_index(cmdline);

	if (cmd_index < 0) {
		BTD("Unknow CMD for Bluetooth NPI Test, cmd_index: %d\n", cmd_index);
		SNPRINT(rsp, "can not match TheClassic/BLE Test Command");
		return;
	}

	BTD("dump: args0: %s, args1: %s, args2: %s, args3: %s, cmd: %s\n",
			args0, args1, args2, args3, dump_cmd_index(cmd_index));

	switch (cmd_index) {
		case BT_TX_INDEX: {
			bt_tx_set(atoi(data[1]), atoi(data[2]), strlen(data[3]) ? atoi(data[3]) : 0 , rsp);
		} break;

		case BT_RX_INDEX: {
			bt_rx_set(atoi(data[1]), rsp);
		} break;

		case BT_TX_REQ_INDEX: {
			bt_tx_get(rsp);
		} break;

		case BT_RX_REQ_INDEX: {
			bt_rx_get(rsp);
		} break;

		/* TX Channel */
		case BT_TXCH_REQ_INDEX: {
			bt_channel_get(BTEUT_CMD_TYPE_TX, rsp);
		} break;

		case BT_TXCH_INDEX: {
			bt_channel_set(BTEUT_CMD_TYPE_TX, atoi(data[1]), rsp);
		} break;

		/* RX Channel */
		case BT_RXCH_REQ_INDEX: {
			bt_channel_get(BTEUT_CMD_TYPE_RX, rsp);
		} break;

		case BT_RXCH_INDEX: {
			bt_channel_set(BTEUT_CMD_TYPE_RX, atoi(data[1]), rsp);
		} break;

		/* Pattern */
		case BT_TXPATTERN_REQ_INDEX: {
			bt_pattern_get(BTEUT_CMD_TYPE_TX, rsp);
		} break;

		case BT_TXPATTERN_INDEX: {
			bt_pattern_set(BTEUT_CMD_TYPE_TX, atoi(data[1]), rsp);
		} break;

		case BT_RXPATTERN_REQ_INDEX: {
			bt_pattern_get(BTEUT_CMD_TYPE_RX, rsp);
		} break;

		case BT_RXPATTERN_INDEX: {
			bt_pattern_set(BTEUT_CMD_TYPE_RX, atoi(data[1]), rsp);
		} break;

		/* PKT Type */
		case BT_TXPKTTYPE_REQ_INDEX: {
			bt_pkttype_get(BTEUT_CMD_TYPE_TX, rsp);
		} break;

		case BT_TXPKTTYPE_INDEX: {
			bt_pkttype_set(BTEUT_CMD_TYPE_TX, atoi(data[1]), rsp);
		} break;

		case BT_RXPKTTYPE_REQ_INDEX: {
			bt_pkttype_get(BTEUT_CMD_TYPE_RX, rsp);
		} break;

		case BT_RXPKTTYPE_INDEX: {
			bt_pkttype_set(BTEUT_CMD_TYPE_RX, atoi(data[1]), rsp);
		} break;

		/* PHY Type */
		case BT_TXPHYTYPE_REQ_INDEX: {
			bt_phy_get(BTEUT_CMD_TYPE_TX, rsp);
		} break;

		case BT_TXPHYTYPE_INDEX: {
			bt_phy_set(BTEUT_CMD_TYPE_TX, atoi(data[1]), rsp);
		} break;

		case BT_RXPHYTYPE_REQ_INDEX: {
			bt_phy_get(BTEUT_CMD_TYPE_RX, rsp);
		} break;

		case BT_RXPHYTYPE_INDEX: {
			bt_phy_set(BTEUT_CMD_TYPE_RX, atoi(data[1]), rsp);
		} break;

		/* PKT Length */
		case BT_TXPKTLEN_REQ_INDEX: {
			bt_txpktlen_get(rsp);
		} break;

		case BT_TXPKTLEN_INDEX: {
			bt_txpktlen_set(atoi(data[1]), rsp);
		} break;

		/* TX Power */
		case BT_TXPWR_REQ_INDEX: {
			bt_txpwr_get(rsp);
		} break;

		case BT_TXPWR_INDEX: {
			bt_txpwr_set(atoi(data[1]), atoi(data[2]), rsp);
		} break;

		/* RX Gain */
		case BT_RXGAIN_REQ_INDEX: {
			bt_rxgain_get(rsp);
		} break;

		case BT_RXGAIN_INDEX: {
			bt_rxgain_set(atoi(data[1]), atoi(data[2]), rsp);
		} break;

		/* ADDRESS */
		case BT_ADDRESS_REQ_INDEX: {
			bt_address_get(rsp);
		} break;

		case BT_ADDRESS_INDEX: {
			bt_address_set(data[1], rsp);
		} break;

		/* RX received data */
		case BT_RXDATA_REQ_INDEX: {
			bt_rxdata_get(rsp);
		} break;

		/* Testmode */
		case BT_TESTMODE_REQ_INDEX: {
			if (EUT_BT_MODULE_INDEX == module_index) {
				bt_testmode_get(BTEUT_BT_MODE_CLASSIC, rsp);
			} else if (EUT_BLE_MODULE_INDEX == module_index) {
				bt_testmode_get(BTEUT_BT_MODE_BLE, rsp);
			}
		} break;

		/* Testmode */
		case BT_TESTMODE_INDEX: {
			if (EUT_BT_MODULE_INDEX == module_index) {
				bt_testmode_set(BTEUT_BT_MODE_CLASSIC, atoi(data[1]), rsp);
			} else if (EUT_BLE_MODULE_INDEX == module_index) {
				bt_testmode_set(BTEUT_BT_MODE_BLE, atoi(data[1]), rsp);
			}
		} break;

		case BT_RXMODINDEXE_INDEX: {
			bt_rx_mod_index_set(atoi(data[1]), rsp);
		} break;

		case BT_RXMODINDEX_REQ_INDEX: {
			bt_rx_mod_index_get(rsp);
		} break;

		case BT_RFPATH_INDEX: {
			bt_prf_path_set(atoi(data[1]), rsp);
		} break;

		case BT_RFPATH_REQ_INDEX: {
			bt_prf_path_get(rsp);
		} break;

		//-----------------------------------------------------
		default:
			BTD("can not match the at command: %d\n", cmd_index);
			SNPRINT(rsp, "can not match the at command");
			return;
	}
}

/********************************************************************
*   name   bt_str2bd
*   ---------------------------
*   descrition: covert string address to hex format
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
static int bt_str2bd(const char *str, bt_bdaddr_t *addr) {
	unsigned char i = 0;

	for (i = 0; i < 6; i++) {
		addr->address[i] = (unsigned char)strtoul(str, (char **)&str, 16);
		str++;
	}

	return 0;
}

/********************************************************************
*   name   bt_testmode_set
*   ---------------------------
*   descrition: set rx's address to global variable
*   ----------------------------
*   para        IN/OUT      type            note
*   bt_mode     IN          bteut_bt_mode   the mode is BT or BLE
*   testmode    IN          bteut_testmode  test mode
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
int bt_testmode_set(bteut_bt_mode bt_mode, bteut_testmode testmode, char *rsp) {
	int ret = -1;
	testmode = testmode+1;

	BTD("ADL entry %s(), bt_mode = %d, g_bt_mode = %d, testmode = %d, "
			"g_bteut_testmode = %d\n",
			__func__, bt_mode, g_bt_mode, testmode, g_bteut_testmode);

	if(BTEUT_TESTMODE_LEAVE != testmode && BTEUT_TESTMODE_LEAVE != g_bteut_testmode){
		BTD("%s(), has been in testmode\n", __func__);
		goto err;
	}

	switch (testmode) {
		case BTEUT_TESTMODE_LEAVE: {
			BTD("ADL %s(), case BTEUT_TESTMODE_LEAVE:\n", __func__);

			if (BTEUT_TESTMODE_ENTER_NONSIG == g_bteut_testmode) {
				if (BTEUT_BT_MODE_CLASSIC != g_bt_mode && BTEUT_BT_MODE_BLE != g_bt_mode) {
					/* is not BT_MODE_OFF, is error */
					BTD("ADL %s(), g_bt_mode is ERROR, g_bt_mode = %d\n", __func__, g_bt_mode);
					goto err;
				}

				if (BTEUT_TESTMODE_LEAVE != g_bteut_testmode) {
					if (BTEUT_TXRX_STATUS_TXING == g_bteut_txrx_status ||
						BTEUT_TXRX_STATUS_RXING == g_bteut_txrx_status) {
						BTD("ADL %s(), txrx_status is ERROR, txrx_status = %d\n", __func__, g_bteut_txrx_status);
						goto err;
					}
				}

			} else if (BTEUT_TESTMODE_ENTER_EUT == g_bteut_testmode) {
				ret = engpc_bt_dut_mode_configure(0);
				BTD("ADL %s(), case BTEUT_TESTMODE_LEAVE: called dut_mode_configure(), ret = %d\n",
					__func__, ret);

				if (0 != ret) {
					BTD("ADL %s(), case BTEUT_TESTMODE_LEAVE: called "
							"dut_mode_configure(), ret = %d, goto err\n",
							__func__, ret);
					goto err;
				}

				BTD("ADL %s(), case BTEUT_TESTMODE_LEAVE: set g_bteut_runing to "
						"RUNNING_OFF\n",
						__func__);
				g_bteut_runing = BTEUT_EUT_RUNNING_OFF;
			}

			engpc_bt_off();

			BTD("ADL %s(), case BTEUT_TESTMODE_LEAVE: set g_bteut_testmode to "
					"BTEUT_TESTMODE_LEAVE\n",
					__func__);
			g_bteut_testmode = BTEUT_TESTMODE_LEAVE;
		} break;

		case BTEUT_TESTMODE_ENTER_EUT: {
			BTD("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT:\n", __func__);

			if (BTEUT_TESTMODE_ENTER_EUT != g_bteut_testmode) {
				g_bteut_testmode = BTEUT_TESTMODE_ENTER_EUT;

				ret = engpc_bt_on();
				if (0 != ret) {
					BTD("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG: called "
							"engpc_bt_on is error, goto err\n",
							__func__);
					goto err;
				}

				ret = engpc_bt_dut_mode_configure(1);
				BTD("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT: called "
						"dut_mode_configure(1), ret = %d\n",
						__func__, ret);

				if (0 != ret) {
					BTD("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT: called "
							"dut_mode_configure(1), ret = %d, goto err\n",
							__func__, ret);
					goto err;
				}

				BTD("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT: set g_bteut_runing to "
						"BTEUT_EUT_RUNNING_ON\n",
						__func__);
				g_bteut_runing = BTEUT_EUT_RUNNING_ON;
			} else {
				BTD("ADL %s(), case BTEUT_TESTMODE_ENTER_EUT: now is ENTER_EUT, error.\n",
						__func__);
				goto err;
			}
		} break;

		case BTEUT_TESTMODE_ENTER_NONSIG: {
			BTD("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG:\n", __func__);

			if (BTEUT_TESTMODE_ENTER_NONSIG != g_bteut_testmode) {
				g_bteut_testmode = BTEUT_TESTMODE_ENTER_NONSIG;
			} else {
				BTD("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG: now is ENTER NONSIG, "
						"error.\n",
						__func__);
				goto err;
			}
			BTD("calling engpc_bt_on\n");
			ret = engpc_bt_on();
			if (0 != ret) {
				BTD("ADL %s(), case BTEUT_TESTMODE_ENTER_NONSIG: called engpc_bt_on() "
						"is error, goto err\n",
						__func__);
				goto err;
			}
		} break;

		default:
			BTD("ADL %s(), case default\n", __func__);
	}

	BTD("ADL %s(), set bt_mode is %d\n", __func__, bt_mode);
	g_bt_mode = bt_mode; /* is CLASSIC OR BLE */

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return 0\n", __func__, rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return -1\n", __func__, rsp);
	return -1;
}

/********************************************************************
*   name   bt_testmode_get
*   ---------------------------
*   descrition: get testmode's value from global variable
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
int bt_testmode_get(bteut_bt_mode bt_mode, char *rsp) {
	BTD("ADL entry %s(), bt_mode = %d\n", __func__, bt_mode);

	BTD("ADL %s(), set g_bt_mode to %d\n", __func__, bt_mode);
	g_bt_mode = bt_mode;

	snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
			(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TESTMODE_REQ_RET : BT_TESTMODE_REQ_RET),
			(int)g_bteut_testmode-1);

	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s\n", __func__, rsp);
	return 0;
}

/********************************************************************
*   name   bt_address_set
*   ---------------------------
*   descrition: set rx's address to global variable
*   ----------------------------
*   para        IN/OUT      type            note
*   addr        IN          const char *    MAC Address
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_address_set(const char *addr, char *rsp) {
	BTD("ADL entry %s(), addr = %s\n", __func__, addr);

	if ('\"' == *addr) {
		/* skip \" */
		addr++;
	}

	strncpy(g_bteut_rx.addr, addr, BT_MAC_STR_MAX_LEN);
	BTD("ADL %s(), addr = %s\n", __func__, g_bteut_rx.addr);

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return 0\n", __func__, rsp);
	return 0;
}

/********************************************************************
*   name   bt_channel_get
*   ---------------------------
*   descrition: get rx's address from g_bteut_rx
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_address_get(char *rsp) {
	BTD("ADL entry %s()\n", __func__);

	snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%s",
			(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_ADDRESS_REQ_RET : BT_ADDRESS_REQ_RET),
			g_bteut_rx.addr);

	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s\n", __func__, rsp);
	return 0;
}

/********************************************************************
*   name   bt_channel_set
*   ---------------------------
*   descrition: set channel to global variable
*   ----------------------------
*   para        IN/OUT      type            note
*   cmd_type    IN          bteut_cmd_type  the command is TX or RX
*   ch          IN          int             channel
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/rx, whether is not BT Chip
*
********************************************************************/
int bt_channel_set(bteut_cmd_type cmd_type, int ch, char *rsp) {
	if (BTEUT_CMD_TYPE_TX == cmd_type) {
		g_bteut_tx.channel = ch;
	} else if (BTEUT_CMD_TYPE_RX == cmd_type) {
		g_bteut_rx.channel = ch;
	} else {
		BTE("unknow transmission orientation: %d\n", cmd_type);
		goto err;
	}

	BTD("%s, Channle: %d\n", cmd_type == BTEUT_CMD_TYPE_TX ? "TX" : "RX" , ch);

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);

	RESPONSE_DUMP(rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("Failure, Response: %s\n", rsp);
	return -1;
}

/********************************************************************
*   name   bt_channel_get
*   ---------------------------
*   descrition: get channel from g_bteut_tx/g_bteut_rx
*   ----------------------------
*   para        IN/OUT      type            note
*   cmd_type    IN          bteut_cmd_type  the command is TX or RX
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_channel_get(bteut_cmd_type cmd_type, char *rsp) {
	if (BTEUT_CMD_TYPE_TX == cmd_type) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TX_CHANNEL_REQ_RET : BT_TX_CHANNEL_REQ_RET),
				g_bteut_tx.channel);
	} else if (BTEUT_CMD_TYPE_RX == cmd_type) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RX_CHANNEL_REQ_RET : BT_RX_CHANNEL_REQ_RET),
				g_bteut_rx.channel);
	} else {
		BTE("unknow transmission orientation: %d\n", cmd_type);
		goto err;
	}

	RESPONSE_DUMP(rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("Failure, Response: %s\n", rsp);
	return -1;
}

/********************************************************************
*   name   bt_pattern_set
*   ---------------------------
*   descrition: set pattern to global variable
*   ----------------------------
*   para        IN/OUT      type                note
*   cmd_type    IN          cmd_type            the command is TX or RX
*   pattern     IN          int                 pattern
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/rx, whether is not BT Chip
*
********************************************************************/
int bt_pattern_set(bteut_cmd_type cmd_type, int pattern, char *rsp) {
	if (BTEUT_CMD_TYPE_TX == cmd_type) {
		g_bteut_tx.pattern = pattern;
	} else if (BTEUT_CMD_TYPE_RX == cmd_type) {
		g_bteut_rx.pattern = pattern;
	} else {
		BTE("unknow transmission orientation: %d\n", cmd_type);
		goto err;
	}

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);

	RESPONSE_DUMP(rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("Failure, Response: %s\n", rsp);
	return -1;
}

/********************************************************************
*   name   bt_pattern_get
*   ---------------------------
*   descrition: get pattern from g_bteut_tx/g_bteut_rx
*   ----------------------------
*   para        IN/OUT      type            note
*   cmd_type    IN          cmd_type        the command is TX or RX
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_pattern_get(bteut_cmd_type cmd_type, char *rsp) {
	if (BTEUT_CMD_TYPE_TX == cmd_type) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TX_PATTERN_REQ_RET : BT_TX_PATTERN_REQ_RET),
				g_bteut_tx.pattern);
	} else if (BTEUT_CMD_TYPE_RX == cmd_type) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RX_PATTERN_REQ_RET : BT_RX_PATTERN_REQ_RET),
				g_bteut_rx.pattern);
	} else {
		BTE("unknow transmission orientation: %d\n", cmd_type);
		goto err;
	}

	RESPONSE_DUMP(rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("Failure, Response: %s\n", rsp);
	return -1;
}

/********************************************************************
*   name   bt_pkttype_set
*   ---------------------------
*   descrition: set pkttype to global variable
*   ----------------------------
*   para        IN/OUT      type                note
*   cmd_type    IN          cmd_type            the command is TX or RX
*   pkttype     IN          int                 pkttype
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/rx, whether is not BT Chip
*
********************************************************************/
int bt_pkttype_set(bteut_cmd_type cmd_type, int pkttype, char *rsp) {
	if (BTEUT_CMD_TYPE_TX == cmd_type) {
		g_bteut_tx.pkttype = pkttype;
	} else if (BTEUT_CMD_TYPE_RX == cmd_type) {
		g_bteut_rx.pkttype = pkttype;
	} else {
		BTE("unknow transmission orientation: %d\n", cmd_type);
		goto err;
	}

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);

	RESPONSE_DUMP(rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("Failure, Response: %s\n", rsp);
	return -1;
}

/********************************************************************
*   name   bt_pkttype_get
*   ---------------------------
*   descrition: get pkttype from g_bteut_tx/g_bteut_rx
*   ----------------------------
*   para        IN/OUT      type            note
*   cmd_type    IN          cmd_type        the command is TX or RX
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx/g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_pkttype_get(bteut_cmd_type cmd_type, char *rsp) {
	if (BTEUT_CMD_TYPE_TX == cmd_type) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TX_PKTTYPE_REQ_RET : BT_TX_PKTTYPE_REQ_RET),
				(int)g_bteut_tx.pkttype);
	} else if (BTEUT_CMD_TYPE_RX == cmd_type) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RX_PKTTYPE_REQ_RET : BT_RX_PKTTYPE_REQ_RET),
				(int)g_bteut_rx.pkttype);
	} else {
		BTE("unknow transmission orientation: %d\n", cmd_type);
		goto err;
	}

	RESPONSE_DUMP(rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("Failure, Response: %s\n", rsp);
	return -1;
}

/********************************************************************
*   name   bt_txpktlen_set
*   ---------------------------
*   descrition: set pktlen to global variable
*   ----------------------------
*   para        IN/OUT      type                note
*   pktlen      IN          int                 pktlen
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx, whether is not BT Chip
*
********************************************************************/
int bt_txpktlen_set(unsigned int pktlen, char *rsp) {
	g_bteut_tx.pktlen = pktlen;

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);

	RESPONSE_DUMP(rsp);
	return 0;
}

/********************************************************************
*   name   bt_txpktlen_get
*   ---------------------------
*   descrition: get pkttype from g_bteut_tx
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx, whether is not BT Chip
*
********************************************************************/
int bt_txpktlen_get(char *rsp) {
	snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
			(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TXPKTLEN_REQ_RET : BT_TXPKTLEN_REQ_RET),
			g_bteut_tx.pktlen);

	RESPONSE_DUMP(rsp);
	return 0;
}

/********************************************************************
*   name   bt_txpwr_set
*   ---------------------------
*   descrition: set txpwr to global variable
*   ----------------------------
*   para        IN/OUT      type                note
*   txpwr       IN          int                 txpwr
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx, whether is not BT Chip
*
********************************************************************/
int bt_txpwr_set(bteut_txpwr_type txpwr_type, unsigned int value, char *rsp) {
	int ret = -1;

	BTD("txpwr_type = %d, value = %d\n", (int)txpwr_type, value);
	BTD("g_bteut_testmode = %d, g_bteut_runing = %d\n", (int)g_bteut_testmode,
			(int)g_bteut_runing);

	g_bteut_tx.txpwr.power_type = txpwr_type;
	g_bteut_tx.txpwr.power_value = value;

	if (BTEUT_TESTMODE_ENTER_EUT == g_bteut_testmode && BTEUT_EUT_RUNNING_ON == g_bteut_runing) {
		unsigned char buf[3] = { 0x00 };
		buf[0] = (unsigned char)txpwr_type;
		buf[1] = (unsigned char)(value & 0x00FF);
		buf[2] = (unsigned char)((value & 0xFF00) >> 8);

		ret = engpc_bt_dut_mode_send(HCI_DUT_SET_TXPWR, buf, 3);

		if (0 != ret) {
			BTD("ADL %s(), call dut_mode_send() is ERROR, goto err\n", __func__);
			goto err;
		}
	}

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);
	RESPONSE_DUMP(rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);
	BTD("Failure, Response: %s\n", rsp);
	return -1;
}

/********************************************************************
*   name   bt_txpwr_get
*   ---------------------------
*   descrition: get txpwr type and value from g_bteut_tx
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx, whether is not BT Chip
*
********************************************************************/
int bt_txpwr_get(char *rsp) {
	snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%d",
			(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TXPWR_REQ_RET : BT_TXPWR_REQ_RET),
			(int)g_bteut_tx.txpwr.power_type, g_bteut_tx.txpwr.power_value);

	RESPONSE_DUMP(rsp);
	return 0;
}

/********************************************************************
*   name   bt_rxgain_set
*   ---------------------------
*   descrition: set rx gain to global variable
*   ----------------------------
*   para        IN/OUT      type                note
*   mode        IN          bteut_gain_mode     RX Gain's mode
*   value       IN          unsigned int        value
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_tx, whether is not BT Chip
*
********************************************************************/
int bt_rxgain_set(bteut_gain_mode mode, unsigned int value, char *rsp) {
	int ret = -1;

	BTD("ADL entry %s(), rxgain_mode = %d, value = %d\n", __func__, (int)mode, value);

	g_bteut_rx.rxgain.mode = mode;
	if (BTEUT_GAIN_MODE_FIX == mode) {
		g_bteut_rx.rxgain.value = value;
	} else if (BTEUT_GAIN_MODE_AUTO == mode) {
		g_bteut_rx.rxgain.value = 0; /* set to 0 */
	} else {
		BTE("unknow rx gain mode: %d\n", mode);
		goto err;
	}

	BTD("ADL %s(), g_bteut_testmode = %d, g_bteut_runing = %d, value = %d\n", __func__,
			(int)g_bteut_testmode, (int)g_bteut_runing, g_bteut_rx.rxgain.value);
	if (BTEUT_TESTMODE_ENTER_EUT == g_bteut_testmode && BTEUT_EUT_RUNNING_ON == g_bteut_runing) {
		unsigned char buf[1] = { 0x00 };

		buf[0] = (unsigned char)g_bteut_rx.rxgain.value;
		ret = engpc_bt_dut_mode_send(HCI_DUT_SET_RXGIAN, buf, 1);
		BTD("ADL %s(), callED dut_mode_send(HCI_DUT_SET_RXGIAN), ret = %d\n", __func__, ret);

		if (0 != ret) {
			BTD("ADL %s(), call dut_mode_send() is ERROR, goto err\n", __func__);
			goto err;
		}
	}

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return 0\n", __func__, rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return -1\n", __func__, rsp);
	return -1;
}

/********************************************************************
*   name   bt_rxgain_get
*   ---------------------------
*   descrition: get rx gain from g_bteut_rx
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*   channel saved in g_bteut_rx, whether is not BT Chip
*
********************************************************************/
int bt_rxgain_get(char *rsp) {
	bteut_gain_mode mode = BTEUT_GAIN_MODE_INVALID;
	mode = g_bteut_rx.rxgain.mode;

	BTD("ADL entry %s(), mode = %d\n", __func__, mode);

	if (BTEUT_GAIN_MODE_FIX == mode) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RXGAIN_REQ_RET : BT_RXGAIN_REQ_RET), mode,
				g_bteut_rx.rxgain.value);
	} else if (BTEUT_GAIN_MODE_AUTO == mode) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RXGAIN_REQ_RET : BT_RXGAIN_REQ_RET), mode);
	} else {
		BTE("unknow rx gain mode: %d\n", mode);
		goto err;
	}

	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s\n", __func__, rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return -1\n", __func__, rsp);
	return -1;
}

int bt_phy_set(bteut_cmd_type cmd_type, int phy, char *rsp) {
	if (BTEUT_CMD_TYPE_TX == cmd_type) {
		g_bteut_tx.phy = phy;
	} else if (BTEUT_CMD_TYPE_RX == cmd_type) {
		g_bteut_rx.phy = phy;
	} else {
		BTE("unknow transmission orientation: %d\n", cmd_type);
		goto err;
	}

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);

	RESPONSE_DUMP(rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("Failure, Response: %s\n", rsp);
	return -1;
}

int bt_phy_get(bteut_cmd_type cmd_type, char *rsp) {
	if (BTEUT_CMD_TYPE_TX == cmd_type) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
			BLE_TX_PHY_REQ_RET,
			g_bteut_tx.phy);
	} else if (BTEUT_CMD_TYPE_RX == cmd_type) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
			BLE_RX_PHY_REQ_RET,
			g_bteut_rx.phy);
	} else {
		BTE("unknow transmission orientation: %d\n", cmd_type);
		goto err;
	}

	RESPONSE_DUMP(rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("Failure, Response: %s\n", rsp);
	return -1;
}

int bt_rx_mod_index_set(int index, char *rsp) {
	g_bteut_rx.modulation_index = index;

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);

	RESPONSE_DUMP(rsp);
	return 0;
}

int bt_rx_mod_index_get(char *rsp) {
	snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
		BLE_RXMODINDEX_REQ_RET,
		g_bteut_rx.modulation_index);
	RESPONSE_DUMP(rsp);
	return 0;
}

int bt_prf_path_set(int index, char *rsp) {
	bteut_rf_path = index;

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);

	engpc_bt_set_rf_path(bteut_rf_path);

	RESPONSE_DUMP(rsp);
	return 0;
}

int bt_prf_path_get(char *rsp) {

	bteut_rf_path = engpc_bt_get_rf_path();

	snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
		BLE_RFPATH_REQ_RET,
		bteut_rf_path);

	RESPONSE_DUMP(rsp);
	return 0;
}

/********************************************************************
*   name   bt_tx_set
*   ---------------------------
*   descrition: set TX start or TX stop to Chip
*   ----------------------------
*   para        IN/OUT      type                note
*   on_off      IN          int                 1:start TX 0:Stop TX
*   tx_mode     IN          bteut_tx_mode       continues or single
*   pktcnt      IN          unsigned int        [options] send package count
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int bt_tx_set(int on_off, int instru_tx_mode, unsigned int pktcnt, char *rsp) {
	int ret = -1;
	char is_ble = 0;
	char instru_type = (char)(instru_tx_mode >> 8);

	BTD("on_off = %d, tx_mode = %d, pktcnt = %d, g_bt_mode = %d\n",
			on_off, (int)instru_tx_mode, pktcnt, g_bt_mode);

	if (BTEUT_BT_MODE_CLASSIC == g_bt_mode) {
		is_ble = 0;
	} else if (BTEUT_BT_MODE_BLE == g_bt_mode) {
		is_ble = 1;
	} else if (BTEUT_BT_MODE_OFF == g_bt_mode) {
		BTD("g_bt_mode is ERROR, g_bt_mode = %d, goto err;\n", g_bt_mode);
		goto err;
	}

	BTD("instru_type = %x\n", instru_type);
	g_bteut_tx.pkttype |= (int)instru_type << 8;

	BTD("on_off = %d, is_ble = %d\n", on_off, is_ble);
	if (0 == on_off) {
		if (BTEUT_TXRX_STATUS_TXING != g_bteut_txrx_status) {
			BTD("ADL %s(), g_bteut_status is ERROR, g_bteut_txrx_status = %d\n", __func__,
				g_bteut_txrx_status);
			goto err;
		}

		BTD("ADL %s(), call set_nonsig_tx_testmode(), enable = 0, is_ble = %d, "
				"g_bt_mode = %d, the rest of other parameters all 0.\n",
				__func__, is_ble, g_bt_mode);

		if (le_test_mode_inuse() && is_ble) {
			ret = engpc_bt_le_test_end();
		} else {
			ret = engpc_bt_set_nonsig_tx_testmode(0, is_ble, 0, 0, 0, 0, 0, 0, 0);
		}

		BTD("ADL %s(), called set_nonsig_tx_testmode(), ret = %d\n", __func__, ret);

		if (0 == ret) {
			g_bteut_txrx_status = BTEUT_TXRX_STATUS_OFF;
		} else {
			BTD("ADL %s(), called set_nonsig_tx_testmode(), ret is ERROR, ret = %d\n", __func__, ret);
			goto err;
		}
	} else if (1 == on_off) {
		if (BTEUT_TXRX_STATUS_OFF != g_bteut_txrx_status) {
			BTD("ADL %s(), g_bteut_status is ERROR, g_bteut_txrx_status = %d\n", __func__,
					g_bteut_txrx_status);
			goto err;
		}

		BTD("ADL %s(), call set_nonsig_tx_testmode(), enable = 1, le = %d, pattern "
				"= %d, channel = %d, pac_type = %d, pac_len = %d, pwr_type = %d, "
				"pwr_value = %d, pkt_cnt = %d\n",
				__func__, is_ble, (int)g_bteut_tx.pattern, g_bteut_tx.channel, g_bteut_tx.pkttype,
				g_bteut_tx.pktlen, g_bteut_tx.txpwr.power_type, g_bteut_tx.txpwr.power_value,
				pktcnt);

		if (le_test_mode_inuse() && is_ble) {
			ret= engpc_bt_le_enhanced_transmitter(g_bteut_tx.channel, g_bteut_tx.pktlen, g_bteut_tx.pattern, g_bteut_tx.phy);
		} else {
			ret = engpc_bt_set_nonsig_tx_testmode(
				1, is_ble, g_bteut_tx.pattern, g_bteut_tx.channel, g_bteut_tx.pkttype,
				g_bteut_tx.pktlen, g_bteut_tx.txpwr.power_type, g_bteut_tx.txpwr.power_value, pktcnt);
		}

		BTD("ADL %s(), called set_nonsig_tx_testmode(), ret = %d\n", __func__, ret);

		if (0 == ret) {
			g_bteut_txrx_status = BTEUT_TXRX_STATUS_TXING;
		} else {
			BTD("ADL %s(), called set_nonsig_tx_testmode(), ret is ERROR, ret = %d\n", __func__, ret);
			goto err;
		}
	} else {
		BTD("ADL %s(), on_off's value is ERROR, goto err\n", __func__);
		goto err;
	}

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return 0\n", __func__, rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return -1\n", __func__, rsp);
	return -1;
}

/********************************************************************
*   name   bt_tx_get
*   ---------------------------
*   descrition: get tx's status
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
int bt_tx_get(char *rsp) {
	bteut_txrx_status bt_txrx_status = g_bteut_txrx_status;
	BTD("ADL entry %s(), \n", __func__);

	if (BTEUT_TXRX_STATUS_OFF == bt_txrx_status || BTEUT_TXRX_STATUS_TXING == bt_txrx_status) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_TX_REQ_RET : BT_TX_REQ_RET),
				(int)bt_txrx_status);
	} else {
		BTD("ADL %s(), g_bteut_status is ERROR, bt_txrx_status = %d\n", __func__, bt_txrx_status);
		goto err;
	}

	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s\n", __func__, rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return -1\n", __func__, rsp);
	return -1;
}

/********************************************************************
*   name   bt_rx_set
*   ---------------------------
*   descrition: set RX start or RX stop to Chip
*   ----------------------------
*   para        IN/OUT      type                note
*   on_off      IN          int                 1:start TX 0:Stop TX
*   rsp         OUT         char *              response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
*
********************************************************************/
int bt_rx_set(int on_off, char *rsp) {
	int ret = -1;
	char is_ble = 0;
	bt_bdaddr_t addr = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

	BTD("ADL entry %s(), on_off = %d\n", __func__, on_off);

	if (BTEUT_BT_MODE_CLASSIC == g_bt_mode) {
		is_ble = 0;
	} else if (BTEUT_BT_MODE_BLE == g_bt_mode) {
		is_ble = 1;
	} else if (BTEUT_BT_MODE_OFF == g_bt_mode) {
		BTD("ADL %s(), g_bt_mode is ERROR, g_bt_mode = %d, goto err;\n", __func__, g_bt_mode);
		goto err;
	}

	if (0 == on_off) {
		if (BTEUT_TXRX_STATUS_RXING != g_bteut_txrx_status) {
			BTD("ADL %s(), g_bteut_status is ERROR, g_bteut_txrx_status = %d\n", __func__,
					g_bteut_txrx_status);
			goto err;
		}

		BTD("ADL %s(), call set_nonsig_rx_testmode(), enable = 0, le = 0, the rest "
				"of other parameters all 0.\n",
				__func__);

		if (le_test_mode_inuse() && is_ble) {
			ret = engpc_bt_le_test_end();
		} else {
			ret = engpc_bt_set_nonsig_rx_testmode(0, is_ble, 0, 0, 0, 0, &addr);
		}

		BTD("ADL %s(), called set_nonsig_rx_testmode(), ret = %d\n", __func__, ret);

		if (0 == ret) {
			g_bteut_txrx_status = BTEUT_TXRX_STATUS_OFF;
		} else {
			BTD("ADL %s(), called set_nonsig_rx_testmode(), ret is ERROR, ret = %d\n", __func__, ret);
			goto err;
		}
	} else if (1 == on_off) {
		int rxgain_value = 0;

		if (BTEUT_TXRX_STATUS_OFF != g_bteut_txrx_status) {
			BTD("ADL %s(), g_bteut_status is ERROR, g_bteut_txrx_status = %d\n", __func__,
					g_bteut_txrx_status);
			goto err;
		}

		if (0 == g_bteut_rx.rxgain.mode) {
			rxgain_value = 0;
		} else {
			rxgain_value = g_bteut_rx.rxgain.value;
		}

		bt_str2bd(g_bteut_rx.addr, &addr);
		BTD("ADL %s(), call set_nonsig_rx_testmode(), enable = 1, le = 0, pattern "
				"= %d, channel = %d, pac_type = %d, rxgain_value = %d, addr = %s\n",
				__func__, (int)g_bteut_rx.pattern, g_bteut_rx.channel, g_bteut_rx.pkttype,
				rxgain_value, g_bteut_rx.addr);


		if (le_test_mode_inuse() && is_ble) {
			ret = engpc_bt_le_enhanced_receiver(g_bteut_rx.channel, g_bteut_rx.phy, g_bteut_rx.modulation_index);
		} else {
			ret = engpc_bt_set_nonsig_rx_testmode(1, is_ble, g_bteut_rx.pattern, g_bteut_rx.channel,
												g_bteut_rx.pkttype, rxgain_value, &addr);
		}

		BTD("ADL %s(), called set_nonsig_rx_testmode(), ret = %d\n", __func__, ret);

		if (0 == ret) {
			g_bteut_txrx_status = BTEUT_TXRX_STATUS_RXING;
		} else if (0 != ret) {
			BTD("ADL %s(), called set_nonsig_rx_testmode(), ret is ERROR, ret = %d\n", __func__, ret);
			goto err;
		}
	} else {
		BTD("ADL %s(), on_off's value is ERROR, goto err\n", __func__);
		goto err;
	}

	strncpy(rsp, (BTEUT_BT_MODE_BLE == g_bt_mode ? EUT_BLE_OK : EUT_BT_OK),
			BT_EUT_COMMAND_RSP_MAX_LEN);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return 0\n", __func__, rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return -1\n", __func__, rsp);
	return -1;
}

/********************************************************************
*   name   bt_rx_get
*   ---------------------------
*   descrition: get rx's status
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
int bt_rx_get(char *rsp) {
	bteut_txrx_status bt_txrx_status = g_bteut_txrx_status;
	BTD("ADL entry %s(), \n", __func__);

	if (BTEUT_TXRX_STATUS_OFF == bt_txrx_status || BTEUT_TXRX_STATUS_RXING == bt_txrx_status) {
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RX_REQ_RET : BT_RX_REQ_RET),
				(int)bt_txrx_status);
	} else {
		BTD("ADL %s(), bt_txrx_status is ERROR, bt_txrx_status = %d\n", __func__, bt_txrx_status);
		goto err;
	}

	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s\n", __func__, rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return -1\n", __func__, rsp);
	return -1;
}

/********************************************************************
*   name   bt_rxdata_get
*   ---------------------------
*   descrition: get rx data value from chip
*   ----------------------------
*   para        IN/OUT      type            note
*   rsp         OUT         char *          response result
*   ----------------------------------------------------
*   return
*   0:exec successful
*   other:error
*   ------------------
*   other:
*
********************************************************************/
int bt_rxdata_get(char *rsp) {
	int ret = -1;
	bteut_txrx_status bt_txrx_status = g_bteut_txrx_status;
	BTD("ADL entry %s(), rf_status = %d, g_bteut_testmode = %d, g_bteut_runing = %d\n",
			__func__, bt_txrx_status, g_bteut_testmode, (int)g_bteut_runing);

	BTD("ADL %s(), g_bt_mode = %d\n", __func__, g_bt_mode);
	if (BTEUT_TESTMODE_ENTER_NONSIG == g_bteut_testmode) {
		char buf[255] = { 0 };
		uint16_t result_len = 0;

		int rssi = 0;
		int pkt_cnt= 0;
		int pkt_err_cnt= 0;
		int bit_cnt= 0;
		int bit_err_cnt= 0;

		if (BTEUT_TXRX_STATUS_RXING == bt_txrx_status) {
			unsigned char ble = 0;
			if (BTEUT_BT_MODE_CLASSIC == g_bt_mode) {
				ble = 0;
			} else if (BTEUT_BT_MODE_BLE == g_bt_mode) {
				ble = 1;
			}

			BTD("ADL %s(), call engpc_bt_get_nonsig_rx_data(), ble = %d\n", __func__, ble);

			ret = engpc_bt_get_nonsig_rx_data(ble, buf, sizeof(buf), &result_len);
			if (!memcmp(buf, OK_STR, strlen(OK_STR))) {
				BTD("memcpy OK_STR\n");
				uki_sscanf(buf, "OK rssi:%d, pkt_cnt:%d, pkt_err_cnt:%d, bit_cnt:%d, "
						"bit_err_cnt:%d",
						&rssi, &pkt_cnt, &pkt_err_cnt, &bit_cnt, &bit_err_cnt);
			} else if (!memcmp(buf, FAIL_STR, strlen(FAIL_STR))) {
				BTD("memcpy FAIL_STR\n");
				return -1;
			} else {
				BTD("memcpy else\n");
				return -1;
			}

			BTD("ADL %s(), called engpc_bt_get_nonsig_rx_data(), ret = %d\n", __func__, ret);

			if (0 != ret) {
				BTD("ADL %s(), call engpc_bt_get_nonsig_rx_data() is ERROR, ret = %d, goto err\n",
						__func__, ret);
				goto err;
			}
		} else {
			BTD("ADL %s(), rf_status is ERROR, ret = %d, goto err\n", __func__, bt_txrx_status);
			goto err;
		}

		if (result_len <= 0) {
			BTD("ADL %s(), result_len = %d, goto err\n", __func__, result_len);
			goto err;
		}
		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d,%d,%d,%d,%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RXDATA_REQ_RET : BT_RXDATA_REQ_RET),
				bit_err_cnt, bit_cnt, pkt_err_cnt, pkt_cnt, rssi);

	} else if (BTEUT_TESTMODE_ENTER_EUT == g_bteut_testmode &&
				BTEUT_EUT_RUNNING_ON == g_bteut_runing) {
		char buf[255] = { 0 };
		uint16_t result_len = 0;
		ret = engpc_bt_dut_mode_get_rx_data(buf, sizeof(buf), &result_len);
		int rssi= 0;
		int rx_status= 0;
		if (!memcmp(buf, OK_STR, strlen(OK_STR))) {
			BTD("memcpy OK_STR\n");
			uki_sscanf(buf, "OK HCI_DUT_GET_RXDATA status:%d, rssi:%d", &rx_status, &rssi);
		} else if (!memcmp(buf, FAIL_STR, strlen(FAIL_STR))) {
			BTD("memcpy FAIL_STR\n");
			return -1;
		} else {
			BTD("memcpy else\n");
			return -1;
		}

		BTD("ADL %s(), callED dut_mode_send(HCI_DUT_GET_RXDATA), ret = %d\n", __func__, ret);

		if (0 != ret) {
			BTD("ADL %s(), callED dut_mode_send(HCI_DUT_GET_RXDATA) is ERROR, ret = %d, goto err\n",
					__func__, ret);
			goto err;
		}

		if (result_len <= 0) {
			BTD("ADL %s(), result_len = %d, goto err\n", __func__, result_len);
			goto err;
		}

		snprintf(rsp, BT_EUT_COMMAND_RSP_MAX_LEN, "%s%d",
				(BTEUT_BT_MODE_BLE == g_bt_mode ? BLE_RXDATA_REQ_RET : BT_RXDATA_REQ_RET), rssi);
	}
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s\n", __func__, rsp);
	return 0;

err:
	bt_build_err_resp(rsp);
	RESPONSE_DUMP(rsp);

	BTD("ADL leaving %s(), rsp = %s, return -1\n", __func__, rsp);
	return -1;
}

int check_bteut_ready(void) {
	return (BTEUT_TESTMODE_LEAVE == g_bteut_testmode ? 0 : -1);
}
