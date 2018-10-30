#ifndef __UNISOC_BLUES_H_
#define __UNISOC_BLUES_H_

void blues_init(void);

typedef struct {
    uint32_t  role;
    uint8_t  address[6];
    uint8_t  auto_run;
    uint8_t  mesh_proved;
    uint8_t  profile_health_enabled;
    uint8_t  profile_light_enabled;
    uint8_t  net_key[16];
    uint8_t  device_key[16];
    uint8_t  app_key[16];
    uint16_t  node_address;
    uint8_t  firmware_log_mode;

/*
* Ali Genie
*/
	uint8_t ali_mesh;
	uint32_t ali_pid;
	uint8_t ali_mac[6];
    uint8_t  ali_secret[16];
}blues_config_t;


#define DEVICE_ROLE_MESH 1
#define DEVICE_ROLE_BLUES 2


#endif
