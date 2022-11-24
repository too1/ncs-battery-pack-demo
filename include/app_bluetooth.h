#ifndef __APP_BLUETOOTH_H
#define __APP_BLUETOOTH_H

#include <zephyr.h>

#define NUS_STRING_LEN_MAX 		128
#define BT_TX_MSG_COUNT			8
#define BT_TX_THREAD_STACKSIZE	1024
#define BT_TX_THREAD_PRIORITY	5

int app_bt_init(void);

int app_bt_send(uint8_t *data_ptr, uint16_t length);

#endif
