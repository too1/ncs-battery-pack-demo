#ifndef __APP_BLUETOOTH_H
#define __APP_BLUETOOTH_H

#include <zephyr.h>

int app_bt_init(void);

int app_bt_send(uint8_t *data_ptr, uint16_t length);

#endif
