#include <zephyr.h>
#include <logging/sys_log.h>
#include <string.h>
#include <sipc.h>
#include <sblock.h>

#include "wifi_main.h"
#define RX_BUF_SIZE	(128)
static unsigned char rx_buf[RX_BUF_SIZE];

int wifi_rx_complete_handle(void *data,int len)
{
	SYS_LOG_ERR("wifi rx complete.");
	return 0;
}

int wifi_tx_complete_handle(void * data,int len)
{
	struct txc_addr_buff *txc_addr = (struct txc_addr_buff *)data;
	u16_t payload_num;
	u32_t payload_addr;
	int i;
	struct net_pkt *pkt;

	SYS_LOG_ERR("wifi tx complete.");

	payload_num = txc_addr->number;

	for (i = 0; i < payload_num; i++) {
		/* we use only 4 byte addr */
		memcpy(&payload_addr, txc_addr->data + (i * SPRDWL_PHYS_LEN), 4);

		payload_addr -= sizeof(struct tx_msdu_dscr);
		SPRD_CP_TO_AP_ADDR(payload_addr);
		pkt = (struct net_pkt *)uwp_get_addr_from_payload(payload_addr);

		SYS_LOG_ERR("pkt addr: 0x%x.", pkt);
		net_pkt_unref(pkt);
	}

	return 0;
}

int wifi_tx_cmd(void *data, int len)
{
	int ret;

	ret = wifi_ipc_send(SMSG_CH_WIFI_CTRL,QUEUE_PRIO_HIGH,
			data,len,WIFI_CTRL_MSG_OFFSET);

	return ret;
}

int wifi_data_process(struct wifi_priv *priv, char *data, int len)
{
	struct sprdwl_common_hdr *common_hdr = (struct sprdwl_common_hdr *)data;
	data += sizeof(struct sprdwl_common_hdr);
	switch (common_hdr->type) {
		case SPRDWL_TYPE_DATA_SPECIAL:
			if (common_hdr->direction_ind)  /* direction_ind=1, rx data */
				wifi_rx_complete_handle(data, len);
			else
				wifi_tx_complete_handle(data, len); /* direction_ind=0, tx complete */
			break;
		case SPRDWL_TYPE_DATA:
		case SPRDWL_TYPE_DATA_HIGH_SPEED:
			break;
		default:
			SYS_LOG_ERR("unknown type :%d\n", common_hdr->type);
	}
	return 0;
}


#define TXRX_STACK_SIZE		(1024)
K_THREAD_STACK_MEMBER(txrx_stack, TXRX_STACK_SIZE);
static struct k_thread txrx_thread_data;

static struct k_sem	event_sem;
static void txrx_thread(void *p1)
{
	int ret;
	struct wifi_priv *priv = (struct wifi_priv *)p1;
	u8_t *addr = rx_buf;
	int len;

	while (1) {
		SYS_LOG_DBG("wait for data.");
		k_sem_take(&event_sem, K_FOREVER);

		while (1) {
			memset(addr, RX_BUF_SIZE, 0);
			ret = wifi_ipc_recv(SMSG_CH_WIFI_CTRL, addr, &len, 0);
			if (ret == 0) {
				SYS_LOG_INF("Recieve cmd/evt %p len %i", addr, len);

				wifi_cmdevt_process(priv, addr, len);
			} else {
				break;
			}
		}

		while (1) {
			ret = wifi_ipc_recv(SMSG_CH_WIFI_DATA_NOR, addr, &len, 0);
			if (ret == 0) {
				SYS_LOG_ERR("Recieve data %p len %i", addr, len);
				read8_cmd_exe((u32_t)addr, len);
				wifi_data_process(priv, addr, len);
			} else {
				break;
			}
		}

	}
}

static void wifi_rx_event(int ch)
{
	if(ch == SMSG_CH_WIFI_CTRL)
		k_sem_give(&event_sem);
}

