#ifndef __WIFI_RF_H__
#define __WIFI_RF_H__

#include <zephyr.h>
#include <stdio.h>
#include <stdint.h>

struct nvm_cali_cmd {
	s8_t itm[64];
	s32_t par[256];
	s32_t num;
};

struct nvm_name_table {
	s8_t *itm;
	u32_t mem_offset;
	s32_t type;
};
/*[Section 1: Version] */
struct version_t {
	u16_t major;
	u16_t minor;
};

/*[Section 2: Board Config]*/
struct board_config_t {
	u16_t calib_bypass;
	u8_t txchain_mask;
	u8_t rxchain_mask;
};

/*[Section 3: Board Config TPC]*/
struct board_config_tpc_t {
	u8_t dpd_lut_idx[8];
	u16_t tpc_goal_chain0[8];
	u16_t tpc_goal_chain1[8];
};

struct tpc_element_lut_t {
	u8_t rf_gain_idx;
	u8_t pa_bias_idx;
	s8_t dvga_offset;
	s8_t residual_error;
};
/*[Section 4: TPC-LUT]*/
struct tpc_lut_t {
	struct tpc_element_lut_t chain0_lut[8];
	struct tpc_element_lut_t chain1_lut[8];
};

/*[Section 5: Board Config Frequency Compensation]*/
struct board_conf_freq_comp_t {
	s8_t channel_2g_chain0[14];
	s8_t channel_2g_chain1[14];
	s8_t channel_5g_chain0[25];
	s8_t channel_5g_chain1[25];
	s8_t reserved[2];
};

/*[Section 6: Rate To Power with BW 20M]*/
struct power_20m_t {
	s8_t power_11b[4];
	s8_t power_11ag[8];
	s8_t power_11n[17];
	s8_t power_11ac[20];
	s8_t reserved[3];
};

/*[Section 7: Power Backoff]*/
struct power_backoff_t {
	s8_t green_wifi_offset;
	s8_t ht40_power_offset;
	s8_t vht40_power_offset;
	s8_t vht80_power_offset;
	s8_t sar_power_offset;
	s8_t mean_power_offset;
	s8_t reserved[2];
};

/*[Section 8: Reg Domain]*/
struct reg_domain_t {
	u32_t reg_domain1;
	u32_t reg_domain2;
};

/*[Section 9: Band Edge Power offset (MKK, FCC, ETSI)]*/
struct band_edge_power_offset_t {
	u8_t bw20m[39];
	u8_t bw40m[21];
	u8_t bw80m[6];
	u8_t reserved[2];
};

/*[Section 10: TX Scale]*/
struct tx_scale_t {
	s8_t chain0[39][16];
	s8_t chain1[39][16];
};

/*[Section 11: misc]*/
struct misc_t {
	s8_t dfs_switch;
	s8_t power_save_switch;
	s8_t fem_lan_param_setup;
	s8_t rssi_report_diff;
};

/*[Section 12: debug reg]*/
struct debug_reg_t {
	u32_t address[16];
	u32_t value[16];
};

/*[Section 13:coex_config] */
struct coex_config_t {
	u32_t bt_performance_cfg0;
	u32_t bt_performance_cfg1;
	u32_t wifi_performance_cfg0;
	u32_t wifi_performance_cfg2;
	u32_t strategy_cfg0;
	u32_t strategy_cfg1;
	u32_t strategy_cfg2;
	u32_t compatibility_cfg0;
	u32_t compatibility_cfg1;
	u32_t ant_cfg0;
	u32_t ant_cfg1;
	u32_t isolation_cfg0;
	u32_t isolation_cfg1;
	u32_t reserved_cfg0;
	u32_t reserved_cfg1;
	u32_t reserved_cfg2;
	u32_t reserved_cfg3;
	u32_t reserved_cfg4;
	u32_t reserved_cfg5;
	u32_t reserved_cfg6;
	u32_t reserved_cfg7;
};

/*wifi config section1 struct*/
struct wifi_conf_sec1_t {
	struct version_t version;
	struct board_config_t board_config;
	struct board_config_tpc_t board_config_tpc;
	struct tpc_lut_t tpc_lut;
	struct board_conf_freq_comp_t board_conf_freq_comp;
	struct power_20m_t power_20m;
	struct power_backoff_t power_backoff;
	struct reg_domain_t reg_domain;
	struct band_edge_power_offset_t band_edge_power_offset;
};
/*wifi config section2 struct*/
struct wifi_conf_sec2_t {
	struct tx_scale_t tx_scale;
	struct misc_t misc;
	struct debug_reg_t debug_reg;
	struct coex_config_t coex_config;
};
/*wifi config struct*/
struct wifi_conf_t {
	struct version_t version;
	struct board_config_t board_config;
	struct board_config_tpc_t board_config_tpc;
	struct tpc_lut_t tpc_lut;
	struct board_conf_freq_comp_t board_conf_freq_comp;
	struct power_20m_t power_20m;
	struct power_backoff_t power_backoff;
	struct reg_domain_t reg_domain;
	struct band_edge_power_offset_t band_edge_power_offset;
	struct tx_scale_t tx_scale;
	struct misc_t misc;
	struct debug_reg_t debug_reg;
	struct coex_config_t coex_config;
};

/* int get_wifi_config_param(struct wifi_conf_t *p); */
int uwp_wifi_download_ini(void);
#endif
