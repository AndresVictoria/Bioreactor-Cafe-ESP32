#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include "esp_err.h"

void ph_init(int gpio_num);
float ph_leer(int gpio_num);

#endif