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
	return bt_nus_send(0, data_ptr, length);
}