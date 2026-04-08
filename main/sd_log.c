/**
 * @file sd_log.c
 * @brief Manejo de almacenamiento en tarjeta SD
 */
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "definiciones.h"
#include "sd_log.h"

extern QueueHandle_t xColaEscrituraSD;
extern SemaphoreHandle_t xMutexSPI;

static sdmmc_card_t *card;
/**
 * @brief Inicializa la tarjeta SD
 */
esp_err_t sd_init(void) {
    esp_err_t ret;
   
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = 400; 
    host.slot = SPI3_HOST;   
   
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_GPIO,
        .miso_io_num = SD_MISO_GPIO,
        .sclk_io_num = SD_SCK_GPIO,  
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        printf("[SD] Error inicializando bus SPI: %s\n", esp_err_to_name(ret));
        return ret;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_CS_GPIO;
    slot_config.host_id = host.slot;

    printf("[SD] Montando ADATA 64GB...\n");
   
    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
   
    if (ret != ESP_OK) {
        printf("[SD] Fallo al montar: %s\n", esp_err_to_name(ret));
    } else {
        printf("[SD] ¡Montado con exito!\n");
    }
   
    return ret;
}
/**
 * @brief Tarea de escritura en SD
 */
void tarea_escritura_sd(void *pvParameters) {
    sd_mensaje_t msg;
    while(1) {
        // REQUISITO NOTIFICACIÓN: Se bloquea aquí sin gastar CPU hasta que sensores.c avise
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 

        // Una vez notificado, revisamos si hay algo en la cola
        while (xQueueReceive(xColaEscrituraSD, &msg, 0) == pdTRUE) {
            // REQUISITO MUTEX: Protegemos la escritura
            if (xMutexSPI != NULL && xSemaphoreTake(xMutexSPI, pdMS_TO_TICKS(1000)) == pdTRUE) {
                FILE *f = fopen("/sdcard/registro_cafe.txt", "a");
                if (f != NULL) {
                    fprintf(f, "%s\n", msg.log_linea);
                    fclose(f);
                } else {
                    printf("[SD] Error: No se pudo abrir archivo para escritura\n");
                }
                xSemaphoreGive(xMutexSPI);
            }
        }
    }
}