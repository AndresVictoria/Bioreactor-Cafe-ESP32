#ifndef SD_LOG_H
#define SD_LOG_H

#include "esp_err.h"

// Inicializa el bus SPI y monta la tarjeta ADATA de 64GB
esp_err_t sd_init(void);

// Tarea que se queda esperando datos para escribir en la SD
void tarea_escritura_sd(void *pvParameters);

#endif