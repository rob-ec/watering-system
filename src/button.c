/**
 * @file button.c
 * @brief Implements button control
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#include <stdio.h>

#include "button.h"
#include "led_rgb.h"
#include "buzzer.h"
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Buttons init
 *
 * Set the button pins as digital inputs (pull-up)
 */
void button_init(void) {
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);

    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
}

/**
 * @brief Button listener task.
 *
 * @param pvParameters Task params
 */
void button_task(void *pvParameters) {
    button_init();

    // Timestamps for non-block debounce
    uint32_t a_last_press_time = 0;
    uint32_t b_last_press_time = 0;

    while (1) {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        if (!gpio_get(BUTTON_A_PIN)) {
            // Debounce: Button A
            if (now - a_last_press_time > DEBOUNCE_TIME_MS) {
                a_last_press_time = now; // updates click time

                // BUTTON A ACTION
                buzzer_song_of_storms();
                printf("Button A pressed\n");
            }
        }

        if (!gpio_get(BUTTON_B_PIN)) {
            // Debounce: Button B
            if (now - b_last_press_time > DEBOUNCE_TIME_MS) {
                b_last_press_time = now; // updates click time

                // BUTTON B ACTION
                printf("Button B pressed\n");
                buzzer_play_note(NOTE_C4, 200);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}