/**
 * @file ph_sensor.c
 * @brief Lectura del sensor de pH mediante ADC
 */
#include <stdio.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "ph_sensor.h"

// Variables para el ADC
static adc_oneshot_unit_handle_t adc1_handle;
static bool inicializado = false;

void ph_init(int gpio_num) {
    // Configuración del ADC para el pH
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    // Usamos el canal 0 (ajustar según tu GPIO si es necesario)
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config);
    inicializado = true;
}

float ph_leer(int gpio_num) {
    if (!inicializado) return 0.0;

    int adc_raw = 0;
    // Leemos el valor crudo del ADC
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw);
    
    if (ret == ESP_OK) {
        // Conversión simple de ejemplo (ajustar según calibración)
        float voltaje = (adc_raw / 4095.0f) * 3.3f;
        float ph = 3.5f * voltaje; // Esta es una fórmula genérica
        return ph;
    } else {
        return -1.0; // Error de lectura
    }
}