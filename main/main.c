/**
 * @file main.c
 * @brief Archivo principal del sistema de fermentación de café
 * 
 * Este archivo inicializa el sistema embebido basado en ESP32,
 * crea las tareas de FreeRTOS y coordina el funcionamiento del
 * bioreactor.
 * 
 * @author Andres victoria, michael canchala, miguel daza
 * @date 2026
 */
/**
 * @mainpage Sistema de Fermentación de Café
 *
 * Sistema basado en ESP32 para monitoreo de variables:
 * - Temperatura
 * - pH
 * - Humedad
 * - Luz
 *
 * Incluye almacenamiento en SD y visualización en LCD.
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_wifi.h"       
#include "esp_pm.h"         
#include "definiciones.h"
#include "sensores.h"
#include "i2c_lcd.h"
#include "rgb_control.h"
#include "botones.h"
#include "sd_log.h"

// --- VARIABLES GLOBALES ---
extern int seg_desde_ultima_mezcla;
estado_sistema_t estado_actual_global = ESTADO_REPOSO;

QueueHandle_t xColaDatos = NULL;
QueueHandle_t xColaEscrituraSD = NULL;
SemaphoreHandle_t xMutexI2C = NULL;
SemaphoreHandle_t xMutexSPI = NULL;
SemaphoreHandle_t xSemaforoMezcla = NULL; 

TaskHandle_t xTaskPantallaHandle = NULL;
TaskHandle_t xTaskSDHandle = NULL;

bool toggle_alerta = false;
volatile bool mezcla_manual_hecha = false;

// --- TAREA: PANTALLA LCD ---
void tarea_pantalla(void *pvParameters) {
    datos_fermentacion_t d_cache = {0};
    char l1[17], l2[17];
    lcd_init();
    lcd_clear();
    
    lcd_put_cur(0, 0); lcd_send_string("BIOREACTOR V2.0");
    lcd_put_cur(1, 0); lcd_send_string("LISTO - POPAYAN");

    while (1) {
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000)) == pdPASS) {
            xQueuePeek(xColaDatos, &d_cache, 0);
        }

        if (estado_actual_global != ESTADO_REPOSO) {
            snprintf(l1, 17, "T:%4.1fC H:%2.0f%% ", d_cache.temp_cafe, d_cache.hum_aire);
            lcd_put_cur(0, 0); lcd_send_string(l1);

            if (d_cache.causa_alerta == 0) {
                snprintf(l2, 17, "pH:%4.2f L:%s  ", d_cache.ph_valor, d_cache.luz_nivel ? "ON " : "OFF");
                lcd_put_cur(1, 0); lcd_send_string(l2);
            } else {
                toggle_alerta = !toggle_alerta;
                lcd_put_cur(1, 0);
                if (toggle_alerta) {
                    switch(d_cache.causa_alerta) {
                        case 1:  lcd_send_string("!! TEMP ALTA !!"); break;
                        case 2:  lcd_send_string("!! TEMP BAJA !!"); break;
                        case 3:  lcd_send_string("!! PH ALTO     !!"); break;
                        case 4:  lcd_send_string("!! PH BAJO     !!"); break;
                        case 5:  lcd_send_string("!! MUCHA LUZ !!"); break;
                        case 6:  lcd_send_string("MEZCLAR+PULSAR "); break;
                        default: lcd_send_string("ALERTA MULTIPLE"); break;
                    }
                } else {
                    snprintf(l2, 17, "pH:%4.2f L:%s  ", d_cache.ph_valor, d_cache.luz_nivel ? "ON " : "OFF");
                    lcd_send_string(l2);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// --- TAREA: CONTROL (CORREGIDA) ---
void tarea_control_proceso(void *pvParameters) {
    datos_fermentacion_t datos_vivos;
    int ticks_en_alerta = 0;
    int toggle_visual = 0;
    rgb_init();
    botones_init();

    while (1) {
        boton_mezcla_presionado(); 

        if (xSemaforoMezcla != NULL && xSemaphoreTake(xSemaforoMezcla, 0) == pdTRUE) {
            seg_desde_ultima_mezcla = 0;
            mezcla_manual_hecha = true;
            
            // CORRECCIÓN: Forzamos la limpieza de la cola para que la pantalla se actualice YA
            if (xQueuePeek(xColaDatos, &datos_vivos, 0) == pdTRUE) {
                if (datos_vivos.causa_alerta == 6) {
                    datos_vivos.causa_alerta = 0;
                    xQueueOverwrite(xColaDatos, &datos_vivos);
                }
            }
            printf("[EVENTO] Boton presionado: Mezcla registrada. Tiempo Reset.\n");
        }

        if (estado_actual_global == ESTADO_REPOSO) {
            color_verde();
            if (boton_inicio_presionado()) {
                estado_actual_global = ESTADO_MONITOREO;
                printf("[SISTEMA] Inicio de Monitoreo...\n");
                lcd_clear();
            }
        } else {
            if (xQueueReceive(xColaDatos, &datos_vivos, 0) == pdTRUE) {
                // Si acabamos de mezclar manualmente, anulamos la alerta visual de mezcla
                if (mezcla_manual_hecha && datos_vivos.causa_alerta == 6) {
                    datos_vivos.causa_alerta = 0;
                }

                printf("DATA >> T:%2.1f pH:%2.2f Alerta:%d | Seg_Mezcla: %d\n", 
                        datos_vivos.temp_cafe, datos_vivos.ph_valor, 
                        datos_vivos.causa_alerta, seg_desde_ultima_mezcla);

                if (datos_vivos.causa_alerta == 99) {
                    estado_actual_global = ESTADO_ALARMA;
                } else if (datos_vivos.causa_alerta > 0) {
                    ticks_en_alerta++;
                    if (ticks_en_alerta >= (TIEMPO_MAX_PERSISTENCIA * 5)) estado_actual_global = ESTADO_ALARMA;
                    else estado_actual_global = ESTADO_ALERTA;
                } else {
                    estado_actual_global = ESTADO_MONITOREO;
                    ticks_en_alerta = 0;
                    // Bajamos la bandera de mezcla tras un ciclo limpio
                    if (seg_desde_ultima_mezcla < 10) mezcla_manual_hecha = false;
                }
            }

            toggle_visual = !toggle_visual;
            switch(estado_actual_global) {
                case ESTADO_MONITOREO: color_azul(); break;
                case ESTADO_ALERTA:    if (toggle_visual) color_amarillo(); else color_azul(); break;
                case ESTADO_ALARMA:    if (toggle_visual) color_rojo(); else color_azul(); break;
                default:               color_verde(); break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200)); 
    }
}

void app_main(void) {
    esp_wifi_stop();
    esp_pm_config_esp32_t pm_config = {
        .max_freq_mhz = 80, 
        .min_freq_mhz = 10,
        .light_sleep_enable = true
    };
    esp_pm_configure(&pm_config);

    vTaskDelay(pdMS_TO_TICKS(500));
    
    xColaDatos = xQueueCreate(1, sizeof(datos_fermentacion_t));
    xColaEscrituraSD = xQueueCreate(10, sizeof(sd_mensaje_t));
    xMutexI2C = xSemaphoreCreateMutex();
    xMutexSPI = xSemaphoreCreateMutex();
    xSemaforoMezcla = xSemaphoreCreateBinary();

    if (xColaDatos != NULL) {
        if (sd_init() == ESP_OK) {
            xTaskCreate(tarea_escritura_sd, "TareaSD", 4096, NULL, 4, &xTaskSDHandle);
        }

        xTaskCreate(tarea_sensores, "TareaSensores", 4096, NULL, 5, NULL);
        xTaskCreate(tarea_pantalla, "TareaPantalla", 4096, NULL, 5, &xTaskPantallaHandle);
        xTaskCreate(tarea_control_proceso, "TareaControl", 4096, NULL, 5, NULL);
        
        printf("[SISTEMA] Bioreactor FreeRTOS V2.0 Online.\n");
    }
}