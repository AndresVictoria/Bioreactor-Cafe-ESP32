#ifndef DS1302_H
#define DS1302_H

#include "esp_err.h"

typedef struct {
    uint8_t segundo;
    uint8_t minuto;
    uint8_t hora;
    uint8_t dia;
    uint8_t mes;
    uint16_t anio;
} rtc_tiempo_t;

void ds1302_init(int clk, int dat, int rst);
void ds1302_leer_tiempo(rtc_tiempo_t *tiempo);
void ds1302_fijar_tiempo(rtc_tiempo_t *tiempo);

#endif