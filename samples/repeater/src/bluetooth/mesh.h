#ifndef __UNISOC_MESH_H__
#define __UNISOC_MESH_H__

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/mesh.h>
#include <tinycrypt/hmac_prng.h>


int cmd_mesh(int argc, char *argv[]);
int cmd_led(int argc, char *argv[]);
void mesh_init(void);

u16_t get_net_idx(void);
u16_t get_app_idx(void);
u32_t get_iv_index(void);
struct bt_mesh_model *get_root_modules(void);


#endif
