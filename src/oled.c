#include "FreeRTOS.h"
#include "task.h"
#include "ssd1306.h"
#include "image.h"
#include "acme_5_outlines_font.h"
#include "bubblesstandard_font.h"
#include "crackers_font.h"
#include "BMSPA_font.h"
#include "irrigator.h"

#include <stdio.h>

TaskHandle_t oled_task_handle = NULL;

void oled_task(void *pvParameters)
{
    i2c_init(i2c1, 400000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    ssd1306_t disp;
    disp.external_vcc = false;

    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&disp);

    char irrigator_status[20];
    
    while (true)
    {
        snprintf(irrigator_status, sizeof(irrigator_status), "%s", irrigator_is_on() ? "LIGADO" : "DESLIGADO");

        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 16, 2, irrigator_status);
        ssd1306_draw_string(&disp, 0, 32, 1, "Irrigador");
        ssd1306_show(&disp);

        // Aguarda 2000ms ou até receber notificação (mudança de status)
        // Se receber notificação (>0), reinicia o loop para atualizar o status imediatamente
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000)) > 0) continue;

        ssd1306_clear(&disp);
        ssd1306_draw_string(&disp, 0, 24, 2, "Robson");
        ssd1306_draw_string(&disp, 0, 40, 1, "Gomes");
        ssd1306_show(&disp);

        // Aguarda 2000ms ou até receber notificação
        // Se notificado, acorda e volta para o início do loop (Status)
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
    }
}