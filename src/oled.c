#include "FreeRTOS.h"
#include "task.h"
#include "ssd1306.h"
#include "image.h"
#include "acme_5_outlines_font.h"
#include "bubblesstandard_font.h"
#include "crackers_font.h"
#include "BMSPA_font.h"
#include "irrigator.h"
#include "aht10.h"

#include <stdio.h>

TaskHandle_t oled_task_handle = NULL;

void oled_task(void *pvParameters)
{
    // Aguarda estabilização da alimentação
    vTaskDelay(pdMS_TO_TICKS(2000));

    i2c_init(i2c1, 400000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    ssd1306_t disp;
    disp.external_vcc = false;

    // Scan para detectar endereço correto (0x3C ou 0x3D)
    uint8_t oled_addr = 0x3C; // Padrão
    printf("OLED: Escaneando I2C1 (SDA=2, SCL=3)...\n");
    for (int addr = 0; addr < 128; addr++) {
        if (addr < 0x08 || addr > 0x77) continue;
        uint8_t rx;
        if (i2c_read_blocking(i2c1, addr, &rx, 1, false) >= 0) {
            printf("OLED: Encontrado em 0x%02X\n", addr);
            if (addr == 0x3C || addr == 0x3D) {
                oled_addr = addr;
            }
        }
    }
    printf("OLED: Inicializando no endereço 0x%02X\n", oled_addr);

    ssd1306_init(&disp, 128, 64, oled_addr, i2c1);
    ssd1306_clear(&disp);

    char irrigator_status[20];
    
    uint8_t data[6] = {0};
    float temp = 0.0;
    float hum = 0.0;
    char temperature_status[20];
    char humidity_status[20];

    while (true)
    {
        aht10_get_latest_readings(&temp, &hum);

        snprintf(temperature_status, sizeof(temperature_status), "%.2f C", temp);
        snprintf(humidity_status, sizeof(humidity_status), "%.2f %%", hum);

        snprintf(irrigator_status, sizeof(irrigator_status), "%s", irrigator_is_on() ? "LIGADO" : "DESLIGADO");

        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 16, 2, irrigator_status);
        ssd1306_draw_string(&disp, 0, 32, 1, "Irrigador");
        ssd1306_show(&disp);

        // Aguarda 2000ms ou até receber notificação (mudança de status)
        // Se receber notificação (>0), reinicia o loop para atualizar o status imediatamente
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000)) > 0)
            continue;

        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 24, 2, temperature_status);
        ssd1306_draw_string(&disp, 0, 40, 1, "Temperatura");
        ssd1306_show(&disp);

        // Aguarda 2000ms ou até receber notificação
        // Se notificado, acorda e volta para o início do loop (Status)
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));

        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 24, 2, humidity_status);
        ssd1306_draw_string(&disp, 0, 40, 1, "Humidade");
        ssd1306_show(&disp);

        // Aguarda 2000ms ou até receber notificação
        // Se notificado, acorda e volta para o início do loop (Status)
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
    }
}