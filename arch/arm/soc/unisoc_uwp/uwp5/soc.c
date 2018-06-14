/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>

#include <uwp_hal.h>

extern void enable_caches(void);

static inline void uwp_glb_init(void)
{
	/* Set system clock to 416M */
	sci_write32(REG_APB_EB, 0xffffffff); /* FIXME */
	sci_glb_set(REG_AON_CLK_CTRL4, BIT_AON_APB_CGM_RTC_EN);
	sci_write32(REG_APB_EB, 0xf7ffffff); /* FIXME */

	sci_reg_and(REG_AON_PLL_CTRL_CFG, ~0xFFFF);
	sci_reg_or(REG_AON_PLL_CTRL_CFG, CLK_416M);

	sci_write32(REG_AON_CLK_RF_CGM_ARM_CFG,
			BIT_AON_CLK_RF_CGM_ARM_SEL(CLK_SRC5));

	sci_reg_or(REG_AHB_MTX_CTL1, BIT(22));
	sci_reg_or(REG_AON_PD_AT_RESET, BIT(0));
}

static int unisoc_uwp_init(struct device *arg)
{
	ARG_UNUSED(arg);
	uwp_glb_init();
	enable_caches();

	return 0;
}

SYS_INIT(unisoc_uwp_init, PRE_KERNEL_1, 0);
