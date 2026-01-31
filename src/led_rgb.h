/**
 * @file led_rgb.h
 * @brief Definitions for RGB LED control.
 *
 * This file contains definitions for initialization and control of the RGB LED,
 * including suspending and resuming the LED task.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#ifndef LED_RGB_H
#define LED_RGB_H

#include "FreeRTOS.h"
#include "task.h"

// RGB LED pins according to BitDogLab V6 schematic
#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12

/**
 * @brief Handle for the RGB LED task.
 * Used to control the task (suspend/resume) from other tasks.
 */
extern TaskHandle_t led_rgb_task_handle;

void led_rgb_init(void);
void led_rgb_set_color(int r, int g, int b);
void led_rgb_set_hex_color(uint32_t hex_color);
void led_rgb_turn_off(void);

/**
 * @brief Task that controls the RGB LED color cycle.
 * @param pvParameters Task parameters (unused).
 */
void led_rgb_task(void *pvParameters);

#endif // LED_RGB_H