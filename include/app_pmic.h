#ifndef __APP_PMIC_H
#define __APP_PMIC_H

#include <zephyr.h>

typedef struct {
	int type;
} app_pmic_evt_t;

extern const char *pmic_state_name_strings[];

typedef void (*app_pmic_callback_t)(app_pmic_evt_t *evt);

int app_pmic_init(app_pmic_callback_t callback);

#endif