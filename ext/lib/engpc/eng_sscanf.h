#ifndef _ENG_SSCANf_H
#define _ENG_SSCANf_H

#include <kernel.h>
#include <init.h>
#include <stdint.h>
#include <device.h>
#include <uart.h>
#include <logging/sys_log.h>
#include <zephyr/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <misc/printk.h>
#include <zephyr.h>

#include <stdarg.h>
#include <ctype.h>

int eng_sscanf(const char *buf, const char *fmt, ...);

#endif
