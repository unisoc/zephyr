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
#include "uwp_hal.h"

#if !defined(NODE_ADDR)
#define NODE_ADDR 0x0b0c
#endif

#define GROUP_ADDR 0xc000
#define PUBLISHER_ADDR  0x000f

#define BT_COMP_ID           0x01EC
#define MOD_ID 0x1688
#define OP_VENDOR_ACTION BT_MESH_MODEL_OP_3(0x00, BT_COMP_ID)

#define GPIO_PORT0		"UWP_GPIO_P0"
#define BUTTON1_GPIO_PIN0 0
#define LED1_GPIO_PIN1 1
#define LED2_GPIO_PIN3 3
#define LED3_GPIO_PIN2 2

#define CUR_FAULTS_MAX 4

#define BT_MESH_MODEL_OP_GEN_ONOFF_GET		BT_MESH_MODEL_OP_2(0x82, 0x01)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET		BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS	BT_MESH_MODEL_OP_2(0x82, 0x04)

static struct device *gpio;

static u8_t cur_faults[CUR_FAULTS_MAX];
static u8_t reg_faults[CUR_FAULTS_MAX * 2];

static struct k_work button_work;

static u16_t led_state = 0;
static u16_t led_light_state = 0;

BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv, NULL, 2 + 2);

static const u8_t net_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t dev_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t app_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

extern u32_t sys_rand32_get();

static struct {
	u16_t local;
	u16_t dst;
	u16_t net_idx;
	u16_t app_idx;
} net = {
	.local = BT_MESH_ADDR_UNASSIGNED,
	.dst = BT_MESH_ADDR_UNASSIGNED,
};

struct onoff_state {
	u8_t current;
	u8_t previous;
	u8_t led_gpio_pin;
	struct device *led_device;
};

static struct onoff_state onoff_state[] = {
	{ .led_gpio_pin = LED1_GPIO_PIN1 },
	{ .led_gpio_pin = LED2_GPIO_PIN3 },
	{ .led_gpio_pin = LED3_GPIO_PIN2 },
};

static const u16_t net_idx;
static const u16_t app_idx;
static const u32_t iv_index;
static u8_t flags;
static u16_t addr = NODE_ADDR;

static bt_mesh_input_action_t input_act;
static u8_t input_size;

static struct tc_hmac_prng_struct prng;

static void heartbeat(u8_t hops, u16_t feat)
{
	printk("heartbeat()\n");
}

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_DISABLED,
	.beacon = BT_MESH_BEACON_DISABLED,
#if defined(CONFIG_BT_MESH_FRIEND)
	.frnd = BT_MESH_FRIEND_DISABLED,
#else
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	.gatt_proxy = BT_MESH_GATT_PROXY_DISABLED,
#else
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif

	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};

static void gen_onoff_get(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	BTD("%s\n", __func__);
}

static void gen_onoff_set(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	u16_t state = net_buf_simple_pull_le16(buf);

	BTD("%s %d -> %d\n", __func__, led_light_state, state);

	if (state == led_light_state) {
		return;
	}

	if (state) {
		gpio_pin_write(gpio, LED2_GPIO_PIN3, 1);
		gpio_pin_write(gpio, LED3_GPIO_PIN2, 1);
	} else {
		gpio_pin_write(gpio, LED2_GPIO_PIN3, 0);
		gpio_pin_write(gpio, LED3_GPIO_PIN2, 0);
	}
	led_light_state = state;
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
				struct bt_mesh_msg_ctx *ctx,
				struct net_buf_simple *buf)
{
	BTD("%s\n", __func__);
}

static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
	{ BT_MESH_MODEL_OP_GEN_ONOFF_GET, 0, gen_onoff_get },
	{ BT_MESH_MODEL_OP_GEN_ONOFF_SET, 2, gen_onoff_set },
	{ BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, gen_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};


static void get_faults(u8_t *faults, u8_t faults_size, u8_t *dst, u8_t *count)
{
	u8_t i, limit = *count;

	for (i = 0, *count = 0; i < faults_size && *count < limit; i++) {
		if (faults[i]) {
			*dst++ = faults[i];
			(*count)++;
		}
	}
}