int wifi_tx_data(void *data, int len)
{
	int ret;
	struct hw_addr_buff_t addr_buf;

	memset(&addr_buf, 0, sizeof(struct hw_addr_buff_t));
	addr_buf.common.interface = 0;
	addr_buf.common.type = SPRDWL_TYPE_DATA_SPECIAL;
	addr_buf.common.direction_ind = 0;
	addr_buf.common.buffer_type = 1;
	addr_buf.number = 1;
	addr_buf.offset = 7;
	addr_buf.tx_ctrl.pcie_mh_readcomp = 1;

	memset(addr_buf.pcie_addr[0], 0, SPRDWL_PHYS_LEN);
	memcpy(addr_buf.pcie_addr[0], &data, 4);  //copy addr to addr buf.

	ret = wifi_ipc_send(SMSG_CH_WIFI_DATA_NOR, QUEUE_PRIO_NORMAL,
				(void *)&addr_buf,
				1 * SPRDWL_PHYS_LEN + sizeof(struct hw_addr_buff_t),
				WIFI_DATA_NOR_MSG_OFFSET);
	if (ret < 0) {
		SYS_LOG_ERR("sprd_wifi_send fail\n");
		return ret;
	}

	return 0;
}

static void wifi_rx_data(int ch)
{
	if(ch != SMSG_CH_WIFI_DATA_NOR)
		SYS_LOG_ERR("Invalid data channel: %d.", ch);

	SYS_LOG_ERR("wifi rx data.");
	k_sem_give(&event_sem);
}

int wifi_tx_empty_buf(int num)
{
	int i;
	struct net_buf	*pkt_buf;
	static struct rx_empty_buff buf;
	int ret;
	struct hw_addr_buff_t addr_buf;
	u32_t data_ptr;

	memset(&buf, 0, sizeof(buf));

	for (i = 0; i < num; i++) {
		pkt_buf = net_pkt_get_frag(0, K_NO_WAIT);
		if(pkt_buf == NULL) {
			SYS_LOG_ERR("Could not allocate rx packet");
			break;
		}
		data_ptr = (u32_t)pkt_buf->data;
		uwp_save_addr_before_payload((u32_t)data_ptr,
				(void *)pkt_buf);

		SPRD_AP_TO_CP_ADDR(data_ptr);
		memcpy(&(buf.addr[i][0]), &data_ptr, 4);
	}

	if (i == 0) {
		SYS_LOG_ERR("Nonue rx packet buffer can be used.");
		return -1;
	}

	buf.type = EMPTY_DDR_BUFF;
	buf.num = i;
	buf.common.type = SPRDWL_TYPE_DATA_SPECIAL;
	buf.common.direction_ind = TRANS_FOR_RX_PATH;

	ret = wifi_ipc_send(SMSG_CH_WIFI_DATA_NOR, QUEUE_PRIO_NORMAL,
			(void *)&buf,
			i * SPRDWL_PHYS_LEN + 3,
			WIFI_DATA_NOR_MSG_OFFSET);
	if (ret < 0) {
		SYS_LOG_ERR("sprd_wifi_send fail\n");
		return ret;
	}

	return 0;
}

int wifi_txrx_init(struct wifi_priv *priv)
{
	int ret = 0;
	k_sem_init(&event_sem, 0, 1);

	ret = wifi_ipc_create_channel(SMSG_CH_WIFI_CTRL,
			wifi_rx_event);
	if(ret < 0) {
		SYS_LOG_ERR("Create wifi control channel failed.");
		return ret;
	}

	ret = wifi_ipc_create_channel(SMSG_CH_WIFI_DATA_NOR,
			wifi_rx_data);
	if(ret < 0) {
		SYS_LOG_ERR("Create wifi data channel failed.");
		return ret;
	}

	ret = wifi_ipc_create_channel(SMSG_CH_WIFI_DATA_SPEC,
			wifi_rx_data);
	if(ret < 0) {
		SYS_LOG_ERR("Create wifi data channel failed.");
		return ret;
	}

	k_thread_create(&txrx_thread_data, txrx_stack,
			TXRX_STACK_SIZE,
			(k_thread_entry_t)txrx_thread, (void *) priv, NULL, NULL,
			K_PRIO_COOP(7),
			0, K_NO_WAIT);

	return 0;
}
