#ifndef __EUT_OPT_H__
#define __EUT_OPT_H__

#include <stdio.h>
#include <stdlib.h>

#define ENG_EUT ("EUT")
#define ENG_EUT_REQ ("EUT?")
#define ENG_WIFICH_REQ ("CH?")
#define ENG_WIFICH ("CH")

/* here is WIFI cmd */
#define ENG_WIFIMODE "MODE"
#define ENG_WIFIRATIO_REQ "RATIO?"
#define ENG_WIFIRATIO "RATIO"
#define ENG_WIFITX_FACTOR_REQ "TXFAC?"
#define ENG_WIFITX_FACTOR "TXFAC"

#define ENG_TX_REQ ("TX?")
#define ENG_TX ("TX")
#define ENG_RX_REQ ("RX?")
#define ENG_RX ("RX")
#define ENG_WIFI_CLRRXPACKCOUNT "CLRRXPACKCOUNT"
#define ENG_GPSSEARCH_REQ "SEARCH?"
#define ENG_GPSSEARCH "SEARCH"
#define ENG_GPSPRNSTATE_REQ "PRNSTATE?"
#define ENG_GPSSNR_REQ "SNR?"
#define ENG_GPSPRN "PRN"
//------------------------------------------------

#define ENG_WIFIRATE "RATE"
#define ENG_WIFIRATE_REQ "RATE?"
#define ENG_WIFITXGAININDEX "TXGAININDEX"
#define ENG_WIFITXGAININDEX_REQ "TXGAININDEX?"
#define ENG_WIFIRX_PACKCOUNT "RXPACKCOUNT?"
#define ENG_WIFIRSSI_REQ "RSSI?"
#define ENG_WIFILNA_REQ ("LNA?")
#define ENG_WIFILNA ("LNA")

#define ENG_WIFIBAND_REQ ("BAND?")
#define ENG_WIFIBAND ("BAND")

#define ENG_WIFIBANDWIDTH_REQ ("BW?")
#define ENG_WIFIBANDWIDTH ("BW")

#define ENG_WIFISIGBANDWIDTH_REQ ("SBW?")
#define ENG_WIFISIGBANDWIDTH ("SBW")

#define ENG_WIFITXPWRLV_REQ ("TXPWRLV?")
#define ENG_WIFITXPWRLV ("TXPWRLV")

#define ENG_WIFIPKTLEN_REQ ("PKTLEN?")
#define ENG_WIFIPKTLEN ("PKTLEN")

#define ENG_WIFITXMODE_REQ ("TXMODE?")
#define ENG_WIFITXMODE ("TXMODE")

#define ENG_WIFIPREAMBLE_REQ ("PREAMBLE?")
#define ENG_WIFIPREAMBLE ("PREAMBLE")

#define ENG_WIFIPAYLOAD_REQ ("PAYLOAD?")
#define ENG_WIFIPAYLOAD ("PAYLOAD")

#define ENG_GUARDINTERVAL_REQ ("GUARDINTERVAL?")
#define ENG_GUARDINTERVAL ("GUARDINTERVAL")

#define ENG_MACFILTER_REQ ("MACFILTER?")
#define ENG_MACFILTER ("MACFILTER")

#define ENG_CDECEFUSE ("CDECEFUSE")
#define ENG_CDECEFUSE_REQ ("CDECEFUSE?")

#define ENG_MACEFUSE_REQ ("MACEFUSE?")
#define ENG_MACEFUSE ("MACEFUSE")

#define ENG_WIFIANT_REQ ("ANT?")
#define ENG_WIFIANT ("ANT")
#define ENG_WIFIANTINFO_REQ ("ANTINFO?")

#define ENG_WIFICBANK ("CBANK")

#define ENG_WIFINETMODE ("NETMODE")

#define ENG_WIFIDECODEMODE_REQ ("DECODEMODE?")
#define ENG_WIFIDECODEMODE ("DECODEMODE")


#define EUT_BT_OK ("+SPBTTEST:OK")
#define EUT_BT_ERROR ("+SPBTTEST:ERR=")
#define EUT_BT_REQ ("+SPBTTEST:EUT=")
// BLE
#define EUT_BLE_OK ("+SPBLETEST:OK")
#define EUT_BLE_ERROR ("+SPBLETEST:ERR=")
#define EUT_BLE_REQ ("+SPBLETEST:EUT=")

