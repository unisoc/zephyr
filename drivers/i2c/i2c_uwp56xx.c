#include <errno.h>
#include <i2c.h>
#include <soc.h>
#include <misc/util.h>
#include <gpio.h>

#include "i2c-priv.h"

#include <logging/sys_log.h>

#define BASE_ADDRESS (0x40840000)
#define I2C_SDA_ADDR (0x40840000 | 0x80)
#define I2C_SCL_ADDR (0x40840000 | 0x84)

#define PIN_FUNC2 (0x10)
#define PIN_PULL_UP (0x100)

#define AON_GLB_REG_BASE 0x4083C000
#define I2C_CTL_ADDR (0x40834000)
#define I2C_ADDR_CFG_ADDR (0x40834000 | 0x04)
#define I2C_COUNT_ADDR (0x40834000 | 0x08)
#define I2C_TX_ADDR (0x40834000 | 0x10)
#define I2C_STATUS_ADDR (0x40834000 | 0x14)
#define I2C_ADDR_DV0_ADDR (0x40834000 | 0x20)


struct i2c_unisoc_config_t {
	void (*irq_config_func)(struct device *dev);
	u32_t bitrate;
	u32_t sda_pin;
	u32_t scl_pin;
};


struct i2c_unisoc_data_t {
	struct k_sem sem;
	u32_t rxd:1;
	u32_t txd:1;
	u32_t err:1;
	u32_t stopped:1;
	struct device *gpio;
	struct k_sem lock;
};


static int i2c_unisoc_configure(struct device *dev, u32_t dev_config_raw)
{
#if 0
	const struct i2c_nrf5_config *config = dev->config->config_info;
	struct i2c_nrf5_data *data = dev->driver_data;
	volatile NRF_TWI_Type *twi = config->base;
	int ret = 0;


	SYS_LOG_DBG("");

	if (I2C_ADDR_10_BITS & dev_config_raw) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	switch (I2C_SPEED_GET(dev_config_raw)) {
	case I2C_SPEED_STANDARD:
		twi->FREQUENCY = TWI_FREQUENCY_FREQUENCY_K100;
		break;
	case I2C_SPEED_FAST:
		twi->FREQUENCY = TWI_FREQUENCY_FREQUENCY_K400;
		break;
	default:
		SYS_LOG_ERR("unsupported speed");
		ret = -EINVAL;
	}

	k_sem_give(&data->lock);

	return ret;
#endif
	return 0;
}

static int i2c_unisoc_read(struct device *dev, struct i2c_msg *msg)
{

	SYS_LOG_ERR("");

#if 0
	const struct i2c_nrf5_config *config = dev->config->config_info;
	struct i2c_nrf5_data *data = dev->driver_data;
	volatile NRF_TWI_Type *twi = config->base;

	__ASSERT_NO_MSG(msg->len);

	if (msg->flags & I2C_MSG_RESTART) {
		/* No special behaviour required for
		 * repeated start.
		 */
	}

	for (int offset = 0; offset < msg->len; offset++) {
		if (offset == msg->len-1) {
			SYS_LOG_DBG("SHORTS=2");
			twi->SHORTS = 2; /* BB->STOP */
		} else {
			SYS_LOG_DBG("SHORTS=1");
			twi->SHORTS = 1; /* BB->SUSPEND */
		}

		if (offset == 0) {
			SYS_LOG_DBG("STARTRX");
			twi->TASKS_STARTRX = 1;
		} else {
			SYS_LOG_DBG("RESUME");
			twi->TASKS_RESUME = 1;
		}

		k_sem_take(&data->sem, K_FOREVER);

		if (data->err) {
			data->err = 0;
			SYS_LOG_DBG("rx error 0x%x", twi->ERRORSRC);
			twi->TASKS_STOP = 1;
			twi->ENABLE = TWI_ENABLE_ENABLE_Disabled;
			return -EIO;
		}

		__ASSERT_NO_MSG(data->rxd);

		SYS_LOG_DBG("RXD");
		data->rxd = 0;
		msg->buf[offset] = twi->RXD;
	}

	if (msg->flags & I2C_MSG_STOP) {
		SYS_LOG_DBG("TASK_STOP");
		k_sem_take(&data->sem, K_FOREVER);
		SYS_LOG_DBG("err=%d txd=%d rxd=%d stopped=%d errsrc=0x%x",
			    data->err, data->txd, data->rxd,
			    data->stopped, twi->ERRORSRC);
		__ASSERT_NO_MSG(data->stopped);
		data->stopped = 0;
	}
#endif
	return 0;
}

