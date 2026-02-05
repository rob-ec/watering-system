/**
 * @file clock.c
 * @brief Implementation of clock functionality using RP2040 RTC.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */

#include "clock.h"
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "wifi_connection.h"
#include "FreeRTOS.h"
#include "task.h"

static u_int8_t ntp_synchronized = 0;

// --- Implementação RTC ---


void clock_init(void)
{
    rtc_init();

    // Configura uma data/hora inicial padrão (Segunda, 01/01/2024 12:00:00)
    // Isso garante que o RTC comece a contar se ainda não estiver rodando.
    datetime_t t = {
        .year  = 2024,
        .month = 1,
        .day   = 1,
        .dotw  = 1, // Segunda-feira
        .hour  = 12,
        .min   = 0,
        .sec   = 0
    };

    // Inicializa o RTC com a data definida
    rtc_set_datetime(&t);
}

bool is_ntp_synchronized(void)
{
    return ntp_synchronized;
}

bool clock_get_time(datetime_t *t)
{
    return rtc_get_datetime(t);
}

// --- Implementação NTP ---

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // Segundos entre 1900 e 1970
#define TIMEZONE_OFFSET (-3 * 3600) // UTC-3 (Brasília)

static TaskHandle_t sync_task_handle = NULL;

// Callback chamado quando a resposta NTP é recebida
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    if (p && p->tot_len == NTP_MSG_LEN) {
        uint8_t mode = pbuf_get_at(p, 0) & 0x07;
        // Mode 4 é server
        if (mode == 4) {
            uint8_t seconds_buf[4];
            pbuf_copy_partial(p, seconds_buf, 4, 40); // Transmit Timestamp começa no byte 40
            
            // Converte de Network Byte Order para Host
            uint32_t seconds_since_1900 = (seconds_buf[0] << 24) | (seconds_buf[1] << 16) | (seconds_buf[2] << 8) | seconds_buf[3];
            time_t epoch = seconds_since_1900 - NTP_DELTA;
            epoch += TIMEZONE_OFFSET;

            // Converte epoch para struct tm e depois para datetime_t
            struct tm *tm_info = gmtime(&epoch);
            datetime_t dt = {
                .year  = tm_info->tm_year + 1900,
                .month = tm_info->tm_mon + 1,
                .day   = tm_info->tm_mday,
                .dotw  = tm_info->tm_wday,
                .hour  = tm_info->tm_hour,
                .min   = tm_info->tm_min,
                .sec   = tm_info->tm_sec
            };

            rtc_set_datetime(&dt);
            
            // Notifica a tarefa de sucesso
            if (sync_task_handle) {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xTaskNotifyFromISR(sync_task_handle, 1, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
        }
    }
    pbuf_free(p);
}

// Callback chamado quando o DNS resolve o IP do servidor NTP
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    if (ipaddr) {
        struct udp_pcb *pcb = (struct udp_pcb *)arg;
        
        struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
        uint8_t *req = (uint8_t *)p->payload;
        memset(req, 0, NTP_MSG_LEN);
        req[0] = 0x1B; // LI=0, VN=3, Mode=3 (Client)

        udp_sendto(pcb, p, ipaddr, NTP_PORT);
        pbuf_free(p);
    } else {
        // Falha no DNS
         if (sync_task_handle) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xTaskNotifyFromISR(sync_task_handle, 0, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void clock_sync_task(void *pvParameters) {
    sync_task_handle = xTaskGetCurrentTaskHandle();

    while (1) {
        // Só tenta sincronizar se estiver conectado ao Wi-Fi
        if (wifi_is_connected()) {
            printf("Clock: Iniciando sincronização NTP...\n");
            
            cyw43_arch_lwip_begin();
            struct udp_pcb *pcb = udp_new();
            udp_recv(pcb, ntp_recv, NULL);
            
            ip_addr_t ip;
            int err = dns_gethostbyname(NTP_SERVER, &ip, ntp_dns_found, pcb);
            
            if (err == ERR_OK) {
                // IP já estava em cache, chama callback manualmente
                ntp_dns_found(NTP_SERVER, &ip, pcb);
            }
            cyw43_arch_lwip_end();

            // Aguarda notificação (timeout 10s)
            uint32_t result = 0;
            if (xTaskNotifyWait(0, 0, &result, pdMS_TO_TICKS(10000)) == pdTRUE) {
                if (result == 1) {
                    ntp_synchronized = 1;
                    printf("Clock: Sincronizado com sucesso!\n");
                    
                    // Limpeza
                    cyw43_arch_lwip_begin();
                    udp_remove(pcb);
                    cyw43_arch_lwip_end();

                    // Sincronização bem sucedida, aguarda 24h antes da próxima
                    vTaskDelay(pdMS_TO_TICKS(24 * 3600 * 1000));
                    continue;
                }
            }
            
            // Timeout ou falha: Limpa PCB e tenta novamente mais tarde
            cyw43_arch_lwip_begin();
            udp_remove(pcb);
            cyw43_arch_lwip_end();
        } else {
            // Wi-Fi desconectado
            ntp_synchronized = 0;
        }
        
        // Se falhou ou não conectado, tenta novamente em 1 minuto
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
