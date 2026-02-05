/**
 * @file api_local.h
 * @brief Definitions for api local control.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

 #ifndef API_LOCAL_H
 #define API_LOCAL_H

 #include "FreeRTOS.h"
 #include "task.h"
 
 #include "wifi_connection.h"
 #include "clock.h"
 #include "irrigator.h"

/**
 * @brief Task that initializes the local API server once Wi-Fi is connected.
 * @param pvParameters Task parameters (unused).
 */
void api_local_task(void *pvParameters);

 #endif // API_LOCAL_H