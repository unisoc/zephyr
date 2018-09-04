#ifndef __UNISOC_HEALTH_H__
#define __UNISOC_HEALTH_H__

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/mesh.h>

#define CUR_FAULTS_MAX 4

void health_init(void);

#endif
