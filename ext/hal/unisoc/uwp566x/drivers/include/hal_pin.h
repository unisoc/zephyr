/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HAL_PIN_H
#define __HAL_PIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "uwp_hal.h"

#define CTL_PIN_BASE	BASE_AON_PIN

#define REG_PIN_GPIO0	(CTL_PIN_BASE + 0x10)
#define REG_PIN_ESMD3	(CTL_PIN_BASE + 0x20)
#define REG_PIN_ESMD2	(CTL_PIN_BASE + 0x24)

#define FUNC_PULL_UP	(1 << 8)
#define FUNC_PULL_DOWN	(1 << 7)

	static inline void pin_gpio0_pull_up(void)
	{
		sci_write32(REG_PIN_GPIO0,
				sci_read32(REG_PIN_GPIO0) | FUNC_PULL_UP);
	}

#ifdef __cplusplus
}
#endif

#endif
