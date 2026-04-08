/**
 * @file i2c_lcd.c
 * @brief Control de pantalla LCD por I2C
 */
#include "i2c_lcd.h"
#include "definiciones.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // Necesario para el Mutex
#include <string.h>

// Manejadores del bus (Drivers v5.x)
static i2c_master_bus_handle_t g_bus_handle = NULL;
static i2c_master_dev_handle_t g_dev_handle = NULL;

// Traemos el Mutex del main
extern SemaphoreHandle_t xMutexI2C;

// Bits de control del expansor PCF8574
#define LCD_PIN_RS        0x01
#define LCD_PIN_EN        0x04
#define LCD_PIN_BACKLIGHT 0x08

static esp_err_t lcd_send_nibble(uint8_t nibble, bool rs) {
    uint8_t data_base = LCD_PIN_BACKLIGHT | ((nibble & 0x0F) << 4);
    if (rs) data_base |= LCD_PIN_RS;

    uint8_t seq[3];
    seq[0] = data_base;                
    seq[1] = data_base | LCD_PIN_EN;  
    seq[2] = data_base;                

    esp_err_t err = i2c_master_transmit(g_dev_handle, seq, 3, 100);
    vTaskDelay(pdMS_TO_TICKS(2)); 
    return err;
}

static void lcd_send_byte(uint8_t value, bool rs) {
    lcd_send_nibble((value >> 4) & 0x0F, rs);
    lcd_send_nibble(value & 0x0F, rs);
}

void lcd_send_cmd(uint8_t cmd) {
    // PROTECCIÓN CON MUTEX: Requisito de recurso compartido
    if (xMutexI2C != NULL && xSemaphoreTake(xMutexI2C, pdMS_TO_TICKS(500)) == pdTRUE) {
        lcd_send_byte(cmd, false);
        vTaskDelay(pdMS_TO_TICKS(2));
        xSemaphoreGive(xMutexI2C);
    }
}

void lcd_send_data(uint8_t data) {
    // PROTECCIÓN CON MUTEX
    if (xMutexI2C != NULL && xSemaphoreTake(xMutexI2C, pdMS_TO_TICKS(500)) == pdTRUE) {
        lcd_send_byte(data, true);
        vTaskDelay(pdMS_TO_TICKS(1));
        xSemaphoreGive(xMutexI2C);
    }
}

void lcd_init(void) {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = LCD_SCL_GPIO,
        .sda_io_num = LCD_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &g_bus_handle));

    i2c_device_config_t dev_cfg = {
        .device_address = LCD_ADDR,  
        .scl_speed_hz = 10000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(g_bus_handle, &dev_cfg, &g_dev_handle));

    vTaskDelay(pdMS_TO_TICKS(50));

    // Inicialización manual (sin mutex aún porque no hay tareas corriendo)
    for(int i=0; i<6; i++) {
        lcd_send_nibble(0x03, false);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    lcd_send_nibble(0x02, false);
    vTaskDelay(pdMS_TO_TICKS(10));

    lcd_send_cmd(0x28); 
    lcd_send_cmd(0x0C); 
    lcd_send_cmd(0x06); 
    lcd_send_cmd(0x01); 
    vTaskDelay(pdMS_TO_TICKS(5));
}

void lcd_put_cur(int row, int col) {
    uint8_t pos = (row == 0) ? (0x80 + col) : (0xC0 + col);
    lcd_send_cmd(pos);
}

void lcd_send_string(char *str) {
    // El mutex se maneja dentro de lcd_send_data para cada caracter
    while (*str) lcd_send_data((uint8_t)*str++);
}

void lcd_clear(void) {
    lcd_send_cmd(0x01);
}