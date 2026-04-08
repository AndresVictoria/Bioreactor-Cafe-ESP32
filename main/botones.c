/**
 * @file botones.c
 * @brief Manejo de botones físicos del sistema
 */
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // Necesario para xSemaphoreGive
#include "definiciones.h"
#include "botones.h"

// Traemos el semáforo que creamos en el main.c
extern SemaphoreHandle_t xSemaforoMezcla;
/**
 * @brief Inicializa los botones
 */
void botones_init(void); {
        // 1. Configuración Botón de INICIO (Pin 13)
    gpio_reset_pin(BOTON_INICIO);
    gpio_set_direction(BOTON_INICIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOTON_INICIO, GPIO_PULLUP_ONLY);

    // 2. Configuración Botón de MEZCLA (Pin 12)
    gpio_reset_pin(BOTON_MEZCLA);
    gpio_set_direction(BOTON_MEZCLA, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BOTON_MEZCLA, GPIO_PULLUP_ONLY);
   
    printf("[BOTONES] Inicio (GPIO %d) y Mezcla (GPIO %d) inicializados.\n", BOTON_INICIO, BOTON_MEZCLA);
}
/**
 * @brief Detecta si el botón de inicio fue presionado
 * @return true si fue presionado
 */
bool boton_inicio_presionado(void) {
    if (gpio_get_level(BOTON_INICIO) == 0) {
        vTaskDelay(pdMS_TO_TICKS(50)); 
        if (gpio_get_level(BOTON_INICIO) == 0) {
            // Esperamos a que suelte el botón para que no mande mil señales
            while(gpio_get_level(BOTON_INICIO) == 0) { vTaskDelay(pdMS_TO_TICKS(10)); }
            return true;
        }
    }
    return false;
}

bool boton_mezcla_presionado(void) {
    if (gpio_get_level(BOTON_MEZCLA) == 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
        if (gpio_get_level(BOTON_MEZCLA) == 0) {
            
            // --- TUNEADO PARA REQUISITO: SEMÁFORO ---
            // Si el semáforo ya existe, lo entregamos ("Give")
            if (xSemaforoMezcla != NULL) {
                xSemaphoreGive(xSemaforoMezcla);
            }
            
            // Esperamos a que el operario suelte el botón
            while(gpio_get_level(BOTON_MEZCLA) == 0) { vTaskDelay(pdMS_TO_TICKS(10)); }
            
            return true;
        }
    }
    return false;
}