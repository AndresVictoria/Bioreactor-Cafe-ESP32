#include "ldr.h"
#include "driver/gpio.h"

void ldr_init(int gpio_num) {
    gpio_reset_pin(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
}

int ldr_leer_estado(int gpio_num) {
    return !gpio_get_level(gpio_num); 
}