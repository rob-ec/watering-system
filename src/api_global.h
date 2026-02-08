/**
 * @file api_global.h
 * @brief Definitions for api global control.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

 #ifndef API_GLOBAL_H
 #define API_GLOBAL_H

 #include "FreeRTOS.h"
 #include "task.h"

 // https://sua-api.exemplo.com
 #define API_GLOBAL_URL "URL_DA_SUA_API"
 #define API_PORT 80

 // login
 #define API_CONNECTION_SERIAL_NUMBER "NUMERO_SERIAL_OU_LOGIN"
 #define API_CONNECTION_SECRET_TOKEN "TOKEN_OU_SENHA"

/**
 * @brief Task that initializes the global API connection once Wi-Fi is connected.
 * @param pvParameters Task parameters (unused).
 */
void api_global_task(void *pvParameters);

 #endif // API_GLOBAL_H