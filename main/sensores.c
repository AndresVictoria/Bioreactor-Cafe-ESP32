/**
 * @file sensores.c
 * @brief Tarea principal de adquisición de datos
 * @details Lee sensores, gestiona estados y envía datos a cola
 */
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "definiciones.h"
#include "ds18b20.h"
#include "dht.h"
#include "ldr.h"
#include "ds1302.h"
#include "botones.h"
#include "ph_sensor.h"
#include "sd_log.h"

// --- PUENTES ---
extern estado_sistema_t estado_actual_global;
extern QueueHandle_t xColaEscrituraSD;
extern QueueHandle_t xColaDatos;

// REQUISITO NOTIFICACIONES: Handlers de las tareas destino
extern TaskHandle_t xTaskPantallaHandle;
extern TaskHandle_t xTaskSDHandle;

int seg_desde_ultima_mezcla = 0;
/**
 * @brief Tarea FreeRTOS de sensores
 */
void tarea_sensores(void *pvParameters) {
    datos_fermentacion_t datos;
    sd_mensaje_t msg_sd;
    rtc_tiempo_t ahora = {0};
    static rtc_tiempo_t ultima_buena = {0};
    
    int contador_ph = TIEMPO_LECTURA_PH;
    float ultimo_ph = 0.0;
    static int segundo_anterior_total = -1; // Para cálculo exacto del RTC

    ds18b20_init(DS18B20_GPIO);
    ldr_init(LDR_GPIO);
    ds1302_init(RTC_CLK_GPIO, RTC_DAT_GPIO, RTC_RST_GPIO);
    ph_init(PH_ADC_GPIO);

    while(1) {
        if (estado_actual_global == ESTADO_REPOSO) {
            seg_desde_ultima_mezcla = 0;
            segundo_anterior_total = -1; // Reset del cálculo de tiempo
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // --- 1. GESTIÓN DEL TIEMPO ---
        rtc_tiempo_t lectura_rtc;
        ds1302_leer_tiempo(&lectura_rtc);
        
        if (lectura_rtc.anio >= 2024 && lectura_rtc.anio <= 2030) {
            ultima_buena = lectura_rtc;
            ahora = lectura_rtc;
        } else {
            ultima_buena.segundo += 5;
            if (ultima_buena.segundo >= 60) {
                ultima_buena.segundo -= 60;
                ultima_buena.minuto++;
            }
            ahora = ultima_buena;
        }

        // CORRECCIÓN: Cálculo de segundos reales usando el RTC
        int segundo_actual_total = (int)ahora.segundo + ((int)ahora.minuto * 60) + ((int)ahora.hora * 3600);
        
        if (segundo_anterior_total != -1) {
            int transcurrido = segundo_actual_total - segundo_anterior_total;
            if (transcurrido < 0) transcurrido += 86400; // Caso cambio de día
            seg_desde_ultima_mezcla += transcurrido;
        }
        segundo_anterior_total = segundo_actual_total;

        snprintf(datos.timestamp, sizeof(datos.timestamp), "%02d/%02d/%04d %02d:%02d:%02d",
                 (int)ahora.dia, (int)ahora.mes, (int)ahora.anio,
                 (int)ahora.hora, (int)ahora.minuto, (int)ahora.segundo);
        
        // --- 2. LECTURA DE SENSORES ---
        datos.hum_aire = 0.0;
        datos.temp_aire = 0.0;
        ds18b20_leer_temp(DS18B20_GPIO, &datos.temp_cafe);
        dht_read_data(DHT_DATA_GPIO, &datos.hum_aire, &datos.temp_aire);
        datos.luz_nivel = ldr_leer_estado(LDR_GPIO);
        
        if (contador_ph >= TIEMPO_LECTURA_PH) {
            ultimo_ph = ph_leer(PH_ADC_GPIO);
            contador_ph = 0;
        }
        datos.ph_valor = ultimo_ph;

        // --- 3. EVALUACIÓN DE ALERTAS ---
        bool err_t_alta  = (datos.temp_cafe > TEMP_MAX_CAFE);
        bool err_t_baja  = (datos.temp_cafe < TEMP_MIN_CAFE && datos.temp_cafe != -99.0);
        bool err_ph_alto = (datos.ph_valor > PH_MAX);
        bool err_ph_bajo = (datos.ph_valor < PH_MIN);
        bool err_luz     = false;
        bool err_mezcla  = (seg_desde_ultima_mezcla >= TIEMPO_ALERTA_MAX);

        int num_fallos = (err_t_alta || err_t_baja) + (err_ph_alto || err_ph_bajo) + err_luz + err_mezcla;

        if (num_fallos >= 3) datos.causa_alerta = 99;
        else if (num_fallos == 2) {
            if (err_t_baja && err_ph_bajo)      datos.causa_alerta = 10;
            else if (err_t_baja && err_ph_alto) datos.causa_alerta = 11;
            else if (err_t_alta && err_ph_bajo) datos.causa_alerta = 12;
            else if (err_t_alta && err_ph_alto) datos.causa_alerta = 13;
            else if (err_t_baja && err_luz)     datos.causa_alerta = 20;
            else if (err_t_baja && err_mezcla)  datos.causa_alerta = 21;
            else if (err_t_alta && err_luz)     datos.causa_alerta = 22;
            else if (err_t_alta && err_mezcla)  datos.causa_alerta = 23;
            else if (err_ph_bajo && err_luz)    datos.causa_alerta = 24;
            else if (err_ph_bajo && err_mezcla) datos.causa_alerta = 25;
            else if (err_ph_alto && err_luz)    datos.causa_alerta = 26;
            else if (err_ph_alto && err_mezcla) datos.causa_alerta = 27;
            else if (err_luz && err_mezcla)     datos.causa_alerta = 28;
            else datos.causa_alerta = 29;
        }
        else if (num_fallos == 1) {
            if (err_t_alta)       datos.causa_alerta = 1;
            else if (err_t_baja)  datos.causa_alerta = 2;
            else if (err_ph_alto) datos.causa_alerta = 3;
            else if (err_ph_bajo) datos.causa_alerta = 4;
            else if (err_luz)     datos.causa_alerta = 5;
            else if (err_mezcla)  datos.causa_alerta = 6;
        }
        else datos.causa_alerta = 0;

        // --- 4. ENVÍO DE DATOS Y NOTIFICACIONES ---
        snprintf(msg_sd.log_linea, sizeof(msg_sd.log_linea),
                 "[%s] Ti:%.1f | Te:%.1f | H:%.1f%% | pH:%.2f L:%d",
                 datos.timestamp, datos.temp_cafe, datos.temp_aire,
                 datos.hum_aire, datos.ph_valor, datos.luz_nivel);

        xQueueSend(xColaEscrituraSD, &msg_sd, 0);
        xQueueOverwrite(xColaDatos, &datos);

        if (xTaskPantallaHandle != NULL) xTaskNotifyGive(xTaskPantallaHandle);
        if (xTaskSDHandle != NULL) xTaskNotifyGive(xTaskSDHandle);

        vTaskDelay(pdMS_TO_TICKS(5000));
        contador_ph += 5;
    }
}