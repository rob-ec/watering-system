/**
 * @file wifi_connection.c
 * @brief Implementation of Wi-Fi control.
 *
 * This file contains the task implementation to control Wi-Fi.
 *
 * @author Robson Gomes
 * @email robson.mesquita56@gmail.com
 * @github github.com/rob-ec
 */
#include "wifi_connection.h" // ssid info, pins & prototypes
#include "pico/cyw43_arch.h" // cyw43_arch_...
#include "pico/stdlib.h"     // gpio_...
#include "FreeRTOS.h"        // FreeRTOS Types
#include "task.h"            // vTaskDelay, TaskHandle_t, etc.
#include "lwip/dns.h"

static volatile int internet_connected = 0;

int wifi_has_internet(void)
{
    return internet_connected;
}

int wifi_is_connected(void)
{
    return cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_JOIN;
}

int wifi_connect(void)
{
    return cyw43_arch_wifi_connect_timeout_ms(
               WIFI_SSID,
               WIFI_PASS,
               CYW43_AUTH_WPA2_AES_PSK,
               WIFI_CONNECTION_TIMEOUT) == 0;
}

static void dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    if (ipaddr) {
        internet_connected = 1;
    } else {
        internet_connected = 0;
    }
}

void keep_connection_alive_task(void *pvParameters)
{
    if (cyw43_arch_init())
    {
        printf("Falha ao inicializar o Wi-Fi\n");
        vTaskDelete(NULL);
        return;
    }

    cyw43_arch_enable_sta_mode();

    for (;;)
    {
        if (!wifi_is_connected())
        {
            internet_connected = 0;
            printf("Conectando ao Wi-Fi: %s...\n", WIFI_SSID);

            if (wifi_connect())
            {
                printf("Wi-Fi Conectado!\n");
                struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
                printf("IP: %s\n", ip4addr_ntoa(netif_ip4_addr(n)));
            }
            else
            {
                printf("Falha na conexão Wi-Fi.\n");
            }
        }
        else
        {
            cyw43_arch_lwip_begin();
            ip_addr_t ip;
            int err = dns_gethostbyname("google.com", &ip, dns_found_cb, NULL);
            cyw43_arch_lwip_end();

            if (err == ERR_OK) {
                internet_connected = 1;
            } else if (err != ERR_INPROGRESS) {
                internet_connected = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(WIFI_TASK_TIMEOUT));
    }
}
