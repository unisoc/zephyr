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
#include "health.h"
#include "uwp_hal.h"

static u8_t cur_faults[CUR_FAULTS_MAX];
static u8_t reg_faults[CUR_FAULTS_MAX * 2];

static void hl_get_faults(u8_t *faults, u8_t faults_size, u8_t *dst, u8_t *count)
{
	u8_t i, limit = *count;

	for (i = 0, *count = 0; i < faults_size && *count < limit; i++) {
		if (faults[i]) {
			*dst++ = faults[i];
			(*count)++;
		}
	}
}

static int hl_fault_get_cur(struct bt_mesh_model *model, u8_t *test_id,
			 u16_t *company_id, u8_t *faults, u8_t *fault_count)
{
	printk("Sending current faults\n");

	*test_id = 0x00;
	*company_id = BT_COMP_ID_LF;

	hl_get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

	return 0;
}

static int hl_fault_get_reg(struct bt_mesh_model *model, u16_t cid,
			 u8_t *test_id, u8_t *faults, u8_t *fault_count)
{
	if (cid != BT_COMP_ID_LF) {
		printk("Faults requested for unknown Company ID 0x%04x\n", cid);
		return -EINVAL;
	}

	printk("Sending registered faults\n");

	*test_id = 0x00;

	hl_get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

	return 0;
}

static int hl_fault_clear(struct bt_mesh_model *model, uint16_t cid)
{
	if (cid != BT_COMP_ID_LF) {
		return -EINVAL;
	}

	memset(reg_faults, 0, sizeof(reg_faults));

	return 0;
}

static int hl_fault_test(struct bt_mesh_model *model, uint8_t test_id,
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

static void hl_attention_on(struct bt_mesh_model *model)
{
	printk("attention_on()\n");
}

static void hl_attention_off(struct bt_mesh_model *model)
{
	printk("attention_off()\n");
}


static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.fault_get_cur = hl_fault_get_cur,
	.fault_get_reg = hl_fault_get_reg,
	.fault_clear = hl_fault_clear,
	.fault_test = hl_fault_test,
	.attn_on = hl_attention_on,
	.attn_off = hl_attention_off,
};

struct bt_mesh_health_srv unisoc_health_srv = {
	.cb = &health_srv_cb,
};


void health_init(void)
{
	BTI("%s\n", __func__);
	/* Bind to Health model */
	bt_mesh_cfg_mod_app_bind(get_net_idx(), get_addr(), get_addr(), get_app_idx(),
		BT_MESH_MODEL_ID_HEALTH_SRV, NULL);

	struct bt_mesh_cfg_hb_pub pub = {
		.dst = GROUP_ADDR,
		.count = 0xff,
		.period = 0x05,
		.ttl = 0x07,
		.feat = 0,
		.net_idx = get_net_idx(),
	};

	bt_mesh_cfg_hb_pub_set(get_net_idx(), get_addr(), &pub, NULL);
	printk("Publishing heartbeat messages\n");

}

