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

#define DMA_MAX_CHANNEL     4

#define DEV_CFG(dev) \
	((const struct dma_uwp_config * const)(dev)->config->config_info)
#define DMA_STRUCT(dev) \
	((volatile struct uwp_dma *)(DEV_CFG(dev))->base)

static struct device DEVICE_NAME_GET(uwp_dma0);
struct dma_uwp_config
{
    u32_t base_addr;
    u32_t irq;
    u32_t irq_proi;
};

struct dma_uwp_config uwp_dma0_config = 
{
    .base_addr = 0x40120000,
    .irq = 25,
    .irq_proi = 3,
};

struct dma_channel_uwp
{
    u32_t type;
    u32_t status;
    u32_t addr;
    void *args;
    void (*fn)(void *callback_arg, u32_t channel,int error);
};

static struct dma_channel_uwp g_channel[DMA_MAX_CHANNEL];

static int dma_uwp_isr(struct device *dev)
{
    printk("dma isr handler\n");

    int index = 0;
    for(index = 0; index < 32; index++) {
        if(check_irq_status(index)) {
            printk("has irq\n");
            if (index < 4) {
                   printk("call irq function we have register\n"); 
                   g_channel[index].fn(g_channel[index].args, index, 0);
            }
        }
    }
}

static int uwp_dma_initialize(struct device *dev)
{
    int index = 0;
    const struct dma_uwp_config * const dev_cfg = DEV_CFG(dev);
    dma_enable();
    dma_rst();
    IRQ_CONNECT(DMA_UWP_IRQ	, DMA_UWP_IRQ_PROI, dma_uwp_isr,
			DEVICE_GET(uwp_dma0), 0);
	irq_enable(dev_cfg->irq);

    //init globe channel
    for(index = 0; index < DMA_MAX_CHANNEL; index++) {
        g_channel[index].status = CHANNEL_IDLE;
        g_channel[index].type = TYPE_FULL;
        g_channel[index].addr = dev_cfg->base_addr + 0x1000 + index * 0x40;
    }
}

static int uwp_dma_transfer_stop(struct device *dev, u32_t channel)
{
    return 0;
}

static int uwp_dma_transfer_start(struct device *dev, u32_t channel)
{
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
    if (channel >= 4) {
        printk("Invalid channel num\n");
        return -1;
    }
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

    dma_set_src_addr(g_channel[channel].addr, cfg->head_block->source_address);
    dma_set_dest_addr(g_channel[channel].addr, cfg->head_block->dest_address);
    dma_set_blk_len(g_channel[channel].addr, cfg->head_block->block_size);
    if(cfg->dma_callback)
        g_channel[channel].fn = cfg->dma_callback;

    if(cfg->callback_arg)
        g_channel[channel].args = cfg->callback_arg;

    dma_set_fix_cfg(g_channel[channel].addr);
    return 0;
}

static const struct dma_driver_api uwp_dma_driver_api = {
	.config = uwp_dma_config,
	.reload = uwp_dma_reload,
	.start = uwp_dma_transfer_start,
	.stop = uwp_dma_transfer_stop,
};

DEVICE_AND_API_INIT(uwp_dma0, CONFIG_DMA_0_NAME, &uwp_dma_initialize,
		    NULL, &uwp_dma0_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &uwp_dma_driver_api);