#ifndef __UNISOC_MESH_H__
#define __UNISOC_MESH_H__

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/mesh.h>
#include <tinycrypt/hmac_prng.h>

#define GROUP_ADDR 0xc000
#define PUBLISHER_ADDR  0x000f

#define BT_COMP_ID           0x01EC
#define ALI_COMP_ID           0x01A8
#define MOD_ID 0x1688
#define OP_VENDOR_ACTION BT_MESH_MODEL_OP_3(0x00, BT_COMP_ID)


int cmd_mesh(int argc, char *argv[]);
int cmd_led(int argc, char *argv[]);
void mesh_init(void);

u16_t get_net_idx(void);
u16_t get_app_idx(void);
u32_t get_iv_index(void);
struct bt_mesh_model *get_root_modules(void);
u16_t get_addr(void);


#endif
