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

#include <tinycrypt/sha256.h>
#include <tinycrypt/constants.h>


#include "../../../../drivers/bluetooth/unisoc/uki_utlis.h"
#include "../../../../drivers/bluetooth/unisoc/uki_config.h"
#include "mesh.h"
#include "blues.h"
#include "light.h"
#include "health.h"
#include "uwp_hal.h"

#if !defined(NODE_ADDR)
#define NODE_ADDR 0x0b0c
#endif

static const u8_t net_key[16] = {
	0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t dev_key[16] = {
	0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t app_key[16] = {
	0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x88, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static struct {
	u16_t local;
	u16_t dst;
	u16_t net_idx;
	u16_t app_idx;
} local_net = {
	.local = BT_MESH_ADDR_UNASSIGNED,
	.dst = BT_MESH_ADDR_UNASSIGNED,
};


static const u16_t net_idx;
static const u16_t app_idx;
static const u32_t iv_index;
static u8_t flags;
static u16_t addr = GROUP_ADDR;
static u16_t target = GROUP_ADDR;

static bt_mesh_input_action_t input_act;
static u8_t input_size;

static const u8_t dev_uuid[16] = {0};

extern blues_config_t blues_config;

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

static void vnd_callback(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	printk("src 0x%04x\n", ctx->addr);

	if (ctx->addr == bt_mesh_model_elem(model)->addr) {
		return;
	}
}


static struct bt_mesh_cfg_cli cfg_cli = {
};



extern const struct bt_mesh_model_op unisoc_light_onoff_srv_op[];
BT_MESH_MODEL_PUB_DEFINE(unisoc_light_onoff_pub_srv, NULL, 2 + 2);

extern struct bt_mesh_health_srv unisoc_health_srv;
BT_MESH_HEALTH_PUB_DEFINE(unisoc_health_pub, CUR_FAULTS_MAX);

extern const struct bt_mesh_model_op lightness_srv_op[];
BT_MESH_HEALTH_PUB_DEFINE(lightness_srv_pub, CUR_FAULTS_MAX);

extern const struct bt_mesh_model_op lightness_setup_srv_op[];
BT_MESH_HEALTH_PUB_DEFINE(lightness_setup_srv_pub, CUR_FAULTS_MAX);

extern struct onoff_state unisoc_leds[];

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&unisoc_health_srv, &unisoc_health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, unisoc_light_onoff_srv_op,
			  &unisoc_light_onoff_pub_srv, &unisoc_leds[0]),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV, lightness_srv_op,
    &lightness_srv_pub, NULL),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV, lightness_setup_srv_op,
    &lightness_setup_srv_pub, NULL),
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
	BTI("Configuring...\n");

	/* Add Application Key */
	bt_mesh_cfg_app_key_add(net_idx, addr, net_idx, app_idx, app_key, NULL);

	/* Bind to vendor model */
	bt_mesh_cfg_mod_app_bind_vnd(net_idx, addr, addr, app_idx,
		MOD_ID, BT_COMP_ID, NULL);


	bt_mesh_cfg_mod_app_bind(net_idx, addr, addr, app_idx,
		BT_MESH_MODEL_ID_GEN_ONOFF_SRV, NULL);

	/* Add model subscription */
	bt_mesh_cfg_mod_sub_add_vnd(net_idx, addr, addr, GROUP_ADDR,
				    MOD_ID, BT_COMP_ID, NULL);
	BTV("Configuration complete\n");

}

static void prov_complete(u16_t net_idx, u16_t addr)
{
	BTI("Local node provisioned, net_idx 0x%04x address 0x%04x\n",
	       net_idx, addr);
	local_net.net_idx = net_idx,
	local_net.local = addr;
	//local_net.dst = addr;

	if (blues_config.profile_health_enabled)
		health_init();

	if (blues_config.profile_light_enabled)
		light_init();
}

