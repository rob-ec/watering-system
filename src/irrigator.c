/**
 * @file irrigator.c
 * @brief Implementation of Irrigator control.
 *
 * This file contains the implementation of the task that controls Irrigator.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#include "irrigator.h"     // For function prototype and pin definitions
#include "pico/stdlib.h" // For gpio_... functions
#include "FreeRTOS.h"    // For FreeRTOS types
#include "task.h"        // For vTaskDelay, TaskHandle_t, etc.
#include "buzzer.h"      // For buzzer feedback
#include <stdio.h>       // For printf
#include <limits.h>      // For ULONG_MAX

volatile uint8_t irrigator_on = 0;
TaskHandle_t irrigator_task_handle = NULL;

void irrigator_init(void)
{
    gpio_init(IRRIGATOR_PIN);
    gpio_set_dir(IRRIGATOR_PIN, GPIO_OUT);
    gpio_put(IRRIGATOR_PIN, 0); // irrigator starts turned off
}

void irrigator_turn_on(void)
{
    gpio_put(IRRIGATOR_PIN, 1);
    irrigator_on = 1;
}

void irrigator_turn_off(void)
{
    gpio_put(IRRIGATOR_PIN, 0);
    irrigator_on = 0;
}

void irrigator_toggle(void)
{
    if (irrigator_on)
    {
        irrigator_turn_off();
    }
    else
    {
        irrigator_turn_on();
    }
}

int irrigator_is_on(void)
{
    return irrigator_on;
}

void irrigator_task(void *pvParameters)
{
    irrigator_init();
    uint32_t notification_value;

    while (1)
    {
        // Wait for notification from ISR (Button)
        // 1 = Turn ON request, 2 = Turn OFF request
        xTaskNotifyWait(0, ULONG_MAX, &notification_value, portMAX_DELAY);

        if (notification_value == IRRIGATOR_TURN_ON) // Button A Action
        {
            if (!irrigator_is_on()) {
                irrigator_turn_on();
                buzzer_song_of_storms();
                printf("Irrigação ativada!\n");
            } else {
                printf("A irrigaçao já está ativada!\n");
            }
        }
        else if (notification_value == IRRIGATOR_TURN_OFF) // Button B Action
        {
            if (irrigator_is_on()) {
                irrigator_turn_off();
                buzzer_play_note(NOTE_C4, 200);
                printf("Irrigação desativada!\n");
            } else {
                buzzer_play_note(NOTE_C5, 200);
                printf("A irrigaçao já está desativada!\n");
            }
        }
    }
}