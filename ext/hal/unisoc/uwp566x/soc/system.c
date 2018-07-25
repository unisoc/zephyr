/*
 * Copyright (c) 2018, UNISOC Incorporated
 * All rights reserved.
 *
 */
#include <zephyr.h>
#include <uwp_hal.h>

void uwp_glb_init(void)
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