#define EUT_WIFI_OK ("+SPWIFITEST:OK")
#define EUT_WIFI_ERROR ("+SPWIFITEST:ERR=")
#define EUT_WIFI_REQ ("+SPWIFITEST:EUT=")
#define EUT_WIFI_TX_REQ ("+SPWIFITEST:TX=")
#define EUT_WIFI_RX_REQ ("+SPWIFITEST:RX=")
#define EUT_WIFI_CH_REQ ("+SPWIFITEST:CH=")
#define EUT_WIFI_RATIO_REQ ("+SPWIFITEST:RATIO=")
#define EUT_WIFI_TXFAC_REQ ("+SPWIFITEST:TXFAC=")
#define EUT_WIFI_RXPACKCOUNT_REQ ("+SPWIFITEST:RXPACKCOUNT=")
#define EUT_WIFI_BAND_REQ		("+SPWIFITEST:BAND=")
#define EUT_WIFI_TX_MODE_REQ	("+SPWIFITEST:TXMODE=")
#define EUT_WIFI_TX_PWRLV_REQ	("+SPWIFITEST: TXPWRLV =")
#define EUT_WIFI_PKT_LEN_REQ ("+SPWIFITEST:PKTLEN=")
#define EUT_WIFI_BW_REQ ("+SPWIFITEST:BW=")

#define EUT_WIFIERR_RATIO (150)
#define EUT_WIFIERR_TXFAC (151)
#define EUT_WIFIERR_TXFAC_SETRATIOFIRST (152)

#define EUT_BT_OK ("+SPBTTEST:OK")
#define EUT_BT_ERROR ("+SPBTTEST:ERR=")
#define EUT_BT_REQ ("+SPBTTEST:EUT=")

#define EUT_BLE_OK ("+SPBLETEST:OK")
#define EUT_BLE_ERROR ("+SPBLETEST:ERR=")
#define EUT_BLE_REQ ("+SPBLETEST:EUT=")

struct eut_cmd {
  int index;
  char* name;
};

typedef enum {
  WIFI_MODE_11B = 1,
  WIFI_MODE_11G,
  WIFI_MODE_11N,
} WIFI_WORKE_MODE;

/* band width */
typedef enum {
  WIFI_BW_20MHZ = 0,
  WIFI_BW_40MHZ,
  WIFI_BW_80MHZ,
  WIFI_BW_160MHZ,
  WIFI_BW_INVALID = 0xff
} wifi_bw;

/* preamble type */
typedef enum {
  WIFI_PREAMBLE_TYPE_NORMAL = 0,         /* normal */
  WIFI_PREAMBLE_TYPE_CCK_SHORT,          /* cck short */
  WIFI_PREAMBLE_TYPE_80211_MIX_MODE,     /* 802.11 mixed mode */
  WIFI_PREAMBLE_TYPE_80211N_GREEN_FIELD, /* 802.11n green field */
  WIFI_PREAMBLE_TYPE_80211AC_FIELD,
  WIFI_PREAMBLE_TYPE_MAX_VALUE = 0xff
} wifi_eut_preamble_type;

struct eng_bt_eutops {
  int (*bteut)(int, char *);
  int (*bteut_req)(char *);
};

int bteut(int command_code, char *rsp);
int wifieut(int command_code,char *rsp);
int wifiband(int band, char *rsp);
int wifiband_req(char *rsp);
int wifi_tx_mode_bcm(int tx_mode, char *rsp);
int wifi_tx_mode_req(char *rsp);
int wifi_tx_pwrlv(int pwrlv, char *rsp);
int wifi_tx_pwrlv_req(char *rsp);
int wifi_tx(int command_code,char *rsp);
int wifi_rx(int command_code,char *rsp);
int set_wifi_tx_factor_83780(int factor,char *result);
int set_wifi_ratio(float ratio,char *result);
int set_wifi_ch(int ch,char *result);
int wifi_rxpackcount(char *result);
int wifi_clr_rxpackcount(char *result);
int set_wifi_rate(char *string, char *rsp);
int wifi_rate_req(char *rsp);
int set_wifi_txgainindex(int index, char *rsp);
int wifi_txgainindex_req(char *rsp);
int wifi_rssi_req(char *rsp);
int wifi_pm(int command_code,char *result);
int wifi_eut_preamble_set(wifi_eut_preamble_type preamble_type, char *rsp);
int wifi_pkt_len_set(int pkt_len, char *rsp);
int wifi_pkt_len_get(char *rsp);
int wifi_bw_get(char *rsp);
int wifi_bw_set(wifi_bw band_width, char *rsp);
#endif

