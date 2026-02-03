#include "aht10.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>

static float latest_temp = 0.0f;
static float latest_hum = 0.0f;

void aht10_get_latest_readings(float *temp, float *hum) {
    *temp = latest_temp;
    *hum = latest_hum;
}

static void aht10_i2c_init(void) {
    // 1. Procedimento de "Bus Recovery" para destravar sensores I2C presos
    // Configura SCL como saída e SDA como entrada temporariamente
    gpio_init(AHT10_I2C_SDA);
    gpio_set_dir(AHT10_I2C_SDA, GPIO_IN);
    gpio_pull_up(AHT10_I2C_SDA);

    gpio_init(AHT10_I2C_SCL);
    gpio_set_dir(AHT10_I2C_SCL, GPIO_OUT);
    gpio_pull_up(AHT10_I2C_SCL);

    // Envia 9 pulsos de clock para liberar o SDA se o escravo estiver segurando-o
    for (int i = 0; i < 9; i++) {
        gpio_put(AHT10_I2C_SCL, 0);
        sleep_us(10);
        gpio_put(AHT10_I2C_SCL, 1);
        sleep_us(10);
    }
    
    // Envia condição de STOP
    gpio_set_dir(AHT10_I2C_SDA, GPIO_OUT);
    gpio_put(AHT10_I2C_SDA, 0);
    sleep_us(10);
    gpio_put(AHT10_I2C_SCL, 1);
    sleep_us(10);
    gpio_put(AHT10_I2C_SDA, 1);
    sleep_us(10);

    // 2. Inicializa o periférico I2C0 hardware
    i2c_init(i2c0, 100000); // 500kHz padrão
    
    gpio_set_function(AHT10_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(AHT10_I2C_SCL, GPIO_FUNC_I2C);
    
    // Habilita pull-ups internos (essencial se o módulo não tiver pull-ups fortes)
    gpio_pull_up(AHT10_I2C_SDA);
    gpio_pull_up(AHT10_I2C_SCL);
}

static void aht10_scan_bus(void) {
    printf("\n--- AHT10: Iniciando Scan I2C0 (SDA=%d, SCL=%d) ---\n", AHT10_I2C_SDA, AHT10_I2C_SCL);
    bool found_any = false;
    for (int addr = 0; addr < 128; addr++) {
        if (addr < 0x08 || addr > 0x77) continue; // Endereços reservados
        
        uint8_t rxdata;
        int ret = i2c_read_blocking(i2c0, addr, &rxdata, 1, false);
        
        if (ret >= 0) {
            printf("I2C0: Dispositivo encontrado em 0x%02X\n", addr);
            found_any = true;
        }
    }
    if (!found_any) {
        printf("I2C0: ERRO CRÍTICO - Nenhum dispositivo encontrado!\n");
        printf("I2C0: Verifique se SDA e SCL não estão invertidos.\n");
    } else {
        printf("I2C0: Scan concluído.\n");
    }
    printf("---------------------------------------------------\n");
}

static bool aht10_check_connection(void) {
    uint8_t rxdata;
    // Tenta ler 1 byte apenas para ver se o sensor responde com ACK
    int ret = i2c_read_blocking(i2c0, AHT10_ADDR, &rxdata, 1, false);
    return ret >= 0;
}

static void aht10_soft_reset(void) {
    uint8_t cmd = AHT10_CMD_SOFT_RESET;
    i2c_write_blocking(i2c0, AHT10_ADDR, &cmd, 1, false);
    vTaskDelay(pdMS_TO_TICKS(30)); // Datasheet pede 20ms, damos 30ms
}

static void aht10_calibrate(void) {
    // Comando de inicialização: 0xE1, 0x08, 0x00
    uint8_t cmd[3] = {AHT10_CMD_INIT, 0x08, 0x00};
    i2c_write_blocking(i2c0, AHT10_ADDR, cmd, 3, false);
    vTaskDelay(pdMS_TO_TICKS(20));
}

void aht10_task(void *pvParameters) {
    aht10_i2c_init();
    
    // Aguarda 3 segundos para garantir que o USB Serial esteja conectado
    // e o sensor tenha estabilizado a tensão.
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Executa o scan para diagnóstico
    aht10_scan_bus();

    // 1. Loop de conexão: Não sai daqui até o sensor responder
    while (true) {
        if (aht10_check_connection()) {
            printf("AHT10: Sensor conectado e respondendo em 0x38.\n");
            break;
        }
        printf("AHT10: Aguardando sensor... (Tentando 0x38)\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    // 2. Reset e Calibração inicial
    printf("AHT10: Resetando e Calibrando...\n");
    aht10_soft_reset();
    aht10_calibrate();

    uint8_t trigger_cmd[3] = {AHT10_CMD_TRIGGER, 0x33, 0x00};
    uint8_t data[6];

    while (1) {
        // 3. Envia comando de medição
        int ret = i2c_write_blocking(i2c0, AHT10_ADDR, trigger_cmd, 3, false);
        if (ret < 0) {
            printf("AHT10: Erro de comunicação (Trigger). Ret: %d\n", ret);
            // Tenta um soft reset para destravar
            aht10_soft_reset();
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // 4. Aguarda a medição (datasheet recomenda > 75ms)
        vTaskDelay(pdMS_TO_TICKS(100));

        // 5. Lê os 6 bytes de dados
        ret = i2c_read_blocking(i2c0, AHT10_ADDR, data, 6, false);
        if (ret < 0) {
            printf("AHT10: Erro de leitura dos dados.\n");
        } else {
            // Byte 0: Status
            uint8_t status = data[0];

            // Verifica se o sensor está ocupado (Bit 7)
            if ((status & AHT10_STATUS_BUSY) == 0) {
                // Verifica se está calibrado (Bit 3)
                if (!(status & AHT10_STATUS_CALIBRATED)) {
                    printf("AHT10: Sensor não calibrado. Recalibrando...\n");
                    aht10_calibrate();
                } else {
                    // Conversão dos dados (20 bits)
                    uint32_t raw_hum = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((uint32_t)data[3] >> 4);
                    uint32_t raw_temp = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | (uint32_t)data[5];

                    // Fórmulas do datasheet
                    float h = ((float)raw_hum * 100.0f) / 1048576.0f;
                    float t = (((float)raw_temp * 200.0f) / 1048576.0f) - 50.0f;

                    latest_hum = h;
                    latest_temp = t;
                    
                    // Descomente para debug no serial
                    // printf("AHT10: Temp=%.2f C, Hum=%.2f %%\n", t, h);
                }
            } else {
                printf("AHT10: Sensor ocupado (Busy)\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // Lê a cada 2 segundos
    }
}
