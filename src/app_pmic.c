
#include <app_pmic.h>
#include <npmx_driver.h>
#include <npmx_gpio.h>
#include <npmx_core.h>
#include <npmx_charger.h>
#include <npmx_adc.h>
#include <npmx_errlog.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME pmic_charger
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static app_pmic_callback_t m_callback;

static uint16_t m_battery_voltage_mv = 0;

/** @brief Possible events from requested nPM device. */
typedef enum {
	APP_CHARGER_EVENT_BATTERY_DETECTED, /** Event registered when battery connection detected. */
	APP_CHARGER_EVENT_BATTERY_REMOVED, /** Event registered when battery connection removed. */
	APP_CHARGER_EVENT_VBUS_DETECTED, /** Event registered when VBUS connection detected. */
	APP_CHARGER_EVENT_VBUS_REMOVED, /** Event registered when VBSU connection removed. */
	APP_CHARGER_EVENT_CHARGING_TRICKE_STARTED, /** Event registered when trickle charging started. */
	APP_CHARGER_EVENT_CHARGING_CC_STARTED, /** Event registered when constant current charging started. */
	APP_CHARGER_EVENT_CHARGING_CV_STARTED, /** Event registered when constant voltage charging started. */
	APP_CHARGER_EVENT_CHARGING_COMPLETED, /** Event registered when charging completed. */
	APP_CHARGER_EVENT_BATTERY_LOW_ALERT1, /** Event registered when first low battery voltage alert detected. */
	APP_CHARGER_EVENT_BATTERY_LOW_ALERT2, /** Event registered when second low battery voltage alert detected. */
} npm1300_charger_event_t;

const char *pmic_state_name_strings[] =  {"Battery Detected", 
									"Battery Removed", 
									"VBUS Detected", 
									"VBUS Removed", 
									"Charging Trickle Started", 
									"Charging CC Started", 
									"Charging CV Started", 
									"Charging Completed", 
									"Bat Low Alert 1", 
									"Bat Low Alert 2"};

/**
 * @brief Register the new event received from nPM device.
 *
 * @param[in] event New event type.
 */
static void register_state_change(npm1300_charger_event_t event)
{
	static app_pmic_evt_t app_event;
	switch(event) {
		case APP_CHARGER_EVENT_BATTERY_DETECTED:
			LOG_INF("0 APP_CHARGER_EVENT_BATTERY_DETECTED");
			break;
		case APP_CHARGER_EVENT_BATTERY_REMOVED:
			LOG_INF("1 APP_CHARGER_EVENT_BATTERY_REMOVED");
			break;
		case APP_CHARGER_EVENT_VBUS_DETECTED: 
			LOG_INF("2 APP_CHARGER_EVENT_VBUS_DETECTED");
			break;
		case APP_CHARGER_EVENT_VBUS_REMOVED: 
			LOG_INF("3 APP_CHARGER_EVENT_VBUS_REMOVED");
			break;
		case APP_CHARGER_EVENT_CHARGING_TRICKE_STARTED: 
			LOG_INF("4 APP_CHARGER_EVENT_CHARGING_TRICKE_STARTED");
			break; 
		case APP_CHARGER_EVENT_CHARGING_CC_STARTED:
			LOG_INF("5 APP_CHARGER_EVENT_CHARGING_CC_STARTED");
			break;
		case APP_CHARGER_EVENT_CHARGING_CV_STARTED:
			LOG_INF("6 APP_CHARGER_EVENT_CHARGING_CV_STARTED");
			break;
		case APP_CHARGER_EVENT_CHARGING_COMPLETED:
			LOG_INF("7 APP_CHARGER_EVENT_CHARGING_COMPLETED");
			break;
		case APP_CHARGER_EVENT_BATTERY_LOW_ALERT1:
			LOG_INF("8 APP_CHARGER_EVENT_BATTERY_LOW_ALERT1");
			break;
		case APP_CHARGER_EVENT_BATTERY_LOW_ALERT2:
		default:
			LOG_INF("event %i", event);
			break;
	}
	app_event.type = event;
	m_callback(&app_event);
}

/**
 * @brief Function callback for vbusin events.
 *
 * @param[in] p_pm The pointer to the instance of nPM device.
 * @param[in] type The type of callback, should be always NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE.
 * @param[in] mask Received event mask @ref npmx_event_group_vbusin_mask_t .
 */
static void vbusin_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	if (mask & (uint8_t)NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK) {
		register_state_change(APP_CHARGER_EVENT_VBUS_DETECTED);
	}
	else if (mask & (uint8_t)NPMX_EVENT_GROUP_VBUSIN_REMOVED_MASK) {
		register_state_change(APP_CHARGER_EVENT_VBUS_REMOVED);
	}
	else LOG_WRN("Unhandled vbusin callback reveived!");
}

/**
 * @brief Function callback for adc events.
 *
 * @param[in] p_pm The pointer to the instance of nPM device.
 * @param[in] type The type of callback, should be always NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE.
 * @param[in] mask Received event mask @ref npmx_event_group_vbusin_mask_t.
 */
