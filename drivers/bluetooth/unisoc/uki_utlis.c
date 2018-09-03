#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <atomic.h>
#include <misc/util.h>
#include <misc/slist.h>
#include <misc/byteorder.h>
#include <misc/stack.h>
#include <misc/__assert.h>
#include <soc.h>

#include <bluetooth/bluetooth.h>
#include "uki_utlis.h"

int vendor_log_level = LOG_LEVEL_INFO;
int stack_log_level = LOG_LEVEL_INFO;

void uki_hexdump(char *tag, unsigned char *bin, size_t binsz)
{
	char str[500], hex_str[]= "0123456789ABCDEF";
	size_t i;

	for (i = 0; i < binsz; i++) {
		str[(i * 3) + 0] = hex_str[(bin[i] >> 4) & 0x0F];
		str[(i * 3) + 1] = hex_str[(bin[i]     ) & 0x0F];
		str[(i * 3) + 2] = ' ';
	}
	str[(binsz * 3) - 1] = 0x00;
	if (tag)
		printk("%s %s\n", tag, str);
	else
		printk("%s\n", str);
}

void uki_hex_dump_block(char *tag, unsigned char *bin, size_t binsz)
{
	int loop = binsz / HEX_DUMP_BLOCK_SIZE;
	int tail = binsz % HEX_DUMP_BLOCK_SIZE;
	int i;

	if (!loop) {
		uki_hexdump(tag, bin, binsz);
		return;
	}

	for (i = 0; i < loop; i++) {
		uki_hexdump(tag, bin + i * HEX_DUMP_BLOCK_SIZE, HEX_DUMP_BLOCK_SIZE);
	}

	if (tail)
		uki_hexdump(tag, bin + i * HEX_DUMP_BLOCK_SIZE, tail);
}
