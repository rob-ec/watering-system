#ifndef AHT10_H
#define AHT10_H

#include "FreeRTOS.h"
#include "task.h"

// Definição dos pinos I2C0 (Adaptado: SDA=GP0, SCL=GP1)
#define AHT10_I2C_SDA 0
#define AHT10_I2C_SCL 1

// Endereço padrão do AHT10
#define AHT10_ADDR 0x38

// Comandos do AHT10
#define AHT10_CMD_INIT          0xE1
#define AHT10_CMD_TRIGGER       0xAC
#define AHT10_CMD_SOFT_RESET    0xBA
#define AHT10_STATUS_BUSY       0x80
#define AHT10_STATUS_CALIBRATED 0x08

// Protótipo da tarefa
void aht10_task(void *pvParameters);

/**
 * @brief Obtém os últimos valores de temperatura e umidade lidos pela tarefa.
 * @param temp Ponteiro para armazenar a temperatura.
 * @param hum Ponteiro para armazenar a umidade.
 */
void aht10_get_latest_readings(float *temp, float *hum);

#endif // AHT10_H