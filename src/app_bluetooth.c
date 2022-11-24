#include <app_bluetooth.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <bluetooth/services/nus.h>

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME app_bt
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN	(sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

struct nus_message_type_t {
	uint16_t length;
	uint8_t buf[NUS_STRING_LEN_MAX];
};

K_MSGQ_DEFINE(m_msgq_nus_tx, sizeof(struct nus_message_type_t), BT_TX_MSG_COUNT, 4);

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	LOG_INF("Bluetooth data received");
}

static struct bt_nus_cb nus_cb = {
	.received = bt_receive_cb,
};

int app_bt_init(void)
{
	int ret;

	ret = bt_enable(NULL);
	if (ret < 0) return ret;

	LOG_INF("Bluetooth initialized");

	ret = bt_nus_init(&nus_cb);
	if (ret < 0) {
		LOG_ERR("Failed to initialize UART service (err: %d)", ret);
		return ret;
	}

	ret = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (ret < 0) {
		LOG_ERR("Advertising failed to start (err %d)", ret);
		return ret;
	}

	return 0;
}

int app_bt_send(uint8_t *data_ptr, uint16_t length)
{
	struct nus_message_type_t new_message;

	new_message.length = length;
	memcpy(new_message.buf, data_ptr, length);

	if (k_msgq_put(&m_msgq_nus_tx, (void *)&new_message, K_NO_WAIT) != 0) {
		return -ENOMEM;
	}

	return 0;
}

static void bt_tx_thread_func(void)
{
	struct nus_message_type_t new_message;
	
	while(1) {
		k_msgq_get(&m_msgq_nus_tx, (void *)&new_message, K_FOREVER);

		bt_nus_send(0, new_message.buf, new_message.length);
	}
}

K_THREAD_DEFINE(m_bt_tx_thread, BT_TX_THREAD_STACKSIZE, bt_tx_thread_func,
				NULL, NULL, NULL, BT_TX_THREAD_PRIORITY, 0, 0);