static int fault_get_cur(struct bt_mesh_model *model, u8_t *test_id,
			 u16_t *company_id, u8_t *faults, u8_t *fault_count)
{
	printk("Sending current faults\n");

	*test_id = 0x00;
	*company_id = BT_COMP_ID_LF;

	get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

	return 0;
}

static int fault_get_reg(struct bt_mesh_model *model, u16_t cid,
			 u8_t *test_id, u8_t *faults, u8_t *fault_count)
{
	if (cid != BT_COMP_ID_LF) {
		printk("Faults requested for unknown Company ID 0x%04x\n", cid);
		return -EINVAL;
	}

	printk("Sending registered faults\n");

	*test_id = 0x00;

	get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

	return 0;
}

static int fault_clear(struct bt_mesh_model *model, uint16_t cid)
{
	if (cid != BT_COMP_ID_LF) {
		return -EINVAL;
	}

	memset(reg_faults, 0, sizeof(reg_faults));

	return 0;
}

static int fault_test(struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t cid)
{
	if (cid != BT_COMP_ID_LF) {
		return -EINVAL;
	}

	if (test_id != 0x00) {
		return -EINVAL;
	}

	return 0;
}

static void attention_on(struct bt_mesh_model *model)
{
	printk("attention_on()\n");
}

static void attention_off(struct bt_mesh_model *model)
{
	printk("attention_off()\n");
}

static void vnd_callback(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	printk("src 0x%04x\n", ctx->addr);

	if (ctx->addr == bt_mesh_model_elem(model)->addr) {
		return;
	}
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.fault_get_cur = fault_get_cur,
	.fault_get_reg = fault_get_reg,
	.fault_clear = fault_clear,
	.fault_test = fault_test,
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

static struct bt_mesh_cfg_cli cfg_cli = {
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, CUR_FAULTS_MAX);

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op,
			  &gen_onoff_pub_srv, &onoff_state[0]),
};


static const struct bt_mesh_model_op vnd_ops[] = {
	{ OP_VENDOR_ACTION, 0, vnd_callback },
	BT_MESH_MODEL_OP_END,
};


static struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND(BT_COMP_ID, MOD_ID, vnd_ops, NULL, NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vnd_models),
};

static const struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static void configure(void)
{
	printk("Configuring...\n");

	/* Add Application Key */
	bt_mesh_cfg_app_key_add(net_idx, addr, net_idx, app_idx, app_key, NULL);

	/* Bind to vendor model */
	bt_mesh_cfg_mod_app_bind_vnd(net_idx, addr, addr, app_idx,
		MOD_ID, BT_COMP_ID, NULL);

	/* Bind to Health model */
	bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx,
		BT_MESH_MODEL_ID_HEALTH_SRV, NULL);

	bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx,
		BT_MESH_MODEL_ID_GEN_ONOFF_SRV, NULL);

	/* Add model subscription */
	bt_mesh_cfg_mod_sub_add_vnd(net_idx, addr, addr, GROUP_ADDR,
				    MOD_ID, BT_COMP_ID, NULL);

	struct bt_mesh_cfg_hb_pub pub = {
		.dst = GROUP_ADDR,
		.count = 0xff,
		.period = 0x05,
		.ttl = 0x07,
		.feat = 0,
		.net_idx = net_idx,
	};

	bt_mesh_cfg_hb_pub_set(net_idx, addr, &pub, NULL);
	printk("Publishing heartbeat messages\n");
	printk("Configuration complete\n");

}

static const u8_t dev_uuid[16] = { 0xdd, 0xdd };

static void prov_complete(u16_t net_idx, u16_t addr)
{
	printk("Local node provisioned, net_idx 0x%04x address 0x%04x\n",
	       net_idx, addr);
	net.net_idx = net_idx,
	net.local = addr;
	net.dst = addr;
}

static void prov_reset(void)
{
	printk("The local node has been reset and needs reprovisioning\n");
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	printk("OOB Number: %u\n", number);
	return 0;
}

static int output_string(const char *str)
{
	printk("OOB String: %s\n", str);
	return 0;
}

