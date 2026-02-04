/**
 * @file clock.h
 * @brief Definitions for clock verify.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/util/datetime.h"
#include "FreeRTOS.h"

/**
 * @brief Inicializa o RTC (Real Time Clock).
 */
void clock_init(void);

/**
 * @brief Obtém a data e hora atuais do RTC.
 * @param t Ponteiro para a estrutura datetime_t onde os dados serão preenchidos.
 * @return true se a leitura for bem sucedida, false caso contrário.
 */
bool clock_get_time(datetime_t *t);

/**
 * @brief Tarefa que sincroniza o relógio via NTP quando há Wi-Fi.
 * @param pvParameters Parâmetros da tarefa (não utilizado).
 */
void clock_sync_task(void *pvParameters);

#endif // CLOCK_H