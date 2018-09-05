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
#ifndef __UKI_CONFIG_H_
#define __UKI_CONFIG_H_


#define BT_CONFIG_PSKEY_FILE "/NAND:/BT_CON~1.INI"
#define BT_CONFIG_RF_FILE "/NAND:/BT_CON~2.INI"
#define BT_CONFIG_INFO_FILE "/NAND:/BT_INFO.INI"


#define CONF_COMMENT '#'
#define CONF_DELIMITERS " =\n\r\t"
#define CONF_VALUES_DELIMITERS "=\n\r\t#"
#define CONF_VALUES_PARTITION " ,=\n\r\t#"
#define CONF_MAX_LINE_LEN 255

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
void vnd_load_configure(const char *p_path, const conf_entry_t *entry, unsigned int mask);

#endif  // LIBBT_CONF_SPRD_MARLIN_INCLUDE_MARLIN_H_
