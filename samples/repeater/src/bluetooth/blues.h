#ifndef __UNISOC_BLUES_H_
#define __UNISOC_BLUES_H_

void blues_init(void);

typedef struct {
    uint32_t  role;
    uint8_t  address[6];
    uint8_t  auto_run;
    uint8_t  net_key[16];
    uint8_t  device_key[16];
    uint8_t  app_key[16];
    uint16_t  node_address;
}blues_config_t;


#define DEVICE_ROLE_MESH 1

#endif