static const char *bearer2str(bt_mesh_prov_bearer_t bearer)
{
	switch (bearer) {
	case BT_MESH_PROV_ADV:
		return "PB-ADV";
	case BT_MESH_PROV_GATT:
		return "PB-GATT";
	default:
		return "unknown";
	}
}

static void link_open(bt_mesh_prov_bearer_t bearer)
{
	printk("Provisioning link opened on %s\n", bearer2str(bearer));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	printk("Provisioning link closed on %s\n", bearer2str(bearer));
}

static int input(bt_mesh_input_action_t act, u8_t size)
{
	switch (act) {
	case BT_MESH_ENTER_NUMBER:
		printk("Enter a number (max %u digits) with: input-num <num>\n",
		       size);
		break;
	case BT_MESH_ENTER_STRING:
		printk("Enter a string (max %u chars) with: input-str <str>\n",
		       size);
		break;
	default:
		printk("Unknown input action %u (size %u) requested!\n",
		       act, size);
		return -EINVAL;
	}

	input_act = act;
	input_size = size;
	return 0;
}


static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.link_open = link_open,
	.link_close = link_close,
	.complete = prov_complete,
	.reset = prov_reset,
	.static_val = NULL,
	.static_val_len = 0,
	.output_size = 6,
	.output_actions = (BT_MESH_DISPLAY_NUMBER | BT_MESH_DISPLAY_STRING),
	.output_number = output_number,
	.output_string = output_string,
	.input_size = 6,
	.input_actions = (BT_MESH_ENTER_NUMBER | BT_MESH_ENTER_STRING),
	.input = input,
};


static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	printk("Mesh initialized\n");

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		printk("Loading stored settings\n");
		settings_load();
	}

	err = bt_mesh_provision(net_key, net_idx, flags, iv_index, addr,
				dev_key);
	if (err == -EALREADY) {
		printk("Using stored settings\n");
	} else if (err) {
		printk("Provisioning failed (err %d)\n", err);
		return;
	} else {
		printk("Provisioning completed\n");
		configure();
	}
}

static u16_t target = GROUP_ADDR;


void vendor_net_send(void)
{
	NET_BUF_SIMPLE_DEFINE(msg, 2 + 6 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = sys_le16_to_cpu(BT_MESH_ADDR_ALL_NODES),
		.send_ttl = 5,
	};
	u8_t data[] = {0x88, 0x66, 0x88};
	int err;

	printk("ttl 0x%02x dst 0x%04x payload_len %d\n", ctx.send_ttl,
		    ctx.addr, sizeof(data));

	bt_mesh_model_msg_init(&msg, OP_VENDOR_ACTION);
	net_buf_simple_add_mem(&msg, data, sizeof(data));

	err = bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL);
	if (err) {
		SYS_LOG_ERR("Failed to send (err %d)", err);
	}
}


void vendor_message_send(void)
{
	NET_BUF_SIMPLE_DEFINE(msg, 2 + 6 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = app_idx,
		.addr = target,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};

	/* Bind to Health model */
	bt_mesh_model_msg_init(&msg, 0x88);

	if (bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL)) {
		printk("Unable to send Vendor message\n");
	}

	printk("Vendor message sent with OpCode 0x%08x\n", 0x88);
}


void vendor_led_on_send(void)
{
	NET_BUF_SIMPLE_DEFINE(msg, 2 + 1 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = app_idx,
		.addr = 0xFFFF,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GEN_ONOFF_SET);
	net_buf_simple_add_u8(&msg, 1);
	if (bt_mesh_model_send(&root_models[4], &ctx, &msg, NULL, NULL)) {
		BTE("Unable to send On Off Status response");
	}

}

void mesh_enable(void)
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
}

int cmd_led(int argc, char *argv[])
{
	if (argc < 2) {
		printk("%s, argc: %d", __func__, argc);
		return -1;
	}
	int status = strtoul(argv[1], NULL, 0);

	switch (status) {
		case 0:
			gpio_pin_write(gpio, LED1_GPIO_PIN1, 1);
			gpio_pin_write(gpio, LED2_GPIO_PIN3, 1);
			gpio_pin_write(gpio, LED3_GPIO_PIN2, 1);
			break;
		case 1:
			gpio_pin_write(gpio, LED1_GPIO_PIN1, 0);
			gpio_pin_write(gpio, LED2_GPIO_PIN3, 0);
			gpio_pin_write(gpio, LED3_GPIO_PIN2, 0);
			break;

	}
	return 0;
}


