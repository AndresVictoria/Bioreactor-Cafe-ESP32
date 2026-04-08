#ifndef DHT_H
#define DHT_H

#include "esp_err.h"

// Definimos el tipo de sensor que tienes
#define DHT_TYPE_DHT22 1

esp_err_t dht_read_data(int gpio_num, float *hum, float *temp);

#endif