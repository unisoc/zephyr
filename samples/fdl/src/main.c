/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

#include <gpio.h>
#include <watchdog.h>
#include <device.h>
#include <console.h>
#include <flash.h>
#include <shell/shell.h>
#include <string.h>
#include <misc/util.h>
#include <uart.h>
#include <stdlib.h>
#include <logging/sys_log.h>

#include "uwp_hal.h"

#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN	"UNISOC"

#define UART_2			"UART_2"

void main(void)
{
	SYS_LOG_WRN("Unisoc fdl");



	while(1) {}
}
