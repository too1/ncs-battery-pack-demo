#ifndef __APP_BLUETOOTH_H
#define __APP_BLUETOOTH_H

#include <zephyr.h>

#define NUS_STRING_LEN_MAX 		128
#define BT_TX_MSG_COUNT			8
#define BT_TX_THREAD_STACKSIZE	1024
#define BT_TX_THREAD_PRIORITY	5

typedef enum {APP_BT_EVT_CONNECTED, APP_BT_EVT_DISCONNECTED, APP_BT_EVT_NUS_DATA_RECEIVED} app_bt_evt_type_t;

typedef struct {
	int type;
	const uint8_t *buf;
	uint16_t length;
} app_bt_evt_t;

typedef void (*app_bt_callback_t)(app_bt_evt_t *bt_evt);

int app_bt_init(app_bt_callback_t callback);

int app_bt_send(uint8_t *data_ptr, uint16_t length);

#endif
