#ifndef __APP_LED_H
#define __APP_LED_H

#include <zephyr.h>

#define APP_LED_STATUS 	0
#define APP_LED_PMIC 	1
#define APP_LED_BT		2

int app_led_init(void);

int app_led_on(int led_index);

int app_led_off(int led_index);

int app_led_toggle(int led_index);

#endif
