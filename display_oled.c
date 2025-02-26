#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"

#define UART_ID uart1
#define TX_PIN 8
#define RX_PIN 9
#define BAUD_RATE 9600

#define GREEN_PIN 12
#define BLUE_PIN 11

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

int main() {
    stdio_init_all();

    // Inicialização do I2C para o OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(); // Inicializa o display OLED

    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // Zerar o display
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);

    // Configuração UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);

    // Configuração dos LEDs
    gpio_init(GREEN_PIN);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_init(BLUE_PIN);
    gpio_set_dir(BLUE_PIN, GPIO_OUT);

    char buffer[64];
    float umidade = 0.0, luminosidade = 0.0;

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        if (uart_is_readable(UART_ID)) {
            int index = 0;

            while (uart_is_readable(UART_ID)) {
                char c = uart_getc(UART_ID);
                if (c == '\n') break;
                buffer[index++] = c;
            }
            buffer[index] = '\0';

            // Extrair os valores corretamente
            if (sscanf(buffer, "Umidade:%f, Luz:%f", &umidade, &luminosidade) == 2) {
                printf("Umidade: %.2f, Luminosidade: %.2f\n", umidade, luminosidade);

                // Limpa apenas a área do texto
                memset(ssd, 0, ssd1306_buffer_length);
                
                char linha1[20], linha2[20];
                sprintf(linha1, "U: %.2f", umidade);
                sprintf(linha2, "L: %.2f", luminosidade);

                // Escreve os novos valores
                ssd1306_draw_string(ssd, 5, 0, linha1);
                ssd1306_draw_string(ssd, 5, 10, linha2);

                // Atualiza o display
                render_on_display(ssd, &frame_area);

                // Lógica para controlar os LEDs com base na umidade
                if (umidade > 45.0) {
                    // Acende o LED verde e apaga o vermelho
                    gpio_put(GREEN_PIN, 1);
                    gpio_put(BLUE_PIN, 0);
                } else {
                    // Acende o LED vermelho e apaga o verde
                    gpio_put(BLUE_PIN, 1);
                    gpio_put(GREEN_PIN, 0);
                }
            }
        }

        sleep_ms(100); // Atualização rápida para acompanhar o serial
    }

    return 0;
}
