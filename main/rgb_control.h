#ifndef RGB_CONTROL_H
#define RGB_CONTROL_H

#include "esp_err.h"

// Funciones básicas
void rgb_init(void);
void rgb_set_color(int r, int g, int b);

// Atajos para tu lógica de Popayán
void color_verde(void);    // Reposo
void color_azul(void) ;    // Monitoreo
void color_amarillo(void); // Alerta / Mezcla
void color_rojo(void);     // Alarma
void color_off(void);

#endif