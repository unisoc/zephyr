#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <gpio.h>
#include <logging/sys_log.h>
#include <stdbool.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/mesh.h>
#include <tinycrypt/hmac_prng.h>

#include "../../../../drivers/bluetooth/unisoc/uki_utlis.h"
#include "mesh.h"
#include "light.h"
#include "uwp_hal.h"


#define GPIO_PORT0		"UWP_GPIO_P0"
#define BANNER_TIMEOUT K_MSEC(800)

static struct device *gpio;
static struct gpio_callback button_cb;

static u16_t led_state = 0;

static struct k_work button_work;
static struct k_timer banner_timer;
static u8_t is_banner_running = 0;

struct onoff_state unisoc_leds[] = {
	{ .pin = LED1_GPIO_PIN,
	  .state = LED_STATE_OFF
	},
	{ .pin = LED2_GPIO_PIN,
	  .state = LED_STATE_OFF
	},
	{ .pin = LED3_GPIO_PIN,
	  .state = LED_STATE_OFF
	},
};

static void led_all_turn_on()
{
	int i;
	for (i = 0; i < sizeof(unisoc_leds)/sizeof(struct onoff_state); i++) {
		if (gpio, unisoc_leds[i].state == LED_STATE_OFF) {
			gpio_pin_write(gpio, unisoc_leds[i].pin, LED_ON_VALUE);
			unisoc_leds[i].state = LED_STATE_ON;
		}
	}
	led_state = 1;
}

static void led_all_turn_off()
{
	int i;
	for (i = 0; i < sizeof(unisoc_leds)/sizeof(struct onoff_state); i++) {
		if (gpio, unisoc_leds[i].state == LED_STATE_ON) {
			gpio_pin_write(gpio, unisoc_leds[i].pin, LED_OFF_VALUE);
			unisoc_leds[i].state = LED_STATE_OFF;
		}
	}
	led_state = 0;
}


void button_pressed_worker(struct k_work *unused)
{
	if (is_banner_running) {
		k_timer_stop(&banner_timer);
		is_banner_running = 0;
	}

	if (led_state) {
		led_all_turn_off();
	} else {
		led_all_turn_on();
	}

	NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = get_net_idx(),
		.app_idx = get_app_idx(),
		.addr = 0xFFFF,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};

	BTI("SET LED %d\n", led_state);

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GEN_ONOFF_SET);
	net_buf_simple_add_le16(&msg, led_state);
	if (bt_mesh_model_send(get_root_modules() + 4, &ctx, &msg, NULL, NULL)) {
		BTE("Unable to send On Off Status response\n");
	}

}

static void banner_timer_task(struct k_timer *work)
{
	static s8_t index = 0;

	if (!index) {
		index = 3;
		gpio_pin_write(gpio, unisoc_leds[index - 1].pin, LED_ON_VALUE); 
		unisoc_leds[index - 1].state = LED_STATE_ON;
		BTV("LED: %d ON\n", index);
		return;
	}
 	
 	if (unisoc_leds[index - 1].state == LED_STATE_ON) {
		gpio_pin_write(gpio, unisoc_leds[index - 1].pin, LED_OFF_VALUE); 
		unisoc_leds[index - 1].state = LED_STATE_OFF;
		BTV("LED: %d OFF\n", index);
	}

	index--;
	if (!index)
		index = 3;
	
	gpio_pin_write(gpio, unisoc_leds[index - 1].pin, LED_ON_VALUE); 
	unisoc_leds[index - 1].state = LED_STATE_ON;
	BTV("LED: %d ON\n", index);

}

static void button_pressed(struct device *dev, struct gpio_callback *cb,
			   u32_t pins)
{
	BTI("%s\n", __func__);
	u32_t value = 0;
	gpio_pin_read(gpio, BUTTON1_GPIO_PIN0, &value);
	if (value == -1)
		k_work_submit(&button_work);
}

void light_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	BTI("%s\n", __func__);
}

void light_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	u16_t state = net_buf_simple_pull_le16(buf);

	BTI("%s %d -> %d\n", __func__, led_state, state);

	if (is_banner_running) {
		k_timer_stop(&banner_timer);
		is_banner_running = 0;
	}

	if (state == led_state) {
		return;
	}

	if (state) {
		led_all_turn_on();
	} else {
		led_all_turn_off();
	}
}

void light_onoff_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	BTD("%s\n", __func__);
}

int cmd_led(int argc, char *argv[])
{

	return 0;
}

const struct bt_mesh_model_op unisoc_light_onoff_srv_op[] = {
	{ BT_MESH_MODEL_OP_GEN_ONOFF_GET, 0, light_onoff_get },
	{ BT_MESH_MODEL_OP_GEN_ONOFF_SET, 2, light_onoff_set },
	{ BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, light_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

static void gpio_init(void)
{
	gpio = device_get_binding(GPIO_PORT0);
	uwp_aon_irq_enable(AON_INT_GPIO0);
	gpio_pin_configure(gpio, BUTTON1_GPIO_PIN0,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE
			   | GPIO_PUD_PULL_UP
			   | GPIO_INT_ACTIVE_LOW));
	unsigned int *p = (unsigned int*)(0x40840010);
	*p = *p | (1 << 8);

	gpio_init_callback(&button_cb, button_pressed,
			   BIT(BUTTON1_GPIO_PIN0));
	gpio_add_callback(gpio, &button_cb);
	gpio_pin_enable_callback(gpio, BUTTON1_GPIO_PIN0);

	gpio_pin_configure(gpio, LED1_GPIO_PIN, 1);
	gpio_pin_configure(gpio, LED2_GPIO_PIN, 1);
	gpio_pin_configure(gpio, LED3_GPIO_PIN, 1);
}
void light_init(void)
{
	BTI("%s\n", __func__);
	gpio_init();
	k_work_init(&button_work, button_pressed_worker);
	k_timer_init(&banner_timer, banner_timer_task, NULL);
	k_timer_start(&banner_timer, 0, 800);
	is_banner_running = 1;


}
