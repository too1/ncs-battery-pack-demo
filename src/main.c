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
#include <zephyr/drivers/gpio.h>
#include <app_pmic.h>

#include <zephyr/logging/log.h>

static const struct gpio_dt_spec app_led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

void main(void)
{
	int ret;
	
	ret = gpio_pin_configure_dt(&app_led0, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return;
	}

	app_pmic_init();

	printk("Battery pack demo started\n");

	while (1) {
		gpio_pin_toggle_dt(&app_led0);
		k_msleep(100);
	}
}
