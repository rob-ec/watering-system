/**
 * @file buzzer.c
 * @brief Implementation of buzzer control.
 *
 * This file contains the implementation of functions for initialization and control
 * of the buzzer, including the task that generates the PWM signal for the buzzer.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#include "buzzer.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "FreeRTOS.h"
#include "task.h"

// task handler
TaskHandle_t buzzer_task_handle = NULL;

/**
 * @brief Buzzer init
 */
void buzzer_init(void)
{
    // buzzer pin ass PWM function
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();

    // Configures PWM for a frequency of ~2kHz
    pwm_config_set_wrap(&config, 4095);
    pwm_config_set_clkdiv(&config, 25.0f);

    pwm_init(slice_num, &config, true);

    pwm_set_gpio_level(BUZZER_PIN, 0); // buzzer starts turned off
}

/**
 * @brief Emit buzzer beep
 */
void buzzer_beep(int duration_ms)
{
    // Uses a default frequency of ~1220Hz (compatible with initial config)
    buzzer_play_note(1220, duration_ms);
}

/**
 * @brief Play a specific note
 */
void buzzer_play_note(int frequency, int duration_ms)
{
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint chan = pwm_gpio_to_channel(BUZZER_PIN);

    if (frequency <= 0)
    {
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        return;
    }

    // Wrap calculation for the desired frequency (Clock 125MHz / Divisor 25.0)
    // wrap = (125000000 / (frequency * 25.0)) - 1
    uint32_t wrap = (5000000 / frequency) - 1;

    pwm_set_wrap(slice_num, (uint16_t)wrap);
    pwm_set_chan_level(slice_num, chan, (uint16_t)(wrap / 2)); // 50% duty cycle

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    pwm_set_chan_level(slice_num, chan, 0);
}

void buzzer_song_of_storms()
{
    // Part 1: D4, F4, D5 (high)
    buzzer_play_note(NOTE_D4, 150);
    buzzer_play_note(NOTE_F4, 150);
    buzzer_play_note(NOTE_D5, 600);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Part 2: Repeat Part 1
    buzzer_play_note(NOTE_D4, 150);
    buzzer_play_note(NOTE_F4, 150);
    buzzer_play_note(NOTE_D5, 600);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Part 3: Rapid variation
    buzzer_play_note(NOTE_E5, 450);
    buzzer_play_note(NOTE_F5, 150);
    buzzer_play_note(NOTE_E5, 150);
    buzzer_play_note(NOTE_F5, 150);
    buzzer_play_note(NOTE_E5, 150);
    buzzer_play_note(NOTE_C5, 150);
    buzzer_play_note(NOTE_A4, 600);
}

/**
 * @brief Buzzer task.
 *
 * @param pvParameters Task params
 */
void buzzer_task(void *pvParameters)
{
    buzzer_init();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Libera a CPU para tarefas de menor prioridade
    }
}