#ifndef I2C_LCD_H
#define I2C_LCD_H

#include <stdint.h>

// Funciones principales
void lcd_init(void);                          // Inicia la pantalla
void lcd_send_cmd(uint8_t cmd);               // Envía una orden (ej: limpiar)
void lcd_send_data(uint8_t data);             // Envía una letra
void lcd_send_string(char *str);              // Envía una frase completa
void lcd_put_cur(int row, int col);           // Mueve el cursor (Fila 0-1, Col 0-15)
void lcd_clear(void);                         // Borra todo

#endif