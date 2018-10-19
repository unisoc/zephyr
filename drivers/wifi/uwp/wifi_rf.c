/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "wifi_rf.h"
#include "wifi_cmdevt.h"

#define SEC1 (1)
#define SEC2 (2)

int wifi_rf_init(void)
{
	int ret = 0;

	SYS_LOG_DBG("download the first section of config file");
	ret = wifi_cmd_load_ini(sec1_table, sizeof(sec1_table), SEC1);
	if (ret) {
		SYS_LOG_ERR("download first section ini fail,ret = %d", ret);
		return ret;
	}

	SYS_LOG_DBG("download the second section of config file");
	ret = wifi_cmd_load_ini(sec2_table, sizeof(sec2_table), SEC2);
	if (ret) {
		SYS_LOG_ERR("download second section ini fail,ret = %d", ret);
		return ret;
	}

	SYS_LOG_DBG("Load wifi ini success.");

	return 0;
}
