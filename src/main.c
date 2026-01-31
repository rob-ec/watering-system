/**
 * @file main.c
 * @brief Main entry point of the multitasking system for BitDogLab.
 *
 * This file initializes the system, creates tasks for the RGB LED,
 * the buzzer, and the buttons, and starts the FreeRTOS scheduler.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

// Includes headers for peripheral modules
#include "led_rgb.h"
#include "buzzer.h"
#include "button.h"
#include "wifi_connection.h"

/**
 * @brief Main program entry point.
 *
 * - Initializes standard I/O (for USB debugging).
 * - Creates concurrent tasks:
 * 1. led_rgb_task: Controls the RGB LED.
 * 2. buzzer_task: Controls the buzzer.
 * 3. button_task: Monitors buttons to control the other two tasks.
 * - Starts the FreeRTOS scheduler.
 *
 * @return int Never returns, as control is passed to FreeRTOS.
 */
int main() {
    // Initializes USB serial communication for debugging (optional)
    stdio_init_all();

    // Creates the RGB LED task.
    // Parameters:
    // - led_rgb_task: The task function.
    // - "LED_Task": Task name (for debugging).
    // - 256: Stack size in words (256 * 4 bytes).
    // - NULL: Task parameters (none).
    // - 1: Task priority (lower priorities first).
    // - &led_rgb_task_handle: Handle to control the task.
    xTaskCreate(led_rgb_task, "LED_Task", 256, NULL, 1, &led_rgb_task_handle);

    // Creates the buzzer task with the same parameters.
    xTaskCreate(buzzer_task, "Buzzer_Task", 256, NULL, 1, &buzzer_task_handle);

    // Creates the button task.
    // A handle is not necessary as this task will not be controlled by others.
    // Priority is 2, higher than others, to ensure buttons have a fast response.
    xTaskCreate(button_task, "Button_Task", 256, NULL, 2, NULL);

    // Creates the Wi-Fi connection management task.
    // Requires a larger stack (1024) due to network operations.
    xTaskCreate(keep_connection_alive_task, "WiFi_Task", 1024, NULL, 1, NULL);

    // Starts the FreeRTOS scheduler.
    // From this point on, FreeRTOS takes control of the processor
    // and starts executing the created tasks.
    vTaskStartScheduler();

    // The code below will never be reached unless there is a serious error
    // and the scheduler stops (e.g., out of memory).
    while (1) {
        // Safety infinite loop.
    };
}