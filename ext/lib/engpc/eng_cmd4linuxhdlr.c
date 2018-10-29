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

#include "eng_cmd4linuxhdlr.h"
#include "wifi_eut_sprd.h"

#define SPRDENG_OK "OK"
#define SPRDENG_ERROR "ERROR"
#define ENG_STREND "\r\n"
#define ENG_TESTMODE "engtestmode"
#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))

extern int eng_atdiag_euthdlr(char *buf, int len, char *rsp, int module_index);

static int eng_linuxcmd_getwifiaddr(char *req, char *rsp);
static int eng_linuxcmd_setwifiaddr(char *req, char *rsp);
static int eng_linuxcmd_bteutmode(char *req, char *rsp);
static int eng_linuxcmd_wifieutmode(char *req, char *rsp);
static int eng_linuxcmd_get_wcn_chip(char *req, char *rsp);
static int eng_linuxcmd_getwifistatus(char *req, char *rsp);
static int eng_linuxcmd_none_handle(char *req, char *rsp);
static int eng_linuxcmd_bttest(char *req, char *rsp);
static int eng_linuxcmd_getbtaddr(char *req, char *rsp);
static int eng_linuxcmd_setbtaddr(char *req, char *rsp);
static int eng_linuxcmd_atdiag(char *req, char *rsp);
int eng_linuxcmd_bleeutmode(char *req, char *rsp);
static int eng_linuxcmd_batttest(char *req, char *rsp);

static struct eng_linuxcmd_str eng_linuxcmd[] = {
    {CMD_SENDKEY, CMD_TO_AP, "AT+SENDKEY", eng_linuxcmd_none_handle},
    {CMD_GETICH, CMD_TO_AP, "AT+GETICH?", eng_linuxcmd_none_handle},
    {CMD_ETSRESET, CMD_TO_AP, "AT+ETSRESET", eng_linuxcmd_none_handle},
    {CMD_RPOWERON, CMD_TO_AP, "AT+RPOWERON", eng_linuxcmd_none_handle},
    {CMD_GETVBAT, CMD_TO_AP, "AT+GETVBAT", eng_linuxcmd_none_handle},
    {CMD_STOPCHG, CMD_TO_AP, "AT+STOPCHG", eng_linuxcmd_none_handle},
    {CMD_TESTMMI, CMD_TO_AP, "AT+TESTMMI", eng_linuxcmd_none_handle},
    {CMD_BTTESTMODE, CMD_TO_AP, "AT+BTTESTMODE", eng_linuxcmd_bttest},
    {CMD_GETBTADDR, CMD_TO_AP, "AT+GETBTADDR", eng_linuxcmd_getbtaddr},
    {CMD_SETBTADDR, CMD_TO_AP, "AT+SETBTADDR", eng_linuxcmd_setbtaddr},
    {CMD_GSNR, CMD_TO_AP, "AT+GSNR", eng_linuxcmd_none_handle},
    {CMD_GSNW, CMD_TO_AP, "AT+GSNW", eng_linuxcmd_none_handle},
    {CMD_GETWIFIADDR, CMD_TO_AP, "AT+GETWIFIADDR", eng_linuxcmd_getwifiaddr},
    {CMD_SETWIFIADDR, CMD_TO_AP, "AT+SETWIFIADDR", eng_linuxcmd_setwifiaddr},
    {CMD_ETSCHECKRESET, CMD_TO_AP, "AT+ETSCHECKRESET",
     eng_linuxcmd_none_handle},
    {CMD_SIMCHK, CMD_TO_AP, "AT+SIMCHK", eng_linuxcmd_none_handle},
    {CMD_INFACTORYMODE, CMD_TO_AP, "AT+FACTORYMODE",
     eng_linuxcmd_none_handle},
    {CMD_FASTDEEPSLEEP, CMD_TO_APCP, "AT+SYSSLEEP", eng_linuxcmd_none_handle},
    {CMD_CHARGERTEST, CMD_TO_AP, "AT+CHARGERTEST", eng_linuxcmd_none_handle},
    {CMD_SPBTTEST, CMD_TO_AP, "AT+SPBTTEST", eng_linuxcmd_bteutmode},
    {CMD_SPBTTEST, CMD_TO_AP, "AT+SPBLETEST", eng_linuxcmd_bleeutmode},
    {CMD_SPWIFITEST, CMD_TO_AP, "AT+SPWIFITEST",
     eng_linuxcmd_wifieutmode},  // 21
    {CMD_SPGPSTEST, CMD_TO_AP, "AT+SPGPSTEST", eng_linuxcmd_none_handle},
    {CMD_ATDIAG, CMD_TO_AP, "+SPBTWIFICALI", eng_linuxcmd_atdiag},
    {CMD_BATTTEST, CMD_TO_AP, "AT+BATTTEST", eng_linuxcmd_batttest},
    {CMD_TEMPTEST, CMD_TO_AP, "AT+TEMPTEST", eng_linuxcmd_none_handle},
    {CMD_LOGCTL, CMD_TO_AP, "AT+LOGCTL", eng_linuxcmd_none_handle},
    {CMD_RTCTEST, CMD_TO_AP, "AT+RTCCTEST", eng_linuxcmd_none_handle},
    {CMD_SPWIQ, CMD_TO_AP, "AT+SPWIQ", eng_linuxcmd_none_handle},
    {CMD_PROP, CMD_TO_AP, "AT+PROP", eng_linuxcmd_none_handle},
    {CMD_SETUARTSPEED, CMD_TO_AP, "AT+SETUARTSPEED", eng_linuxcmd_none_handle},
    {CMD_AUDIOLOGCTL, CMD_TO_AP, "AT+SPAUDIOOP", eng_linuxcmd_none_handle},
    {CMD_SPCHKSD,        CMD_TO_AP,     "AT+SPCHKSD",      eng_linuxcmd_none_handle},
    {CMD_GETWIFISTATUS,        CMD_TO_AP,     "AT+GETWIFISTATUS",      eng_linuxcmd_getwifistatus},
    {CMD_EMMCSIZE,        CMD_TO_AP,     "AT+EMMCSIZE",      eng_linuxcmd_none_handle},
    {CMD_EMMCDDRSIZE,        CMD_TO_AP,     "AT+EMMCDDRSIZE",      eng_linuxcmd_none_handle},
    {CMD_GETWCNCHIP,    CMD_TO_AP,    "AT+GETWCNCHIP", eng_linuxcmd_get_wcn_chip},
    {CMD_GETANDROIDVER, CMD_TO_AP,  "AT+GETANDROIDVER", eng_linuxcmd_none_handle},
    {CMD_CPLOGCTL, CMD_TO_AP,  "AT+SPATCPLOG", eng_linuxcmd_none_handle},
    {CMD_DBGWFC,        CMD_TO_AP,     "AT+DBGWFC",      eng_linuxcmd_none_handle},
};

