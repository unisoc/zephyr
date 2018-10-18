/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DT_UNISOC_MEM_H
#define __DT_UNISOC_MEM_H

#define __SIZE_K(x) (x * 1024)
#define __SIZE_M(x) (x * 1024 * 1024)

#if defined(CONFIG_SOC_UWP566x)
#define DT_SRAM_START		0x100000
#define DT_SRAM_SIZE		__SIZE_K(832)
#define DT_UART0_START		0x40038000
#define DT_UART1_START		0x40040000
#define DT_UART2_START		0x40838000
#define DT_UART_SIZE		0x30
#define DT_FLASH_START		0x2000000
#define DT_FLASH_SIZE		__SIZE_M(4)
#else
#error "Flash and RAM sizes not defined for this chip"
#endif

#endif /* __DT_UNISOC_MEM_H */
