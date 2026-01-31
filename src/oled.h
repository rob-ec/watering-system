/**
 * @file oled.h
 * @brief Definitions for OLED control.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */
#ifndef OLED_H
#define OLED_H

#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Handle for the OLED task.
 * Used to control the task (suspend/resume) from other tasks.
 */
extern TaskHandle_t oled_task_handle;

void oled_task(void *pvParameters);

#endif // OLED_H