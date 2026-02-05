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

#include "irrigator.h"   // For function prototype and pin definitions
#include "pico/stdlib.h" // For gpio_... functions
#include "FreeRTOS.h"    // For FreeRTOS types
#include "task.h"        // For vTaskDelay, TaskHandle_t, etc.
#include "buzzer.h"      // For buzzer feedback
#include <stdio.h>       // For printf
#include <limits.h>      // For ULONG_MAX
#include "oled.h"        // For oled_task_handle
#include "clock.h"

static uint8_t irrigator_on = 0;
static schedule_item_t schedule[IRRIGATOR_MAX_SCHEDULE_SIZE];
TaskHandle_t irrigator_task_handle = NULL;

void irrigator_set_schedule(int index, uint8_t hour, uint8_t minute, uint8_t duration, uint8_t active)
{
    if (index >= 0 && index < IRRIGATOR_MAX_SCHEDULE_SIZE)
    {
        schedule[index].hour = hour;
        schedule[index].minute = minute;
        schedule[index].duration = duration;
        schedule[index].active = active;
    }
}

void irrigator_get_all_schedules(schedule_item_t *items)
{
    for (int i = 0; i < IRRIGATOR_MAX_SCHEDULE_SIZE; i++)
    {
        items[i] = schedule[i];
    }
}

void irrigator_init(void)
{
    gpio_init(IRRIGATOR_PIN);
    gpio_set_dir(IRRIGATOR_PIN, GPIO_OUT);
    gpio_put(IRRIGATOR_PIN, 0); // irrigator starts turned off

    // default schedule for temporary tests
    // irrigator_set_schedule(0, 13, 12, 60 * 2, 1);
}

void irrigator_turn_on(void)
{
    gpio_put(IRRIGATOR_PIN, 1);
    irrigator_on = 1;
    if (oled_task_handle != NULL)
        xTaskNotifyGive(oled_task_handle);
}

void irrigator_turn_off(void)
{
    gpio_put(IRRIGATOR_PIN, 0);
    irrigator_on = 0;
    if (oled_task_handle != NULL)
        xTaskNotifyGive(oled_task_handle);
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
    int remaining_duration = 0;
    datetime_t t;
    uint8_t last_sec = 60;

    while (1)
    {
        // Wait for notification from ISR (Button) or other tasks
        if (xTaskNotifyWait(0, ULONG_MAX, &notification_value, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            // Handle turning ON
            if (notification_value == IRRIGATOR_TURN_ON || notification_value == IRRIGATOR_REMOTE_TURN_ON)
            {
                if (!irrigator_is_on())
                {
                    irrigator_turn_on();
                    remaining_duration = 0; // Manual/Remote activation
                    if (notification_value == IRRIGATOR_TURN_ON)
                    { // Buzzer only for button
                        buzzer_song_of_storms();
                        printf("Irrigação ativada!\n");
                    }
                    else
                    {
                        printf("Irrigação ativada remotamente!\n");
                    }
                }
                else
                {
                    if (notification_value == IRRIGATOR_TURN_ON)
                    {
                        printf("A irrigaçao já está ativada!\n");
                    }
                }
            }
            // Handle turning OFF
            else if (notification_value == IRRIGATOR_TURN_OFF || notification_value == IRRIGATOR_REMOTE_TURN_OFF)
            {
                if (irrigator_is_on())
                {
                    irrigator_turn_off();
                    remaining_duration = 0;
                    if (notification_value == IRRIGATOR_TURN_OFF)
                    { // Buzzer only for button
                        buzzer_play_note(NOTE_C4, 200);
                        printf("Irrigação desativada!\n");
                    }
                    else
                    {
                        printf("Irrigação desativada remotamente!\n");
                    }
                }
                else
                {
                    if (notification_value == IRRIGATOR_TURN_OFF)
                    {
                        buzzer_play_note(NOTE_C5, 200);
                        printf("A irrigaçao já está desativada!\n");
                    }
                }
            }
        }

        // Check schedule
        if (clock_get_time(&t))
        {
            if (t.sec != last_sec)
            {
                last_sec = t.sec;

                // Handle duration
                if (irrigator_is_on() && remaining_duration > 0)
                {
                    remaining_duration--;
                    if (remaining_duration == 0)
                    {
                        irrigator_turn_off();
                        // buzzer_play_note(NOTE_C4, 200); // Buzzer removido conforme solicitado
                        printf("Irrigação finalizada pelo agendamento.\n");
                    }
                }

                // Check start triggers
                if (t.sec == 0)
                {
                    for (int i = 0; i < IRRIGATOR_MAX_SCHEDULE_SIZE; i++)
                    {
                        if (schedule[i].active && schedule[i].hour == t.hour && schedule[i].minute == t.min)
                        {
                            if (!irrigator_is_on())
                            {
                                irrigator_turn_on();
                                remaining_duration = schedule[i].duration;
                                // buzzer_song_of_storms(); // Buzzer removido conforme solicitado
                                printf("Irrigação iniciada por agendamento: %02d:%02d por %d s.\n", t.hour, t.min, remaining_duration);
                            }
                        }
                    }
                }
            }
        }
    }
}