static void adc_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	if ((mask & (uint8_t)NPMX_EVENT_GROUP_ADC_BAT_READY_MASK)) {
		static uint16_t battery_voltage_millivolts_last = 0;
		if (npmx_adc_meas_get(npmx_adc_get(p_pm, 0), NPMX_ADC_MEAS_VBAT,
				      &m_battery_voltage_mv) == NPMX_SUCCESS) {
			if (m_battery_voltage_mv != battery_voltage_millivolts_last) {
				battery_voltage_millivolts_last = m_battery_voltage_mv;
				LOG_INF("Battery:\t %d mV", m_battery_voltage_mv);
			}
			if (m_battery_voltage_mv < CONFIG_BATTERY_VOLTAGE_THRESHOLD_2) {
				register_state_change(APP_CHARGER_EVENT_BATTERY_LOW_ALERT2);
			} else if (m_battery_voltage_mv <
				   CONFIG_BATTERY_VOLTAGE_THRESHOLD_1) {
				register_state_change(APP_CHARGER_EVENT_BATTERY_LOW_ALERT1);
			}
		}
	}
}

/**
 * @brief Function callback for charger status events.
 *
 * @param[in] p_pm Pointer to the instance of nPM device.
 * @param[in] type Type of callback, should be always NPMX_CALLBACK_TYPE_EVENT_BAT_CHAR_STATUS.
 * @param[in] mask Received event mask @ref npmx_event_group_charger_mask_t .
 */
static void charger_status_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	npmx_charger_t *charger_instance = npmx_charger_get(p_pm, 0);

	if (mask & (uint8_t)NPMX_EVENT_GROUP_CHARGER_ERROR_MASK) {
		/* Check charger errors and run default debug callbacks to log error bits. */
		npmx_charger_errors_check(charger_instance);
	}

	npmx_charger_status_mask_t status;

	/* Delay required for status stabilization. */
	k_msleep(5);

	if (npmx_charger_status_get(charger_instance, &status) == NPMX_SUCCESS) {
		if (status & NPMX_CHARGER_STATUS_TRICKLE_CHARGE_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_TRICKE_STARTED);
		}
		else if (status & NPMX_CHARGER_STATUS_CONSTANT_CURRENT_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_CC_STARTED);
		}
		else if (status & NPMX_CHARGER_STATUS_CONSTANT_VOLTAGE_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_CV_STARTED);
		}
		else if (status & NPMX_CHARGER_STATUS_COMPLETED_MASK) {
			register_state_change(APP_CHARGER_EVENT_CHARGING_COMPLETED);
		}
		else LOG_WRN("Unhandled charger status received!!");
	}
}

/**
 * @brief Function callback for battery events.
 *
 * @param[in] p_pm The pointer to the instance of nPM device.
 * @param[in] type The type of callback, should be always NPMX_CALLBACK_TYPE_EVENT_BAT_CHAR_BAT.
 * @param[in] mask Received event mask @ref npmx_event_group_battery_mask_t .
 */
static void charger_battery_callback(npmx_instance_t *p_pm, npmx_callback_type_t type, uint8_t mask)
{
	if (mask & (uint8_t)NPMX_EVENT_GROUP_BATTERY_DETECTED_MASK) {
		register_state_change(APP_CHARGER_EVENT_BATTERY_DETECTED);
	}

	if (mask & (uint8_t)NPMX_EVENT_GROUP_BATTERY_REMOVED_MASK) {
		register_state_change(APP_CHARGER_EVENT_BATTERY_REMOVED);
	}
}

/**
 * @brief Function for returning charger voltage enum associated with given voltage.
 *
 * @param[in] mv Voltage to be translated.
 *
 * @return Charger voltage enum.
 */
static npmx_charger_voltage_t mv_to_charger_voltage_enum(uint32_t mv)
{
	switch (mv) {
	case 3500:
		return NPMX_CHARGER_VOLTAGE_3V50;
	case 3550:
		return NPMX_CHARGER_VOLTAGE_3V55;
	case 3600:
		return NPMX_CHARGER_VOLTAGE_3V60;
	case 3650:
		return NPMX_CHARGER_VOLTAGE_3V65;
	case 4000:
		return NPMX_CHARGER_VOLTAGE_4V00;
	case 4050:
		return NPMX_CHARGER_VOLTAGE_4V05;
	case 4100:
		return NPMX_CHARGER_VOLTAGE_4V10;
	case 4150:
		return NPMX_CHARGER_VOLTAGE_4V15;
	case 4200:
		return NPMX_CHARGER_VOLTAGE_4V20;
	case 4250:
		return NPMX_CHARGER_VOLTAGE_4V25;
	case 4300:
		return NPMX_CHARGER_VOLTAGE_4V30;
	case 4350:
		return NPMX_CHARGER_VOLTAGE_4V35;
	case 4400:
		return NPMX_CHARGER_VOLTAGE_4V40;
	case 4450:
		return NPMX_CHARGER_VOLTAGE_4V45;
	default:
		__ASSERT(false, "Invalid mV value to translate into charger enum");
		return NPMX_CHARGER_VOLTAGE_DEFAULT;
	}
}

