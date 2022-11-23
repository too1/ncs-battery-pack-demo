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
#include <app_led.h>
#include <app_pmic.h>

#include <zephyr/logging/log.h>

void main(void)
{
	int ret;
	
	ret = app_led_init();
	if (ret < 0) return;

	ret = app_pmic_init();
	if(ret == 0) app_led_on(APP_LED_PMIC);
	else app_led_off(APP_LED_PMIC);

	printk("Battery pack demo started\n");

	while (1) {
		app_led_toggle(APP_LED_STATUS);
		k_msleep(100);
	}
}
