/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_566x_DEBUG
__attribute__ ((section(".dbghdr")))
const unsigned long ulDebugHeader[] = {
	0x5AA5A55A,
	0x000FF800,
	0xEFA3247D
};
#endif