void button_pressed_worker(struct k_work *unused)
{
	if (led_state) {
		BTD("LED OFF\n");
		led_state = 0;
		gpio_pin_write(gpio, LED1_GPIO_PIN1, 1);
	} else {
		BTD("LED ON\n");
		led_state = 1;
		gpio_pin_write(gpio, LED1_GPIO_PIN1, 0);
	}

	NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = app_idx,
		.addr = 0xFFFF,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GEN_ONOFF_SET);
	net_buf_simple_add_le16(&msg, led_state);
	if (bt_mesh_model_send(&root_models[4], &ctx, &msg, NULL, NULL)) {
		BTE("Unable to send On Off Status response");
	}

}

static void button_pressed(struct device *dev, struct gpio_callback *cb,
			   u32_t pins)
{
	u32_t value = 0;
	gpio_pin_read(gpio, BUTTON1_GPIO_PIN0, &value);
	if (value == -1)
		k_work_submit(&button_work);
}



void test() {
	BTD("%s\n", __func__);
}


void mesh_init(void)
{
	static struct gpio_callback button_cb;

	BTD("%s\n", __func__);
	gpio = device_get_binding(GPIO_PORT0);
	uwp_aon_irq_enable(AON_INT_GPIO0);
	gpio_pin_configure(gpio, BUTTON1_GPIO_PIN0,
			   (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE
			   | GPIO_PUD_PULL_UP
			   | GPIO_INT_ACTIVE_LOW));
	unsigned int *p = 0x40840000 + 0x10;
	*p = *p | (1 << 8);

	gpio_init_callback(&button_cb, button_pressed,
			   BIT(BUTTON1_GPIO_PIN0));
	gpio_add_callback(gpio, &button_cb);
	gpio_pin_enable_callback(gpio, BUTTON1_GPIO_PIN0);

	gpio_pin_configure(gpio, LED1_GPIO_PIN1, 1);
	gpio_pin_configure(gpio, LED2_GPIO_PIN3, 1);
	gpio_pin_configure(gpio, LED3_GPIO_PIN2, 1);

	addr = sys_rand32_get() & 0x7F;

	k_work_init(&button_work, button_pressed_worker);

	//mesh_enable();

	gpio_pin_write(gpio, LED1_GPIO_PIN1, 0);
	k_sleep(800);
	gpio_pin_write(gpio, LED1_GPIO_PIN1, 1);
	k_sleep(800);
	gpio_pin_write(gpio, LED1_GPIO_PIN1, 0);
	k_sleep(800);
	gpio_pin_write(gpio, LED1_GPIO_PIN1, 1);


}


void mesh_info(void)
{
	BTD("%s\n", __func__);
	BTI("Current addr: 0x%02X\n", addr);
	//unsigned int *p = 0x40840000;
	//BTI("GPIO 0: 0x%08X\n", *(p + 0x10));
	//BTI("GPIO 1: 0x%08X\n", *(p + 0x14));
	//BTI("GPIO 2: 0x%08X\n", *(p + 0x18));
	//BTI("GPIO 3: 0x%08X\n", *(p + 0x1C));
	//BTI("REST 3: 0x%08X\n", *(p + 0x64));
}

int cmd_mesh(int argc, char *argv[])
{
	if (argc < 2) {
		printk("mesh argc miss: %d\n", argc);
		return -1;
	}

	if (!strcmp(argv[1], "on")) {
		mesh_enable();
	} else if (!strcmp(argv[1], "config")) {
		//configure();
	} else if (!strcmp(argv[1], "net_tx")) {
		vendor_net_send();
	} else if (!strcmp(argv[1], "msg_tx")) {
		vendor_message_send();
	} else if (!strcmp(argv[1], "led")) {
		vendor_led_on_send();
	} else if (!strcmp(argv[1], "info")) {
		mesh_info();
	} else if (!strcmp(argv[1], "test")) {
		test();
	} else {
		printk("error\n");
	}
	return 0;
}
