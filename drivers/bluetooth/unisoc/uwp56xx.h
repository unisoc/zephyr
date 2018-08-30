/******************************************************************************
 *
 *  Copyright (C) 2016 Spreadtrum Corporation
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
#ifndef __MARLIN3_HW_H_
#define __MARLIN3_HW_H_

#include <stdint.h>

#define BT_HCI_OP_PSKEY 0xFCA0
#define BT_HCI_OP_RF 0xFCA2
#define BT_HCI_OP_ENABLE_CMD 0xFCA1

#define HCI_CMD_MAX_LEN 258
#define HCI_CMD_PREAMBLE_SIZE 3

#define CONF_ITEM_TABLE(ITEM, ACTION, BUF, LEN) \
  { #ITEM, ACTION, &(BUF.ITEM), LEN, (sizeof(BUF.ITEM) / LEN) }

#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}
#define UINT24_TO_STREAM(p, u24) {*(p)++ = (uint8_t)(u24); *(p)++ = (uint8_t)((u24) >> 8); *(p)++ = (uint8_t)((u24) >> 16);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (uint8_t)(u8);}


enum { DUAL_MODE = 0, CLASSIC_MODE, LE_MODE };
enum { DISABLE_BT = 0, ENABLE_BT };


typedef int(conf_action_t)(char *p_conf_name, char *p_conf_value, void *buf,
                           int len, int size);

typedef struct {
    const char *conf_entry;
    conf_action_t *p_action;
    void *buf;
    int len;
    int size;
} conf_entry_t;


typedef struct {
    uint32_t  device_class;
    uint8_t  feature_set[16];
    uint8_t  device_addr[6];
    uint16_t  comp_id;
    uint8_t g_sys_uart0_communication_supported;
    uint8_t cp2_log_mode;
    uint8_t LogLevel;
    uint8_t g_central_or_perpheral;
    uint16_t Log_BitMask;
    uint8_t super_ssp_enable;
    uint8_t common_rfu_b3;
    uint32_t common_rfu_w[2];
    uint32_t le_rfu_w[2];
    uint32_t lmp_rfu_w[2];
    uint32_t lc_rfu_w[2];
    uint16_t g_wbs_nv_117;
    uint16_t g_wbs_nv_118;
    uint16_t g_nbv_nv_117;
    uint16_t g_nbv_nv_118;
    uint8_t g_sys_sco_transmit_mode;
    uint8_t audio_rfu_b1;
    uint8_t audio_rfu_b2;
    uint8_t audio_rfu_b3;
    uint32_t audio_rfu_w[2];
    uint8_t g_sys_sleep_in_standby_supported;
    uint8_t g_sys_sleep_master_supported;
    uint8_t g_sys_sleep_slave_supported;
    uint8_t power_rfu_b1;
    uint32_t power_rfu_w[2];
    uint32_t win_ext;
    uint8_t edr_tx_edr_delay;
    uint8_t edr_rx_edr_delay;
    uint8_t tx_delay;
    uint8_t rx_delay;
    uint32_t bb_rfu_w[2];
    uint8_t agc_mode;
    uint8_t diff_or_eq;
    uint8_t ramp_mode;
    uint8_t modem_rfu_b1;
    uint32_t modem_rfu_w[2];
    uint32_t BQB_BitMask_1;
    uint32_t BQB_BitMask_2;
    uint16_t bt_coex_threshold[8];
    uint32_t other_rfu_w[2];
} pskey_config_t;

typedef struct {
    uint16_t g_GainValue_A[6];
    uint16_t g_ClassicPowerValue_A[10];
    uint16_t g_LEPowerValue_A[16];
    uint16_t g_BRChannelpwrvalue_A[8];
    uint16_t g_EDRChannelpwrvalue_A[8];
    uint16_t g_LEChannelpwrvalue_A[8];
    uint16_t g_GainValue_B[6];
    uint16_t g_ClassicPowerValue_B[10];
    uint16_t g_LEPowerValue_B[16];
    uint16_t g_BRChannelpwrvalue_B[8];
    uint16_t g_EDRChannelpwrvalue_B[8];
    uint16_t g_LEChannelpwrvalue_B[8];
    uint16_t LE_fix_powerword;
    uint8_t Classic_pc_by_channel;
    uint8_t LE_pc_by_channel;
    uint8_t RF_switch_mode;
    uint8_t Data_Capture_Mode;
    uint8_t Analog_IQ_Debug_Mode;
    uint8_t RF_common_rfu_b3;
    uint32_t RF_common_rfu_w[5];
}rf_config_t;

#endif  // LIBBT_CONF_SPRD_MARLIN_INCLUDE_MARLIN_H_
