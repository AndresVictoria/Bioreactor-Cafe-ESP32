#ifndef LDR_H
#define LDR_H

#include "esp_err.h"

void ldr_init(int gpio_num);
int ldr_leer_estado(int gpio_num); // <--- Verifica que diga ESTADO y no PORCENTAJE

#endif