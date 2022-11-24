/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*************************************************************/
/* API is not stable and maturity level of SW is evaluation. */
/*************************************************************/

#include <errno.h>
#include <device.h>
#include <zephyr.h>
#include <stdio.h>
#include <app_led.h>
#include <app_pmic.h>
#include <app_bluetooth.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define APP_BT_CMD_READ_BAT_VOLTAGE "ReadBV"
#define APP_BT_CMD_RESET			"Reset"
#define APP_BT_CMD_SET_BUCK_VTG		"SetV"
#define IS_APP_BT_CMD(a, b) (strncmp(a, b, strlen(b)) == 0)

void bt_printf(const char *str, ...)
{
	static uint8_t tmpstring[NUS_STRING_LEN_MAX];

    va_list myargs;
    va_start(myargs, str);
	vsprintf(tmpstring, str, myargs);
    va_end(myargs);

	if (app_bt_send(tmpstring, strlen(tmpstring)) < 0) {
		LOG_ERR("Unable to send data to the NUS service");
	}
}

void pmic_callback(app_pmic_evt_t *evt)
{
	bt_printf("PMIC Evt: %s", pmic_state_name_strings[evt->type]);
}

void process_incoming_nus_data(app_bt_evt_t *bt_evt)
{
	if (IS_APP_BT_CMD(bt_evt->buf, APP_BT_CMD_READ_BAT_VOLTAGE)) {
		LOG_INF("Read battery voltage BT command received");
		uint16_t bat_voltage = app_pmic_get_battery_voltage();
		bt_printf("Battery voltage: %i mV", bat_voltage);
	} else if (IS_APP_BT_CMD(bt_evt->buf, APP_BT_CMD_SET_BUCK_VTG)) {
		int decivolt = (bt_evt->buf[strlen(APP_BT_CMD_SET_BUCK_VTG)] - '0') * 10 +
					   (bt_evt->buf[strlen(APP_BT_CMD_SET_BUCK_VTG) + 1] - '0');
		LOG_INF("Attempting to set buck out to %i decivolt", decivolt);
		app_pmic_set_buck_out_voltage(decivolt);
	} else if (IS_APP_BT_CMD(bt_evt->buf, APP_BT_CMD_RESET)) {
		LOG_INF("Resetting....");
		k_msleep(50);
		sys_reboot(SYS_REBOOT_WARM);
	}
}

void bluetooth_callback(app_bt_evt_t *bt_evt)
{
	switch(bt_evt->type) {
		case APP_BT_EVT_CONNECTED:
			bt_printf("Hello mister");
			break;
		case APP_BT_EVT_DISCONNECTED:
			break;
		case APP_BT_EVT_NUS_DATA_RECEIVED:
			process_incoming_nus_data(bt_evt);
			break;
	}
}

void main(void)
{
	int ret;
	
	ret = app_led_init();
	if (ret < 0) return;

	ret = app_pmic_init(pmic_callback);
	if(ret == 0) app_led_on(APP_LED_PMIC);
	else app_led_off(APP_LED_PMIC);
	
	ret = app_bt_init(bluetooth_callback);
	if (ret < 0) {
		LOG_ERR("Failed to initialize Bluetooth: %i", ret);
		return;
	}

	LOG_INF("Battery pack demo started");

	while (1) {
		app_led_toggle(APP_LED_STATUS);
		k_msleep(100);
	}
}
