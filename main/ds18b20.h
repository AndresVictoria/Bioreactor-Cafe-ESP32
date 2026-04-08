#ifndef DS18B20_H
#define DS18B20_H

#include "esp_err.h"

esp_err_t ds18b20_init(int gpio_num);
esp_err_t ds18b20_leer_temp(int gpio_num, float *temp);

#endif