int app_pmic_init(app_pmic_callback_t callback)
{
	const struct device *pmic_dev = DEVICE_DT_GET(DT_NODELABEL(npm_0));

	if (!device_is_ready(pmic_dev)) {
		LOG_INF("PMIC device is not ready");
		return -1;
	} else {
		LOG_INF("PMIC device ok");
	}

	m_callback = callback;

	/* Get pointer to npmx device. */
	npmx_instance_t *npmx_instance = &((struct npmx_data *)pmic_dev->data)->npmx_instance;

	/* Get pointer to GPIO 0 instance. */
	npmx_gpio_t *gpio_0 = npmx_gpio_get(npmx_instance, 0);

	/* Get pointer to CHARGER instance. */
	npmx_charger_t *charger_instance = npmx_charger_get(npmx_instance, 0);

	/* Register callback for vbus events. */
	npmx_core_register_cb(npmx_instance, vbusin_callback,
			      NPMX_CALLBACK_TYPE_EVENT_VBUSIN_VOLTAGE);

	/* Register callback for adc events. */
	npmx_core_register_cb(npmx_instance, adc_callback, NPMX_CALLBACK_TYPE_EVENT_ADC);

	/* Register callback for battery events. */
	npmx_core_register_cb(npmx_instance, charger_battery_callback,
			      NPMX_CALLBACK_TYPE_EVENT_BAT_CHAR_BAT);

	/* Register callback for charger status events. */
	npmx_core_register_cb(npmx_instance, charger_status_callback,
			      NPMX_CALLBACK_TYPE_EVENT_BAT_CHAR_STATUS);

	/* Check reset errors and run default debug callbacks to log error bits. */
	npmx_errlog_reset_errors_check(npmx_errlog_get(npmx_instance, 0));

	/* Use GPIO 0 as interrupt output. */
	npmx_gpio_mode_set(gpio_0, NPMX_GPIO_MODE_OUTPUT_IRQ);

	/* Disable charger before changing charge current */
	npmx_charger_module_disable_set(charger_instance, NPMX_CHARGER_MODULE_CHARGER_MASK);

	/* Set charging current. */
	npmx_charger_charging_current_set(charger_instance, CONFIG_CHARGING_CURRENT);

	/* Set battery termination voltage. */
	npmx_charger_termination_voltage_normal_set(
		charger_instance, mv_to_charger_voltage_enum(CONFIG_TERMINATION_VOLTAGE));

	/* Enable charger for events handling. */
	npmx_charger_module_enable_set(charger_instance,
				       NPMX_CHARGER_MODULE_CHARGER_MASK |
					       NPMX_CHARGER_MODULE_RECHARGE_MASK |
					       NPMX_CHARGER_MODULE_NTC_LIMITS_MASK);

	/* Enable USB connections interrupts and events handling. */
	npmx_core_event_interrupt_enable(npmx_instance, NPMX_EVENT_GROUP_VBUSIN_VOLTAGE,
					 NPMX_EVENT_GROUP_VBUSIN_DETECTED_MASK |
						 NPMX_EVENT_GROUP_VBUSIN_REMOVED_MASK);

	/* Enable all charging status interrupts and events. */
	npmx_core_event_interrupt_enable(
		npmx_instance, NPMX_EVENT_GROUP_BAT_CHAR_STATUS,
		NPMX_EVENT_GROUP_CHARGER_SUPPLEMENT_MASK | NPMX_EVENT_GROUP_CHARGER_TRICKLE_MASK |
			NPMX_EVENT_GROUP_CHARGER_CC_MASK | NPMX_EVENT_GROUP_CHARGER_CV_MASK |
			NPMX_EVENT_GROUP_CHARGER_COMPLETED_MASK |
			NPMX_EVENT_GROUP_CHARGER_ERROR_MASK);

	/* Enable battery interrupts and events. */
	npmx_core_event_interrupt_enable(npmx_instance, NPMX_EVENT_GROUP_BAT_CHAR_BAT,
					 NPMX_EVENT_GROUP_BATTERY_DETECTED_MASK |
						 NPMX_EVENT_GROUP_BATTERY_REMOVED_MASK);

	/* Enable ADC measurements ready interrupts. */
	npmx_core_event_interrupt_enable(npmx_instance, NPMX_EVENT_GROUP_ADC,
					 NPMX_EVENT_GROUP_ADC_BAT_READY_MASK);

	/* Set NTC type for ADC measurements. */
	npmx_adc_ntc_set(npmx_adc_get(npmx_instance, 0), NPMX_ADC_BATTERY_NTC_TYPE_10_K);

	/* Enable ADC auto measurements every ~1s (default)*/
	npmx_adc_config_t config = {
		.vbat_auto = true,
		.vbat_burst = false
	};

	npmx_adc_config_set(npmx_adc_get(npmx_instance, 0), &config);

	return 0;
}

uint16_t app_pmic_get_battery_voltage(void)
{
	return m_battery_voltage_mv;
}
