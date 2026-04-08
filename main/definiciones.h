/**
 * @file main.c
 * @brief Archivo principal del sistema de fermentación de café
 * 
 * Este archivo inicializa el sistema embebido basado en ESP32,
 * crea las tareas de FreeRTOS y coordina el funcionamiento del
 * bioreactor.
 * 
 * @author Tu Nombre
 * @date 2026
 */
#ifndef DEFINICIONES_H
#define DEFINICIONES_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"  // Necesario para TaskHandle_t (Notificaciones)
#include "driver/i2c.h"

// --- ASIGNACIÓN DE PINES ---
#define LCD_SDA_GPIO    21
#define LCD_SCL_GPIO    22
#define I2C_MASTER_NUM  I2C_NUM_0
#define I2C_FREQ_HZ     50000
#define LCD_ADDR        0x27

#define RTC_RST_GPIO    32
#define RTC_DAT_GPIO    33
#define RTC_CLK_GPIO    25

#define SD_MOSI_GPIO    23
#define SD_MISO_GPIO    19
#define SD_SCK_GPIO     18
#define SD_CS_GPIO      5

#define PH_ADC_GPIO     36
#define DHT_DATA_GPIO   4
#define DS18B20_GPIO    16
#define LDR_GPIO        17

#define RGB_RED_GPIO    14  
#define RGB_GREEN_GPIO  26
#define RGB_BLUE_GPIO   27
#define BOTON_INICIO    13  
#define BOTON_MEZCLA    12  

// --- CONFIGURACIÓN DE LÍMITES ---
#define TEMP_MAX_CAFE       32.0    
#define TEMP_MIN_CAFE       -1
#define PH_MAX              16
#define PH_MIN              -1    
#define LUZ_MIN_REQUERIDA   0      

// --- TIEMPOS ---
#define TIEMPO_ALERTA_MAX       900
#define TIEMPO_LECTURA_PH       900
#define TIEMPO_MAX_PERSISTENCIA 900

// --- ESTRUCTURAS DE DATOS ---
/**
 * @struct datos_fermentacion_t
 * @brief Estructura que almacena los datos de sensores
 */
typedef struct {
    float temp_aire;     /**< Temperatura ambiente */
    float hum_aire;      /**< Humedad ambiente */
    float temp_cafe;     /**< Temperatura del café */
    float ph_valor;      /**< Nivel de pH */
    int luz_nivel;       /**< Nivel de luz */
    char timestamp[32];  /**< Fecha y hora */
    int causa_alerta;    /**< Código de alerta */
} datos_fermentacion_t;
typedef struct {
    char log_linea[128];
} sd_mensaje_t;

/**
 * @enum estado_sistema_t
 * @brief Estados del sistema de fermentación
 */
typedef enum {
    ESTADO_REPOSO,      /**< Sistema detenido */
    ESTADO_MONITOREO,   /**< Lectura de sensores */
    ESTADO_ALERTA,      /**< Condición fuera de rango */
    ESTADO_ALARMA       /**< Estado crítico */
} estado_sistema_t;

// --- RECURSOS COMPARTIDOS (EXPLICACIÓN PARA REQUISITOS) ---

// 1. COLAS (Queues): Comunicación entre sensores, pantalla y SD.
extern QueueHandle_t xColaDatos;      
extern QueueHandle_t xColaEscrituraSD;

// 2. MUTEX: Protección de buses compartidos (I2C y SPI).
extern SemaphoreHandle_t xMutexI2C;    
extern SemaphoreHandle_t xMutexSPI;    // Protege la SD ADATA

// 3. SEMÁFOROS (Semaphores): Sincronización del botón de mezcla.
extern SemaphoreHandle_t xSemaforoMezcla; 

// 4. NOTIFICACIONES (Task Notifications): Despertar tareas específicas.
extern TaskHandle_t xTaskPantallaHandle;
extern TaskHandle_t xTaskSDHandle;

// --- VARIABLES GLOBALES ---
extern estado_sistema_t estado_actual_global;

#endif