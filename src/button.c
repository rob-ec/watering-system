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

#include "irrigator.h" // For irrigator ativation

// Callback function for GPIO interrupts
void gpio_callback(uint gpio, uint32_t events);

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

    // Enable interrupts for both buttons on falling edge (press)
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true);
}

/**
 * @brief Button listener task.
 *
 * @param pvParameters Task params
 */
void button_task(void *pvParameters) {
    button_init();
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief GPIO Interrupt Callback
 */
void gpio_callback(uint gpio, uint32_t events) {
    static uint32_t last_a_time = 0;
    static uint32_t last_b_time = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (gpio == BUTTON_A_PIN) {
        // Debounce Button A
        if (now - last_a_time > DEBOUNCE_TIME_MS) {
            last_a_time = now;
            // Notify Irrigator Task to Turn ON (Action 1)
            if (irrigator_task_handle != NULL) {
                xTaskNotifyFromISR(irrigator_task_handle, 1, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
            }
        }
    } else if (gpio == BUTTON_B_PIN) {
        // Debounce Button B
        if (now - last_b_time > DEBOUNCE_TIME_MS) {
            last_b_time = now;
            // Notify Irrigator Task to Turn OFF (Action 2)
            if (irrigator_task_handle != NULL) {
                xTaskNotifyFromISR(irrigator_task_handle, 2, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
            }
        }
    }

    // Force context switch if a higher priority task was woken
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}