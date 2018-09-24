#ifndef __LIBBT_SPRD_BT_ENG_H__
#define __LIBBT_SPRD_BT_ENG_H__

/*
**  Definitions for HCI groups
*/
#define HCI_GRP_HOST_CONT_BASEBAND_CMDS    (0x03 << 10)            /* 0x0C00 */
#define HCI_GRP_TESTING_CMDS               (0x06 << 10)            /* 0x1800 */
#define HCI_GRP_BLE_CMDS                   (0x08 << 10)            /* 0x2000 */
#define HCI_GRP_VENDOR_SPECIFIC            (0x3F << 10)            /* 0xFC00 */

/* Commands of HCI_GRP_HOST_CONT_BASEBAND_CMDS */
#define HCI_SET_EVENT_MASK                 (0x0001 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_RESET                          (0x0003 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_SET_EVENT_FILTER               (0x0005 | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_SCAN_ENABLE              (0x001A | HCI_GRP_HOST_CONT_BASEBAND_CMDS)
#define HCI_WRITE_CURRENT_IAC_LAP          (0x003A | HCI_GRP_HOST_CONT_BASEBAND_CMDS)

/* LE Get Vendor Capabilities Command OCF */
#define NONSIG_TX_ENABLE                   (0x00D1 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_TX_DISABLE                  (0x00D2 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_RX_ENABLE                   (0x00D3 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_RX_GETDATA                  (0x00D4 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_RX_DISABLE                  (0x00D5 | HCI_GRP_VENDOR_SPECIFIC)

#define NONSIG_LE_TX_ENABLE                (0x00D6 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_TX_DISABLE               (0x00D7 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_RX_ENABLE                (0x00D8 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_RX_GETDATA               (0x00D9 | HCI_GRP_VENDOR_SPECIFIC)
#define NONSIG_LE_RX_DISABLE               (0x00DA | HCI_GRP_VENDOR_SPECIFIC)

#define HCI_DUT_SET_TXPWR                  (0x00E1 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_DUT_SET_RXGIAN                 (0x00E2 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_DUT_GET_RXDATA                 (0x00E3 | HCI_GRP_VENDOR_SPECIFIC)

#define HCI_DUT_SET_RF_PATH                (0x00DB | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_DUT_GET_RF_PATH                (0x00DC | HCI_GRP_VENDOR_SPECIFIC)

#define HCI_OP_PSKEY                       (0x00A0 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_RF                          (0x00A2 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_ENABLE                      (0x00A1 | HCI_GRP_VENDOR_SPECIFIC)

/* Commands of HCI_GRP_TESTING_CMDS group */
#define HCI_ENABLE_DEV_UNDER_TEST_MODE     (0x0003 | HCI_GRP_TESTING_CMDS)

/* LE CONTROLLER COMMANDS */
#define HCI_BLE_ENHANCED_RECEIVER_TEST     (0x0033 | HCI_GRP_BLE_CMDS)
#define HCI_BLE_ENHANCED_TRANSMITTER_TEST  (0x0034 | HCI_GRP_BLE_CMDS)
#define HCI_BLE_TEST_END                   (0x001F | HCI_GRP_BLE_CMDS)

/* Scan enable flags */
#define HCI_INQUIRY_SCAN_ENABLED        0x01
#define HCI_PAGE_SCAN_ENABLED           0x02

/* Filters that are sent in set filter command */
#define HCI_FILTER_CONNECTION_SETUP     0x02

#define HCI_FILTER_COND_NEW_DEVICE      0x00
#define HCI_FILTER_COND_DEVICE_CLASS    0x01
#define HCI_FILTER_COND_BD_ADDR         0x02

#define HCI_DO_AUTO_ACCEPT_CONNECT      2   /* role switch disabled */

typedef struct {
	uint8_t address[6];
} bt_bdaddr_t;

typedef struct {
	uint16_t opcode;
	uint16_t param_len;
	uint8_t* p_param_buf;
} hci_cmd_complete_t;

struct bt_npi_dev {
	u16_t last_cmd;
	u16_t len;
	u16_t opcode;
	u8_t data[300];
	struct k_sem sync;
};

int engpc_bt_dut_mode_configure(uint8_t enable);
int engpc_bt_off(void);
int engpc_bt_on(void);
int engpc_bt_dut_mode_send(uint16_t opcode, uint8_t *buf, uint8_t len);
int engpc_bt_set_nonsig_tx_testmode(uint16_t enable, uint16_t is_le, uint16_t pattern,
									uint16_t channel, uint16_t pac_type, uint16_t pac_len,
									uint16_t power_type, uint16_t power_value, uint16_t pac_cnt);

int engpc_bt_set_nonsig_rx_testmode(uint16_t enable, uint16_t is_le, uint16_t pattern,
									uint16_t channel, uint16_t pac_type, uint16_t rx_gain,
									bt_bdaddr_t *addr);
int engpc_bt_get_nonsig_rx_data(uint16_t le, char *buf, uint16_t buf_len, uint16_t *read_len);
int engpc_bt_dut_mode_get_rx_data(char *buf, uint16_t buf_len, uint16_t *read_len);
int engpc_bt_le_test_mode(uint16_t opcode, uint8_t *buf, uint8_t len);
int engpc_bt_le_enhanced_receiver(uint8_t channel, uint8_t phy, uint8_t modulation_index);
int engpc_bt_le_enhanced_transmitter(uint8_t channel, uint8_t length, uint8_t payload, uint8_t phy);
int engpc_bt_le_test_end(void);
int engpc_bt_set_rf_path(uint8_t path);
int engpc_bt_get_rf_path(void);
int engpc_bt_dut_mode_send(uint16_t opcode, uint8_t *buf, uint8_t len);
#endif
