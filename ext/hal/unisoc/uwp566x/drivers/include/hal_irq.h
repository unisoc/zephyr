/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_IRQ_H
#define __HAL_IRQ_H

#ifdef __cplusplus
extern "C" {
#endif

#define NVIC_INT_FIQ			0
#define NVIC_INT_IRQ			1
#define NVIC_INT_IPI			19
#define NVIC_INT_AON_INTC		20
#define NVIC_INT_UART0			35
#define NVIC_INT_UART1			36
#define NVIC_INT_WDG			40
#define NVIC_INT_GNSS2BTWF_IPI	50

#ifdef __cplusplus
}
#endif

#endif
