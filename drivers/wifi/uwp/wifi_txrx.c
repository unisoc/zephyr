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
	return 0;
}

int wifi_tx_complete_handle(void * data,int len)
{
	return 0;
}

int wifi_tx_cmd(void *data, int len)
{
	int ret;

	ret = wifi_ipc_send(SMSG_CH_WIFI_CTRL,QUEUE_PRIO_HIGH,
			data,len,WIFI_CTRL_MSG_OFFSET);

	return ret;
}

#if 0
int wifi_rx_data(int channel,void *data,int len)
{
	struct sprdwl_common_hdr *common_hdr = (struct sprdwl_common_hdr *)data;

	if (channel > SMSG_CH_WIFI_DATA_SPEC || data == NULL ||len ==0){
		SYS_LOG_ERR("invalid parameter,channel=%d data=%p,len=%d\n",channel,data,len);
		return -1;
	}
	if (channel == SMSG_CH_WIFI_DATA_SPEC) {
		SYS_LOG_INF("rx spicial data\n");
	} else if (channel ==SMSG_CH_WIFI_DATA_NOR) {
		SYS_LOG_INF("rx data\n");
	} else {
		SYS_LOG_ERR("should not be here[channel = %d]\n",channel);
		return -1;
	}

	switch (common_hdr->type){
		case SPRDWL_TYPE_DATA_PCIE_ADDR:
			if (common_hdr->direction_ind) /* direction_ind=1, rx data */
				wifi_rx_complete_handle(data,len);
			else
				wifi_tx_complete_handle(data,len); /* direction_ind=0, tx complete */
			break;
		case SPRDWL_TYPE_DATA:
		case SPRDWL_TYPE_DATA_SPECIAL:
			break;
		default :
			SYS_LOG_INF("unknown type :%d\n",common_hdr->type);			
	}

	return 0;
}
#endif

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
			if (ret != 0) {
				SYS_LOG_INF("Recieve none data.");
				break;
			}

			SYS_LOG_INF("Recieve data %p len %i", addr, len);

			/* process data here */
			wifi_cmdevt_process(priv, addr, len);
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
	addr_buf.common.type = SPRDWL_TYPE_DATA_PCIE_ADDR;
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