static int i2c_unisoc_write(struct device *dev,
			  struct i2c_msg *msg)
{
	__ASSERT_NO_MSG(msg->len);

	SYS_LOG_ERR("");

	*((u32_t*)(I2C_COUNT_ADDR)) = msg->len;
	SYS_LOG_ERR("0x%08X : 0x%08X", I2C_COUNT_ADDR, *((u32_t*)(I2C_COUNT_ADDR)));
	for (int offset = 0; offset < msg->len; offset++) {
		//SYS_LOG_ERR("txd=0x%x", msg->buf[offset]);
		*((u32_t*)(I2C_TX_ADDR)) = msg->buf[offset];
	}

	*((u32_t*)(I2C_ADDR_DV0_ADDR)) = 0X003D003D;
	*((u32_t*)(I2C_CTL_ADDR)) = 0x00088005;

	return 0;
}

static int i2c_unisoc_transfer(struct device *dev, struct i2c_msg *msgs,
			     u8_t num_msgs, u16_t addr)
{
	int ret = 0;

	*((u32_t*)(I2C_SDA_ADDR)) = 0x8010;
	*((u32_t*)(I2C_SCL_ADDR)) = 0x8010;
	*((u32_t*)(AON_GLB_REG_BASE + 0x24)) |=  0x01 << 8;

	//k_sem_take(&data->lock, K_FOREVER);

	SYS_LOG_ERR("transaction-start addr=0x%x", addr);

	*((u32_t*)(I2C_CTL_ADDR)) = 0x00088000;
	*((u32_t*)(I2C_STATUS_ADDR)) = 0x00;
	*((u32_t*)(I2C_ADDR_CFG_ADDR)) = addr;

	for (int i = 0; i < num_msgs; i++) {
		SYS_LOG_ERR("msg len=%d %s%s%s", msgs[i].len,
			    (msgs[i].flags & I2C_MSG_READ) ? "R":"W",
			    (msgs[i].flags & I2C_MSG_STOP) ? "S":"-",
			    (msgs[i].flags & I2C_MSG_RESTART) ? "+":"-");

		if (msgs[i].flags & I2C_MSG_READ) {
			ret = i2c_unisoc_read(dev, msgs + i);
		} else {
			ret = i2c_unisoc_write(dev, msgs + i);
		}

		if (ret != 0) {
			break;
		}
	}
	//k_sem_give(&data->lock);

	return ret;
}

static int i2c_unisoc_init(struct device *dev)
{
	*((u32_t*)(I2C_SDA_ADDR)) = PIN_FUNC2 | PIN_PULL_UP;
	*((u32_t*)(I2C_SCL_ADDR)) = PIN_FUNC2 | PIN_PULL_UP;
	*((u32_t*)(AON_GLB_REG_BASE)) |=  0x01 << 8;
	return 0;
}

static const struct i2c_driver_api i2c_unisoc_driver_api = {
	.configure = i2c_unisoc_configure,
	.transfer = i2c_unisoc_transfer,
};

static const struct i2c_unisoc_config_t i2c_unisoc_config = {
	.irq_config_func = NULL,
	.bitrate = 0,
	.sda_pin = 0,
	.scl_pin = 0,
};

static struct i2c_unisoc_data_t i2c_unisoc_data = {
	.lock = _K_SEM_INITIALIZER(i2c_unisoc_data.lock, 1, 1),
};

DEVICE_AND_API_INIT(i2c_uwp56xx, "uwp56xx_i2c", i2c_unisoc_init,
		    &i2c_unisoc_data, &i2c_unisoc_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &i2c_unisoc_driver_api);
