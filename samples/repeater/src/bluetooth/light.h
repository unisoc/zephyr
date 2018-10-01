#ifndef __UNISOC_LIGHT_H__
#define __UNISOC_LIGHT_H__

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/mesh.h>

#define BUTTON1_GPIO_PIN0 0
#define LED1_GPIO_PIN 1
#define LED2_GPIO_PIN 3
#define LED3_GPIO_PIN 2

#define LED_STATE_OFF 0
#define LED_STATE_ON 1

#define LED_OFF_VALUE 1
#define LED_ON_VALUE 0


#define BT_MESH_MODEL_OP_GEN_ONOFF_GET		BT_MESH_MODEL_OP_2(0x82, 0x01)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET		BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS	BT_MESH_MODEL_OP_2(0x82, 0x04)

struct onoff_state {
	u8_t current;
	u8_t previous;
	u8_t pin;
	u8_t state;
	struct device *led_device;
};

void light_init(void);
void light_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf);
void light_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf);
void light_onoff_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf);

int cmd_led(int argc, char *argv[]);


#endif
