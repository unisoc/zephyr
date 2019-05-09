/*
 * Copyright (c) 2018, UNISOC Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <device.h>
#include <dma.h>

#include "uwp_hal.h"
#include "hal_dma.h"

#define DEV_CFG(dev) \
	((const struct dma_uwp_config * const)(dev)->config->config_info)
#define DMA_STRUCT(dev) \
	((volatile struct uwp_dma *)(DEV_CFG(dev))->base)

#define CHANNEL_ADDR(x)		(DMA_BASE_ADDR + 0x1000 + x*0x40)
#define MAX_CHANNEL_NUM		(32)
static struct device DEVICE_NAME_GET(uwp_dma0);
struct dma_uwp_config
{
	const u32_t base_addr;
	const u32_t irq;
	const u32_t irq_proi;
};

static struct dma_uwp_config uwp_dma0_config =
{
	.base_addr = DMA_BASE_ADDR,
	.irq = DMA_UWP_IRQ,
	.irq_proi = DMA_UWP_IRQ_PROI,
};

struct dma_channel_uwp
{
	u32_t args;
	void (*fn)(void *callback_arg, u32_t channel,int error);
};

static struct dma_channel_uwp g_channel[MAX_CHANNEL_NUM];

static int dma_uwp_isr(struct device *dev)
{
	int index = 0;
	for(index = 0; index < 32; index++) {
		if(check_irq_status(index)) {
			clear_channel_irq(CHANNEL_ADDR(index));
			if(g_channel[index].fn != NULL) {
				g_channel[index].fn(g_channel[index].args, index, 0);
			}
		}
        }
}

int uwp_dma_initialize(struct device *dev)
{
	const struct dma_uwp_config* const dev_cfg = DEV_CFG(dev);
	dma_enable();
	dma_rst();
	IRQ_CONNECT(DMA_UWP_IRQ, DMA_UWP_IRQ_PROI, dma_uwp_isr,
			DEVICE_GET(uwp_dma0), 0);
	irq_enable(DMA_UWP_IRQ);
	return 0;
}

static int uwp_dma_transfer_stop(struct device *dev, u32_t channel)
{
	dma_stop_channel(channel);
}

static int uwp_dma_transfer_start(struct device *dev, u32_t channel)
{
	const struct dma_uwp_config * const dev_cfg = DEV_CFG(dev);
	u32_t chn_addr = CHANNEL_ADDR(channel);

	set_channel_enable(chn_addr);
	if(channel == DMA_CHANNEL_UART1_TX) {
		dma_req_for_uart(channel);
		set_uart_dma_mode(channel);
	}
	return 0;
}

static int uwp_dma_reload(struct device *dev, u32_t channel,
		u32_t src, u32_t dst, size_t size)
{
	printk("can not support reload function now\n");
	return 0;
}

static int uwp_dma_config(struct device *dev, u32_t channel,
			 struct dma_config *cfg)
{
	const struct dma_uwp_config * const dev_cfg = DEV_CFG(dev);
	u32_t chn_addr;
	chn_addr = CHANNEL_ADDR(channel);

	reset_channel_cfg(chn_addr);
	/*for common config*/
	if(!cfg->head_block->source_address) {
		printk("no source addr\n");
		return -1;
	}
	if(!cfg->head_block->dest_address) {
		printk("no dest addr\n");
		return -1;
	}
	if(!cfg->head_block->block_size) {
		printk("no block size\n");
		return -1;
	}

	dma_set_src_addr(chn_addr, cfg->head_block->source_address);
	dma_set_dest_addr(chn_addr, cfg->head_block->dest_address);

	if(cfg->dma_callback)
		g_channel[channel].fn = cfg->dma_callback;

	if(cfg->callback_arg)
		g_channel[channel].args = cfg->callback_arg;

	/*for special config*/
	if(channel == DMA_CHANNEL_UART1_TX) {
		dma_uart_set_len(chn_addr, cfg->head_block->block_size);
		dma_set_uart_write_cfg(chn_addr);
	}

	return 0;
}

static const struct dma_driver_api uwp_dma_driver_api = {
	.config = uwp_dma_config,
	.reload = uwp_dma_reload,
	.start = uwp_dma_transfer_start,
	.stop = uwp_dma_transfer_stop,
};

DEVICE_AND_API_INIT(uwp_dma0, CONFIG_DMA_0_NAME, uwp_dma_initialize,
		    NULL, &uwp_dma0_config, PRE_KERNEL_2,
		    10, (void *)&uwp_dma_driver_api);
