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
#include "irrigator.h"
#include "led_rgb.h"
#include "buzzer.h"
#include "button.h"
#include "wifi_connection.h"
#include "oled.h"
#include "aht10.h"

// Definição de prioridades (Maior valor = Maior prioridade no FreeRTOS)
#define PRIO_TASK_WIFI       1  // Baixa prioridade: conexão em background (evita travar UI)
#define PRIO_TASK_BUZZER     2
#define PRIO_TASK_OLED       3
#define PRIO_TASK_LED        4
#define PRIO_TASK_AHT10      12  // Monitoramento de sensores
#define PRIO_TASK_IRRIGATOR  10 // Prioridade crítica para controle do hardware
#define PRIO_TASK_BUTTON     11 // Prioridade máxima para garantir inicialização rápida das interrupções

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
    xTaskCreate(led_rgb_task, "LED_Task", 256, NULL, PRIO_TASK_LED, &led_rgb_task_handle);
    xTaskCreate(oled_task, "OLED_Task", 1024, NULL, PRIO_TASK_OLED, &oled_task_handle);

    // Creates the buzzer task with the same parameters.
    xTaskCreate(buzzer_task, "Buzzer_Task", 256, NULL, PRIO_TASK_BUZZER, &buzzer_task_handle);

    // Creates the button task.
    // A handle is not necessary as this task will not be controlled by others.
    // Priority set to high to ensure interrupts are initialized before heavy tasks (like WiFi).
    xTaskCreate(button_task, "Button_Task", 256, NULL, PRIO_TASK_BUTTON, NULL);

    // Creates the Wi-Fi connection management task.
    // Requires a larger stack (1024) due to network operations.
    xTaskCreate(keep_connection_alive_task, "WiFi_Task", 1024, NULL, PRIO_TASK_WIFI, NULL);

    // Creates the AHT10 Sensor task
    xTaskCreate(aht10_task, "Hum_Temp_Task", 1024*2, NULL, PRIO_TASK_AHT10, NULL);

    // 
    xTaskCreate(irrigator_task, "Irrigator_Task", 256, NULL, PRIO_TASK_IRRIGATOR, &irrigator_task_handle);

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