int eng_linuxcmd_none_handle(char *req, char *rsp)
{
	ENG_LOG("%s\n", __func__);
	return 0;
}

int eng_linuxcmd_bttest(char *req, char *rsp)
{
	ENG_LOG("%s\n", __func__);
	return 0;
}

int eng_linuxcmd_getbtaddr(char *req, char *rsp)
{
	ENG_LOG("%s\n", __func__);
	return 0;
}

int eng_linuxcmd_setbtaddr(char *req, char *rsp)
{
	ENG_LOG("%s\n", __func__);
	return 0;
}

int eng_linuxcmd_atdiag(char *req, char *rsp)
{
	ENG_LOG("%s\n", __func__);
	return 0;
}

int eng_linuxcmd_batttest(char *req, char *rsp)
{
	ENG_LOG("%s\n", __func__);
	return 0;
}

static int eng_linuxcmd_getwifistatus(char *req, char *rsp)
{
	ENG_LOG("%s\n", __func__);
	return 0;
}

int eng_at2linux(char *buf) {
  int ret = -1;
  int i;

  for (i = 0; i < (int)NUM_ELEMS(eng_linuxcmd); i++) {
    if (strstr(buf, eng_linuxcmd[i].name) != NULL) {
      ENG_LOG("eng_at2linux %s\n", eng_linuxcmd[i].name);
      if ((strstr(buf, "AT+TEMPTEST")) &&
          ((strstr(buf, "AT+TEMPTEST=1,0,1")) ||
           (strstr(buf, "AT+TEMPTEST=1,1,1")) ||
           (strstr(buf, "AT+TEMPTEST=1,0,4")) ||
           (strstr(buf, "AT+TEMPTEST=1,1,4"))))
        return -1;
      else
        return i;
    }
  }

  return ret;
}

