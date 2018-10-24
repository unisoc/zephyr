/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <flash.h>
#include <init.h>
#include <soc.h>
#include <logging/sys_log.h>

#include "uwp_hal.h"

#define DEV_CFG(dev) \
	((struct flash_uwp_config *)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct flash_uwp_data * const)(dev)->driver_data)

#define FLASH_WRITE_BLOCK_SIZE 0x1

struct flash_uwp_config {
	struct spi_flash flash;
	struct spi_flash_params *params;
	struct k_mutex lock;
};

/* Device run time data */
struct flash_uwp_data {
};

static struct flash_uwp_config uwp_config;
static struct flash_uwp_data uwp_data;

/*
 * This is named flash_uwp_lock instead of flash_uwp_lock (and
 * similarly for flash_uwp_unlock) to avoid confusion with locking
 * actual flash pages.
 */
static inline void flash_uwp_lock(struct device *dev)
{
	k_mutex_lock(&DEV_CFG(dev)->lock, K_FOREVER);
}

static inline void flash_uwp_unlock(struct device *dev)
{
	k_mutex_unlock(&DEV_CFG(dev)->lock);
}

static int flash_uwp_write_protection(struct device *dev, bool enable)
{
	struct flash_uwp_config *cfg = DEV_CFG(dev);
	struct spi_flash *flash = &(cfg->flash);

	int ret = 0;

	flash_uwp_lock(dev);

	if(enable)
		ret = flash->lock(flash, 0, flash->size);
	else
		ret = flash->unlock(flash, 0, flash->size);

	flash_uwp_unlock(dev);

	return ret;
}

static int flash_uwp_read(struct device *dev, off_t offset, void *data,
			    size_t len)
{
	int ret = 0;
	//struct flash_uwp_config *cfg = DEV_CFG(dev);
	//struct spi_flash *flash = &(cfg->flash);

	if (!len) {
		return 0;
	}

	SYS_LOG_WRN("flash read: 0x%x len: 0x%x", offset, len);
	flash_uwp_lock(dev);

	//ret = flash->read(flash, offset, data, len, READ_SPI_FAST);
	//ret = flash->read(flash, offset, data, len, READ_SPI);
	memcpy(data, (void *)((u32_t)CONFIG_FLASH_BASE_ADDRESS + offset), len);


	flash_uwp_unlock(dev);

	return ret;
}

static int flash_uwp_erase(struct device *dev, off_t offset, size_t len)
{
	int ret;
	struct flash_uwp_config *cfg = DEV_CFG(dev);
	struct spi_flash *flash = &(cfg->flash);

	if (!len) {
		return 0;
	}

	flash_uwp_lock(dev);

	ret = flash->erase(flash, CONFIG_FLASH_BASE_ADDRESS + offset, len);

	flash_uwp_unlock(dev);

	return ret;
}

static int flash_uwp_write(struct device *dev, off_t offset,
			     const void *data, size_t len)
{
	int ret;
	struct flash_uwp_config *cfg = DEV_CFG(dev);
	struct spi_flash *flash = &(cfg->flash);

	if (!len) {
		return 0;
	}

	flash_uwp_lock(dev);

	ret = flash->write(flash, CONFIG_FLASH_BASE_ADDRESS + offset, len, data);

	flash_uwp_unlock(dev);

	return ret;
}

static const struct flash_driver_api flash_uwp_api = {
	.write_protection = flash_uwp_write_protection,
	.erase = flash_uwp_erase,
	.write = flash_uwp_write,
	.read = flash_uwp_read,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
//	.page_layout = flash_uwp_page_layout,
#endif
#ifdef FLASH_WRITE_BLOCK_SIZE
	.write_block_size = FLASH_WRITE_BLOCK_SIZE,
#else
#error Flash write block size not available
	/* Flash Write block size is extracted from device tree */
	/* as flash node property 'write-block-size' */
#endif
};

#define SFC_CLK_OUT_DIV_1       (0x0)
#define SFC_CLK_OUT_DIV_2       BIT(0)
#define SFC_CLK_OUT_DIV_4       BIT(1)
#define SFC_CLK_SAMPLE_DELAY_SEL    BIT(2)
#define SFC_CLK_2X_EN           BIT(10)
#define SFC_CLK_OUT_2X_EN       BIT(9)
#define SFC_CLK_SAMPLE_2X_PHASE     BIT(8)
#define SFC_CLK_SAMPLE_2X_EN        BIT(7)

static int uwp_flash_init(struct device *dev)
{
	int ret = 0;

#if 0
	SFCDRV_ClkCfg(SFC_CLK_OUT_DIV_2 | SFC_CLK_OUT_2X_EN |
			SFC_CLK_2X_EN | SFC_CLK_SAMPLE_2X_PHASE |
			SFC_CLK_SAMPLE_2X_EN);

	/*
	 * cgm_sfc_1x_div: clk_sfc_1x = clk_src/(bit 9:8 + 1)
	 * */
	sci_write32(REG_AON_CLK_RF_CGM_SFC_1X_CFG, 0x100);
	/* 0: xtal MHz 1: 133MHz 2: 139MHz 3: 160MHz 4: 208MHz
	 * cgm_sfc_2x_sel: clk_sfc_1x source (bit 2:1:0)
	 * */
	sci_write32(REG_AON_CLK_RF_CGM_SFC_2X_CFG, 0x4);

	struct flash_uwp_config *cfg = DEV_CFG(dev);
	struct spi_flash *flash = &(cfg->flash);

	ret = uwp_spi_flash_init(flash, &(cfg->params));
	if(!ret) {
		printk("uwp spi flash init failed.\n");
		return ret;
	}
	k_mutex_init(&cfg->lock);
#endif

	return ret;
}

DEVICE_AND_API_INIT(uwp_flash, FLASH_DEV_NAME,
		    uwp_flash_init,
			&uwp_data,
			&uwp_config, APPLICATION,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_uwp_api);
