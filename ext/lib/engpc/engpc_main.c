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

#include "engpc.h"
#include "eng_diag.h"

static char ext_data_buf[DATA_EXT_DIAG_SIZE];
static int ext_buf_len;
static struct device *uart;
//just use 2048, phone use (4096*64)
#define DATA_BUF_SIZE (2048)

#define CONFIG_ENGPC_THREAD_DEFPRIO 100
#define CONFIG_ENGPC_THREAD_STACKSIZE 4096

typedef enum {
	ENG_DIAG_RECV_TO_AP,
	ENG_DIAG_RECV_TO_CP,
} eng_diag_state;

#define LOG_LEN		2048
static unsigned char log_data[LOG_LEN];
static unsigned int log_data_len;
//static char backup_data_buf[DATA_BUF_SIZE];
static int g_diag_status = ENG_DIAG_RECV_TO_AP;
static struct k_sem	uart_rx_sem;

struct engpc_buf
{
	unsigned char buf[LOG_LEN];
	unsigned int used;
	bool valid;
};

struct engpc_buf g_buf;
int get_user_diag_buf( unsigned char *buf, int len)
{
	int i;
	int is_find = 0;
	//eng_dump(buf, len, len, 1, __FUNCTION__);

	for (i = 0; i < len; i++) {
		if (buf[i] == 0x7e && ext_buf_len == 0) {  // start
			ext_data_buf[ext_buf_len++] = buf[i];
		} else if (ext_buf_len > 0 && ext_buf_len < DATA_EXT_DIAG_SIZE) {
			ext_data_buf[ext_buf_len] = buf[i];
			ext_buf_len++;
			if (buf[i] == 0x7e) {
				is_find = 1;
				break;
			}
		}
	}
	ENG_LOG("jessie: print ext_data_buf, ext_buf_len is %d\n", ext_buf_len);
	//eng_dump(ext_data_buf, ext_buf_len, ext_buf_len, 1, __FUNCTION__);
	return is_find;
}

void clean_engpc_buf()
{
	ENG_LOG(" start to clean buf\n");
	memset(g_buf.buf, 0, LOG_LEN);
	g_buf.used = 0;
	g_buf.valid = true;
}
void init_user_diag_buf(void)
{
	memset(ext_data_buf, 0, DATA_EXT_DIAG_SIZE);
	ext_buf_len = 0;
	//clean_engpc_buf();
}

void show_buf(unsigned char *buf, unsigned int len)
{
	unsigned int index = 0;
	ENG_LOG(" len is : %d \n", len);
	for(index = 0; index < len; index++) {
		ENG_LOG("%02x ", buf[index]);
	}
	ENG_LOG("\n");
}

static unsigned int offset = 0;
void uart_cb(struct device *dev)
{
	u8_t c;
	if (uart_irq_rx_ready(uart)) {

		while (1) {
			if (uart_fifo_read(uart, &c , 1) == 0) {
				clean_engpc_buf();
				//ENG_LOG("jessie: uart_fifo_read is null, clean buf & break\n");
				break;
			}

			log_data[offset++] = c;

			if (offset >= LOG_LEN){
				//ENG_LOG("jessie: offset is larger than LOG_LEN, break\n");
				break;
			}
		}
		k_sem_give(&uart_rx_sem);
	}
}

int engpc_thread(int argc, char *argv[])
{
	int has_processed = 0;

	init_user_diag_buf();
	while(1) {
		ENG_LOG("jessie: %s before take sem\n", __FUNCTION__);
		k_sem_take(&uart_rx_sem, K_FOREVER);
		if (offset > 0) {
			//ENG_LOG("jessie: print log_data, offset is %d\n", offset);
			show_buf(log_data, offset);
			//check_data(log_data, offset);
			memcpy(g_buf.buf + g_buf.used, log_data, offset);
			g_buf.used += offset;
			//ENG_LOG("jessie: print gbuf, gbuf used  is %d\n", g_buf.used);
			show_buf(g_buf.buf, g_buf.used);
			memset(log_data, 0, LOG_LEN);
			offset = 0;
			if (get_user_diag_buf(g_buf.buf, g_buf.used)){
				ENG_LOG("jessie: find 0x7e\n");
				g_diag_status = ENG_DIAG_RECV_TO_AP;
			}
			has_processed = eng_diag(uart, ext_data_buf, ext_buf_len);

			init_user_diag_buf();
		}

		ENG_LOG("ALL handle finished!!!!!!!!!\n");
#if 0
		if (get_user_diag_buf(g_buf.buf, g_buf.used)){
			ENG_LOG("jessie: find 0x7e\n");
			g_diag_status = ENG_DIAG_RECV_TO_AP;
		}
		has_processed = eng_diag(uart, ext_data_buf, ext_buf_len);

		init_user_diag_buf();
		ENG_LOG("ALL handle finished!!!!!!!!!\n");
#endif
	}
}

#define ENGPC_STACK_SIZE		(2048)
K_THREAD_STACK_MEMBER(engpc_stack, ENGPC_STACK_SIZE);
static struct k_thread engpc_thread_data;
int engpc_init(struct device *dev)
{
	u8_t c;

	uart = device_get_binding(UART_DEV);
	if(uart == NULL) {
		SYS_LOG_ERR("Can not find device %s.", UART_DEV);
		return -1;
	}

	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);
	uart_irq_callback_set(uart, uart_cb);

	while (uart_irq_rx_ready(uart)) {
		uart_fifo_read(uart, &c, 1);
	}

	uart_irq_rx_enable(uart);

	SYS_LOG_INF("open serial success");

	k_sem_init(&uart_rx_sem, 0, 1);

	SYS_LOG_INF("start engpc thread.");
	k_thread_create(&engpc_thread_data, engpc_stack,
			ENGPC_STACK_SIZE,
			(k_thread_entry_t)engpc_thread, NULL, NULL, NULL,
			K_PRIO_COOP(7),
			0, K_NO_WAIT);

	return 0;
}

SYS_INIT(engpc_init, APPLICATION, 1);
