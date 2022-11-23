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

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

int send_type = 0;

static void send_bt_packet(struct k_work *item)
{
	uint8_t string[20];
	sprintf(string, "%i", send_type);
	app_bt_send(string, strlen(string));
}

K_WORK_DEFINE(work_send_bt_packet, send_bt_packet);

void pmic_callback(app_pmic_evt_t *evt)
{
	send_type = evt->type;
	k_work_submit(&work_send_bt_packet);
}

void main(void)
{
	int ret;
	
	ret = app_led_init();
	if (ret < 0) return;

	ret = app_pmic_init(pmic_callback);
	if(ret == 0) app_led_on(APP_LED_PMIC);
	else app_led_off(APP_LED_PMIC);
	
	ret = app_bt_init();
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
