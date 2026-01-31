/**
 * @file wifi_connection.h
 * @brief Definitions for Wi-Fi control.
 *
 * This file contains the definitions for initialization and controls of Wi-Fi,
 * including keeping the connection alive.
 * 
 * Before compiling, make sure that WIFI_SSID and WIFI_PASS are correctly set.
 *
 * @author Robson M. Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */
#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include "FreeRTOS.h"     // FreeRTOS Types
#include "task.h"         // vTaskDelay, TaskHandle_t, etc.

#define WIFI_SSID "NOME_DO_WIFI"
#define WIFI_PASS "SENHA_DO_WIFI"
#define WIFI_TASK_TIMEOUT 30000 // ms
#define WIFI_CONNECTION_TIMEOUT 5000 // ms

/**
 * @brief Attempt to connect to Wi-Fi
 * @return 1 if the connection was a success and 0 if not
 */
int wifi_connect(void);

/**
 * @brief Verify if the Wi-Fi is connected
 * @return 1 if it is connected and 0 if not
 */
int wifi_is_connected(void);

/**
 * @brief Task that keeps the Wi-Fi connection alive
 * @param pvParameters Task parameters
 */
void keep_connection_alive_task(void *pvParameters);

#endif // WIFI_CONNECTION_H