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

static unsigned char log_data[128];
static unsigned int log_data_len;
//static char backup_data_buf[DATA_BUF_SIZE];
static int g_diag_status = ENG_DIAG_RECV_TO_AP;
static struct k_sem	uart_rx_sem;

/*paramters for poll thread*/
//static char poll_name[] = "poll_thread";
//static char poll_para[] = "poll thread from engpc";

int get_user_diag_buf( unsigned char *buf, int len) {
	int i;
	int is_find = 0;
	eng_dump(buf, len, len, 1, __FUNCTION__);

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
	return is_find;
}

void init_user_diag_buf(void)
{
	memset(ext_data_buf, 0, DATA_EXT_DIAG_SIZE);
	ext_buf_len = 0;
}

void uart_cb(struct device *dev)
{
	int has_processed = 0;

	log_data_len = uart_fifo_read(uart, log_data, sizeof(log_data)); //just read 128 byte.

	if (get_user_diag_buf(log_data, log_data_len)) {
		g_diag_status = ENG_DIAG_RECV_TO_AP;
	}

	k_sem_give(&uart_rx_sem);
}

int engpc_thread(int argc, char *argv[])
{
	int has_processed = 0;

	memset(log_data,0x00, sizeof(log_data));
	init_user_diag_buf();
	while(1) {
		k_sem_take(&uart_rx_sem, K_FOREVER);
		SYS_LOG_INF("read cnt : %d :", log_data_len);

		has_processed = eng_diag(uart, ext_data_buf, ext_buf_len);

		memset(log_data,0x00, sizeof(log_data));
		init_user_diag_buf();
		printk("ALL handle finished!!!!!!!!!\n");
	}
}

#define ENGPC_STACK_SIZE		(1024)
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

	k_sem_init(&uart_rx_sem, 0, UINT_MAX);

	SYS_LOG_INF("start engpc thread.");
	k_thread_create(&engpc_thread_data, engpc_stack,
			ENGPC_STACK_SIZE,
			(k_thread_entry_t)engpc_thread, NULL, NULL, NULL,
			K_PRIO_COOP(7),
			0, K_NO_WAIT);

	return 0;
}

SYS_INIT(engpc_init, APPLICATION, 1);