int eng_linuxcmd_hdlr(int cmd, char *req, char *rsp)
{
	ENG_LOG("cmd is : %d  req is : %s\n", cmd, req);

	if (cmd >= (int)NUM_ELEMS(eng_linuxcmd)) {
		ENG_LOG("%s: no handler for cmd%d", __FUNCTION__, cmd);
		return -1;
  	}

	return eng_linuxcmd[cmd].cmd_hdlr(req, rsp);
}
static int eng_linuxcmd_get_wcn_chip(char *req, char *rsp) {
  if (strchr(req, '?') != NULL) {
    #ifdef WCN_USE_MARLIN3
      sprintf(rsp, "%s", "sc2355");
    #elif WCN_USE_MARLIN2
      sprintf(rsp, "%s", "sc2351");
    #elif WCN_USE_MARLIN
      sprintf(rsp, "%s", "sc2342");
    #elif WCN_USE_RS2351
      sprintf(rsp, "%s", "rs2351");
    #else
      sprintf(rsp, "%s", "ERROR");
    #endif
    return 0;
  } else {
    sprintf(rsp, "%s", "ERROR");
    return -1;
  }
}


extern int eng_atdiag_wifi_euthdlr(char *buf, int len, char *rsp, int module_index);
int eng_linuxcmd_wifieutmode(char *req, char *rsp) {
  int ret = -1;
  int len = 0;

  ENG_LOG("test... req: %s\n", req);
  ENG_LOG("%s(), cmd = %s\n", __func__, req);

  ENG_LOG("req: 0x%x.\n", req);
  ret = eng_atdiag_wifi_euthdlr(req, len, rsp, WIFI_MODULE_INDEX);

  return ret;
}

int eng_btwifimac_read(char *mac, MacType type) {
 	return 0;
}

int eng_btwifimac_write(char *bt_mac, char *wifi_mac) {
  int ret = 0;
  return ret;
}

int eng_linuxcmd_getwifiaddr(char *req, char *rsp) {
  char wifiaddr[32] = {0};
  int ret = 0;

  ret = eng_btwifimac_read(wifiaddr, ENG_WIFI_MAC);
  if (ret < 0) {
    sprintf(rsp, "%s%s", SPRDENG_ERROR, ENG_STREND);
  } else {
    sprintf(rsp, "%s%s%s%s", wifiaddr, ENG_STREND, SPRDENG_OK, ENG_STREND);
  }

  ENG_LOG("%s: rsp=%s\n", __FUNCTION__, rsp);

  return 0;
}

int eng_linuxcmd_setwifiaddr(char *req, char *rsp) {
  char address[64];
  char btaddr[32] = {0};
  char *ptr = NULL;
  char *endptr = NULL;
  int length;

  ENG_LOG("%s: req=%s\n", __FUNCTION__, req);

  ptr = strchr(req, '"');
  if (ptr == NULL) {
    ENG_LOG("%s: req %s ERROR no start \"\n", __FUNCTION__, req);
    return -1;
  }

  ptr++;

  endptr = strchr(ptr, '"');
  if (endptr == NULL) {
    ENG_LOG("%s: req %s ERROR no end \"\n", __FUNCTION__, req);
    return -1;
  }

  length = endptr - ptr + 1;

  memset(address, 0, sizeof(address));
  snprintf(address, length, "%s", ptr);

  ENG_LOG("%s: wifi address is %s; length=%d\n", __FUNCTION__, address, length);

  eng_btwifimac_read(btaddr, ENG_BT_MAC);
  eng_btwifimac_write(btaddr, address);

  sprintf(rsp, "%s%s", SPRDENG_OK, ENG_STREND);

  return 0;
}

int eng_atdiag_bt_euthdlr(char *buf, int len, char *rsp, int module_index)
{
	if (BT_MODULE_INDEX == module_index || BLE_MODULE_INDEX == module_index) {
		ENG_LOG("handle BT eut command\n");
		//bt_npi_parse(module_index, buf, rsp);
		return 0;
	}

	return 0;
}

int eng_linuxcmd_bleeutmode(char *req, char *rsp)
{
	int ret = -1;
	int len = 0;

	ENG_LOG("%s(), cmd = %s", __func__, req);

	ret = eng_atdiag_bt_euthdlr(req, len, rsp, BLE_MODULE_INDEX);
	return ret;
}

int eng_linuxcmd_bteutmode(char *req, char *rsp) {
	int ret = -1;
	int len = 0;

	ENG_LOG("%s(), cmd = %s", __FUNCTION__, req);

	ret = eng_atdiag_bt_euthdlr(req, len, rsp, BT_MODULE_INDEX);
	return ret;
}

