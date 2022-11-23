#include <app_led.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec m_app_led[] = {GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
											  GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios)};
#define APP_LED_NUM_LEDS (sizeof(m_app_led) / sizeof(m_app_led[0]))

int app_led_init(void)
{
	int ret;
	for(int i = 0; i < APP_LED_NUM_LEDS; i++) {
		ret = gpio_pin_configure_dt(&m_app_led[i], GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int app_led_on(int led_index)
{
	if(led_index >= APP_LED_NUM_LEDS) return -EINVAL;
	return gpio_pin_set_dt(&m_app_led[led_index], 1);
}

int app_led_off(int led_index)
{
	if(led_index >= APP_LED_NUM_LEDS) return -EINVAL;
	return gpio_pin_set_dt(&m_app_led[led_index], 1);
}

int app_led_toggle(int led_index)
{
	if(led_index >= APP_LED_NUM_LEDS) return -EINVAL;
	return gpio_pin_toggle_dt(&m_app_led[led_index]);
}