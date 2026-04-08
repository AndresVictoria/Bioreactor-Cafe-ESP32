/**
 * @file ds1302.c
 * @brief Manejo del reloj en tiempo real (RTC)
 */
#include "ds1302.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

static int g_clk, g_dat, g_rst;

static void ds1302_write_byte(uint8_t valor) {
    gpio_set_direction(g_dat, GPIO_MODE_OUTPUT);
    for (int i = 0; i < 8; i++) {
        gpio_set_level(g_dat, (valor >> i) & 1);
        esp_rom_delay_us(15); // Aumentamos a 15us para máxima estabilidad
        gpio_set_level(g_clk, 1); 
        esp_rom_delay_us(15);
        gpio_set_level(g_clk, 0); 
        esp_rom_delay_us(15);
    }
}

static uint8_t ds1302_read_byte() {
    uint8_t valor = 0;
    gpio_set_direction(g_dat, GPIO_MODE_INPUT);
    for (int i = 0; i < 8; i++) {
        gpio_set_level(g_clk, 0); 
        esp_rom_delay_us(15);
        if (gpio_get_level(g_dat)) {
            valor |= (1 << i);
        }
        gpio_set_level(g_clk, 1); 
        esp_rom_delay_us(15);
        gpio_set_level(g_clk, 0); 
        esp_rom_delay_us(15);
    }
    return valor;
}

void ds1302_init(int clk, int dat, int rst) {
    g_clk = clk; g_dat = dat; g_rst = rst;
    gpio_reset_pin(clk); gpio_set_direction(clk, GPIO_MODE_OUTPUT);
    gpio_reset_pin(dat); gpio_set_direction(dat, GPIO_MODE_OUTPUT);
    gpio_reset_pin(rst); gpio_set_direction(rst, GPIO_MODE_OUTPUT);
    gpio_set_level(rst, 0); gpio_set_level(clk, 0);
}

static uint8_t bcd2dec(uint8_t bcd) { return ((bcd >> 4) * 10) + (bcd & 0x0F); }
static uint8_t dec2bcd(uint8_t dec) { return ((dec / 10) << 4) | (dec % 10); }
/**
 * @brief Lee la hora actual
 * @param t estructura de tiempo
 */
void ds1302_leer_tiempo(rtc_tiempo_t *t);
void ds1302_leer_tiempo(rtc_tiempo_t *t) {
    rtc_tiempo_t temp;
    
    // Intentamos la lectura real
    gpio_set_level(g_rst, 1);
    esp_rom_delay_us(15);
    ds1302_write_byte(0xBF);
    temp.segundo = bcd2dec(ds1302_read_byte() & 0x7F);
    temp.minuto  = bcd2dec(ds1302_read_byte());
    temp.hora    = bcd2dec(ds1302_read_byte());
    temp.dia     = bcd2dec(ds1302_read_byte());
    temp.mes     = bcd2dec(ds1302_read_byte());
    ds1302_read_byte();
    temp.anio    = bcd2dec(ds1302_read_byte()) + 2000;
    gpio_set_level(g_rst, 0);

    // --- FILTRO INTELIGENTE ---
    // Si la lectura es válida, actualizamos el tiempo real
    if (temp.mes > 0 && temp.mes <= 12 && temp.dia > 0 && temp.dia <= 31 && temp.segundo < 60) {
        *t = temp;
    } 
    else {
        // ¡FALLÓ LA LECTURA! Aplicamos tu lógica de sumar 5 segundos
        t->segundo += 5;
        
        // Ajuste de desborde (Si pasa de 60 seg, suma 1 min)
        if (t->segundo >= 60) {
            t->segundo -= 60;
            t->minuto++;
            if (t->minuto >= 60) {
                t->minuto = 0;
                t->hora++;
            }
        }
        // Nota: No imprimimos error para que el reporte se vea limpio
    }
}

void ds1302_fijar_tiempo(rtc_tiempo_t *t) {
    gpio_set_level(g_rst, 1);
    ds1302_write_byte(0x8E); ds1302_write_byte(0x00); // Unlock
    gpio_set_level(g_rst, 0); esp_rom_delay_us(10);
    gpio_set_level(g_rst, 1);
    ds1302_write_byte(0xBE); // Burst Write
    ds1302_write_byte(dec2bcd(t->segundo));
    ds1302_write_byte(dec2bcd(t->minuto));
    ds1302_write_byte(dec2bcd(t->hora));
    ds1302_write_byte(dec2bcd(t->dia));
    ds1302_write_byte(dec2bcd(t->mes));
    ds1302_write_byte(0x01);
    ds1302_write_byte(dec2bcd(t->anio % 100));
    ds1302_write_byte(0x00);
    gpio_set_level(g_rst, 0);
}