static void prov_reset(void)
{
	printk("The local node has been reset and needs reprovisioning\n");
	bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
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

//static u8_t static_auth[] = {0xbe, 0xb5, 0xb9, 0xc9, 0x98, 0x2d, 0x9b, 0x08,
//				 0x79, 0x4b, 0x72, 0xbd, 0x83, 0xc6, 0x12, 0x1a};
static u8_t static_auth[16] = {0};

static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.link_open = link_open,
	.link_close = link_close,
	.complete = prov_complete,
	.reset = prov_reset,
	.static_val = static_auth,
	.static_val_len = sizeof(static_auth),
	.output_size = 0,
	.output_actions = 0,
	.input_size = 0,
	.input_actions = 0,
	.output_number = output_number,
	.output_string = output_string,
	.input = input,
};

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

static void alimesh_init(void)
{
	static struct tc_sha256_state_struct sha256_ctx;
	uint8_t *p = dev_uuid;
	int ret;
	char static_value[100] = {0};
	uint8_t result[100] = {0};

	UINT16_TO_STREAM(p, ALI_COMP_ID);
	*p++ = 0x71;
	UINT32_TO_STREAM(p, blues_config.ali_pid);
	sys_memcpy_swap(p, blues_config.ali_mac, sizeof(blues_config.ali_mac));
	p += sizeof(blues_config.ali_mac);
	memset(p, 0, dev_uuid + sizeof(dev_uuid) - p);
	uki_hexdump("UUID:", dev_uuid, sizeof(dev_uuid));

	p = static_value;

	ret = snprintf(p, sizeof(static_value),
		"%08x,", blues_config.ali_pid);
	p += ret;
	uki_hex(p, blues_config.ali_mac, sizeof(blues_config.ali_mac));
	p += sizeof(blues_config.ali_mac) * 2;

	*p++ = ',';

	uki_hex(p, blues_config.ali_secret, sizeof(blues_config.ali_secret));
	BTE("ALI KEY: %s\n", static_value);

	ret = tc_sha256_init(&sha256_ctx);
    if (ret != TC_CRYPTO_SUCCESS) {
        BTE("sha256 init fail\n");
    }

    ret = tc_sha256_update(&sha256_ctx, static_value, strlen(static_value));
    if (ret != TC_CRYPTO_SUCCESS) {
        BTE("sha256 udpate fail\n");
    }

    ret = tc_sha256_final(result, &sha256_ctx);
    if (ret != TC_CRYPTO_SUCCESS) {
        BTE("sha256 final fail\n");
    }

	memcpy(static_auth, result, sizeof(static_auth));

	uki_hexdump("static value:", static_auth, sizeof(static_auth));
}


static void mesh_enable(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	if (blues_config.ali_mesh) {
		alimesh_init();
	}

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

	if (blues_config.mesh_proved) {
		BTI("provision local node\n");

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
	} else {
		bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
	}

}

void test() {
	BTD("%s\n", __func__);
	do {
		u16_t value = sys_rand32_get() & 0x7FFF;
		BTI("0x%04X\n", value);
		k_sleep(100);
	} while (1);
}


void mesh_init(void)
{
	int err;

	BTI("%s\n", __func__);

	addr = sys_rand32_get() & 0x7FFF;
	BTI("Node address: 0x%04X\n", addr);

	err = bt_enable(mesh_enable);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
}

u16_t get_net_idx(void)
{
	return net_idx;
}

u16_t get_app_idx(void)
{
	return app_idx;
}

u32_t get_iv_index(void)
{
	return iv_index;
}

u16_t get_addr(void)
{
	return addr;
}

struct bt_mesh_model *get_root_modules(void)
{
	return &root_models[0];
}


void mesh_info(void)
{
	BTD("%s\n", __func__);
	BTI("Current addr: 0x%02X\n", addr);
}

int cmd_mesh(int argc, char *argv[])
{
	if (argc < 2) {
		printk("mesh argc miss: %d\n", argc);
		return -1;
	}

	if (!strcmp(argv[1], "on")) {
		mesh_init();
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
