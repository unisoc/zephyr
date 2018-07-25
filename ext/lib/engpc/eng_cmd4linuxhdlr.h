#ifndef __ENG_CMD4LINUXHDLR_H__
#define __ENG_CMD4LINUXHDLR_H__

typedef enum {
  CMD_SENDKEY = 0,
  CMD_GETICH,
  CMD_ETSRESET,
  CMD_RPOWERON,
  CMD_GETVBAT,
  CMD_STOPCHG,
  CMD_TESTMMI,
  CMD_BTTESTMODE,
  CMD_GETBTADDR,
  CMD_SETBTADDR,
  CMD_GSNR,
  CMD_GSNW,
  CMD_GETWIFIADDR,
  CMD_SETWIFIADDR,
  CMD_CGSNW,
  CMD_ETSCHECKRESET,
  CMD_SIMCHK,
  CMD_ATDIAG,
  CMD_INFACTORYMODE,
  CMD_FASTDEEPSLEEP,
  CMD_CHARGERTEST,
  CMD_SETUARTSPEED,
  CMD_SPBTTEST,
  CMD_SPWIFITEST,
  CMD_SPGPSTEST,
  CMD_BATTTEST,
  CMD_TEMPTEST,
  CMD_LOGCTL,
  CMD_RTCTEST,
  CMD_SPWIQ,
  CMD_PROP,
  CMD_AUDIOLOGCTL,
  CMD_SPCHKSD,
  CMD_GETWIFISTATUS,
  CMD_EMMCSIZE,
  CMD_EMMCDDRSIZE,
  CMD_GETWCNCHIP,
  CMD_GETANDROIDVER,
  CMD_CPLOGCTL,
  CMD_DBGWFC,
  CMD_END
} ENG_CMD;

typedef enum {
  BT_MODULE_INDEX = 0,
  WIFI_MODULE_INDEX,
  GPS_MODULE_INDEX,
  BLE_MODULE_INDEX
} eut_modules;


typedef enum {
  CMD_INVALID_TYPE,
  CMD_TO_AP,  /* handled by AP */
  CMD_TO_APCP /* handled by AP and CP */
} eng_cmd_type;

struct eng_linuxcmd_str {
  int index;
  eng_cmd_type type;
  char *name;
  int (*cmd_hdlr)(char *, char *);
};

typedef enum
{
    ENG_BT_MAC,
    ENG_WIFI_MAC
} MacType;

int eng_linuxcmd_hdlr(int cmd, char *req, char *rsp);
int eng_at2linux(char *buf);
#endif
