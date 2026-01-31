/**
 * @file led_rgb.c
 * @brief Implementation of RGB LED control.
 *
 * This file contains the implementation of the task that toggles RGB LED colors.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#include "led_rgb.h"     // For function prototype and pin definitions
#include "pico/stdlib.h" // For gpio_... functions
#include "FreeRTOS.h"    // For FreeRTOS types
#include "task.h"        // For vTaskDelay, TaskHandle_t, etc.

// Array with LED pins for easier access.
const uint8_t led_pins[] = {LED_R_PIN, LED_G_PIN, LED_B_PIN};

// Task handle, used by the button task to suspend/resume this one.
// The variable is declared here, but main.c will assign the value to it.
TaskHandle_t led_rgb_task_handle = NULL;

void led_rgb_init(void)
{
    for (int i = 0; i < 3; i++)
    {
        gpio_init(led_pins[i]);
        gpio_set_dir(led_pins[i], GPIO_OUT);
        gpio_put(led_pins[i], 0); // leds starts turned off
    }
}

void led_rgb_set_color(int r, int g, int b)
{
    gpio_put(LED_R_PIN, r);
    gpio_put(LED_G_PIN, g);
    gpio_put(LED_B_PIN, b);
}

void led_rgb_set_hex_color(uint32_t hex_color)
{
    int r = (hex_color >> 16) & 0xFF;
    int g = (hex_color >> 8) & 0xFF;
    int b = hex_color & 0xFF;

    led_rgb_set_color(r, g, b);
}

/**
 * @brief Task that controls the RGB LED.
 *
 * Initializes LED pins and enters an infinite loop to
 * cyclically toggle between red, green, and blue colors
 * every 500ms. This task can be suspended and resumed by the button task.
 *
 * @param pvParameters Task parameters (unused).
 */
void led_rgb_task(void *pvParameters)
{
    led_rgb_init();

    while (1)
    {
    }
}
