/**
 * @file ds18b20.c
 * @brief Control del sensor de temperatura DS18B20 (OneWire)
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h" 
#include "ds18b20.h"

static esp_err_t onewire_reset(int pin) {
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    esp_rom_delay_us(480);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    esp_rom_delay_us(70);
    int level = gpio_get_level(pin);
    esp_rom_delay_us(410);
    return (level == 0) ? ESP_OK : ESP_FAIL;
}

static void onewire_write_bit(int pin, int bit) {
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    if (bit) {
        esp_rom_delay_us(6);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        esp_rom_delay_us(64);
    } else {
        esp_rom_delay_us(60);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        esp_rom_delay_us(10);
    }
}

static int onewire_read_bit(int pin) {
    int bit = 0;
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    esp_rom_delay_us(6);
    gpio_set_direction(pin, GPIO_MODE_INPUT);
    esp_rom_delay_us(9);
    bit = gpio_get_level(pin);
    esp_rom_delay_us(55);
    return bit;
}

static void onewire_write_byte(int pin, uint8_t byte) {
    for (int i = 0; i < 8; i++) onewire_write_bit(pin, (byte >> i) & 1);
}

static uint8_t onewire_read_byte(int pin) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) if (onewire_read_bit(pin)) byte |= (1 << i);
    return byte;
}

esp_err_t ds18b20_init(int gpio_num) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pin_bit_mask = (1ULL << gpio_num)
    };
    return gpio_config(&io_conf);
}
/**
 * @brief Lee temperatura del sensor DS18B20
 * @param gpio_num Pin de datos
 * @param temp Valor de temperatura
 */
esp_err_t ds18b20_leer_temp(int gpio_num, float *temp) {
    if (onewire_reset(gpio_num) != ESP_OK) return ESP_FAIL;
    onewire_write_byte(gpio_num, 0xCC); 
    onewire_write_byte(gpio_num, 0x44); 
    vTaskDelay(pdMS_TO_TICKS(800)); 
    if (onewire_reset(gpio_num) != ESP_OK) return ESP_FAIL;
    onewire_write_byte(gpio_num, 0xCC); 
    onewire_write_byte(gpio_num, 0xBE); 
    uint8_t lsb = onewire_read_byte(gpio_num);
    uint8_t msb = onewire_read_byte(gpio_num);
    int16_t raw = (msb << 8) | lsb;
    *temp = (float)raw / 16.0;
    return ESP_OK;
}