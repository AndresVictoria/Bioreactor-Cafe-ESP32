    /**
 * @brief Detecta si el botón de inicio fue presionado
 * @return true si fue presionado
 */
    #include "dht.h"
    #include "driver/gpio.h"
    #include "esp_rom_sys.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
/**
 * @brief Lee datos del sensor DHT
 * @param gpio_num Pin de conexión
 * @param hum Puntero a humedad
 * @param temp Puntero a temperatura
 * @return ESP_OK si la lectura es correcta
 */
    esp_err_t dht_read_data(int gpio_num, float *hum, float *temp) {
        uint8_t data[5] = {0,0,0,0,0};
        uint32_t usec = 0;

        // 1. Pulso de inicio (Start Signal)
        gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
        gpio_set_level(gpio_num, 0);
        esp_rom_delay_us(18000); // 18ms para DHT22
        gpio_set_level(gpio_num, 1);
        esp_rom_delay_us(40);
        gpio_set_direction(gpio_num, GPIO_MODE_INPUT);

        // 2. Esperar respuesta del sensor
        usec = 0;
        while(gpio_get_level(gpio_num) == 1) { if(usec++ > 100) return ESP_FAIL; esp_rom_delay_us(1); }
        usec = 0;
        while(gpio_get_level(gpio_num) == 0) { if(usec++ > 100) return ESP_FAIL; esp_rom_delay_us(1); }
        usec = 0;
        while(gpio_get_level(gpio_num) == 1) { if(usec++ > 100) return ESP_FAIL; esp_rom_delay_us(1); }

        // 3. Leer los 40 bits (5 bytes)
        for(int i = 0; i < 40; i++) {
            usec = 0;
            while(gpio_get_level(gpio_num) == 0) { if(usec++ > 100) return ESP_FAIL; esp_rom_delay_us(1); }
            
            esp_rom_delay_us(30); // Si después de 30us sigue en alto, es un '1'
            
            if(gpio_get_level(gpio_num) == 1) {
                data[i/8] |= (1 << (7 - (i%8)));
                usec = 0;
                while(gpio_get_level(gpio_num) == 1) { if(usec++ > 100) return ESP_FAIL; esp_rom_delay_us(1); }
            }
        }

        // 4. Verificar Checksum y convertir datos
        if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
            *hum = ((data[0] << 8) | data[1]) * 0.1;
            *temp = (((data[2] & 0x7F) << 8) | data[3]) * 0.1;
            if (data[2] & 0x80) *temp *= -1;
            return ESP_OK;
        }

        return ESP_FAIL;
    }