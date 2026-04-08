/**
 * @file rgb_control.c
 * @brief Control del LED RGB para estados del sistema
 */
#include "driver/gpio.h"
#include "definiciones.h"

void rgb_init() {
    gpio_reset_pin(RGB_RED_GPIO);
    gpio_reset_pin(RGB_GREEN_GPIO);
    gpio_reset_pin(RGB_BLUE_GPIO);
    
    gpio_set_direction(RGB_RED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_GREEN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(RGB_BLUE_GPIO, GPIO_MODE_OUTPUT);

    // FUERZA EL APAGADO TOTAL (Nivel 0)
    gpio_set_level(RGB_RED_GPIO, 0);
    gpio_set_level(RGB_GREEN_GPIO, 0);
    gpio_set_level(RGB_BLUE_GPIO, 0);
}

void rgb_set_color(int r, int g, int b) {
    gpio_set_level(RGB_RED_GPIO, r);
    gpio_set_level(RGB_GREEN_GPIO, g);
    gpio_set_level(RGB_BLUE_GPIO, b);
}

// Colores rápidos
void color_verde()   { rgb_set_color(0, 1, 0); }
void color_azul()    { rgb_set_color(0, 0, 1); }
void color_amarillo(){ rgb_set_color(1, 1, 0); } // Rojo + Verde = Amarillo
void color_rojo()    { rgb_set_color(1, 0, 0); }
void color_off()     { rgb_set_color(0, 0, 0); }