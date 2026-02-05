/**
 * @file api_local.c
 * @brief Implementation of local API using lwIP Raw API.
 *
 * @author Robson Gomes
 */

#include "api_local.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "aht10.h"
#include "wifi_connection.h"
#include "irrigator.h"
#include "hardware/rtc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define RX_BUFFER_SIZE 4096

static struct tcp_pcb *server_pcb;

// Helper to find JSON string value by key
// Very basic parser: looks for "key": "value"
static void get_json_value(const char *json, const char *key, char *value, size_t max_len) {
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char *key_pos = strstr(json, search_key);
    if (!key_pos) {
        value[0] = '\0';
        return;
    }

    const char *colon = strchr(key_pos, ':');
    if (!colon) return;

    const char *quote_start = strchr(colon, '\"');
    if (!quote_start) return;
    quote_start++; // Skip opening quote

    const char *quote_end = strchr(quote_start, '\"');
    if (!quote_end) return;

    size_t len = quote_end - quote_start;
    if (len >= max_len) len = max_len - 1;

    strncpy(value, quote_start, len);
    value[len] = '\0';
}

static int get_json_int_value(const char *json, const char *key, int default_val) {
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char *key_pos = strstr(json, search_key);
    if (!key_pos) return default_val;

    const char *colon = strchr(key_pos, ':');
    if (!colon) return default_val;

    const char *val_start = colon + 1;
    while (*val_start && (*val_start == ' ' || *val_start == '\t' || *val_start == '\"')) val_start++;
    
    return atoi(val_start);
}

static bool get_json_bool_value(const char *json, const char *key, bool default_val) {
    char search_key[64];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char *key_pos = strstr(json, search_key);
    if (!key_pos) return default_val;

    const char *colon = strchr(key_pos, ':');
    if (!colon) return default_val;

    const char *val_start = colon + 1;
    while (*val_start && (*val_start == ' ' || *val_start == '\t' || *val_start == '\"')) val_start++;
    
    if (strncmp(val_start, "true", 4) == 0) return true;
    if (strncmp(val_start, "false", 5) == 0) return false;
    
    return default_val;
}

static err_t http_send_response(struct tcp_pcb *pcb, const char *payload, int code) {
    char header[128];
    const char *status_str = (code == 200) ? "OK" : "Bad Request";
    
    snprintf(header, sizeof(header), 
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        code, status_str, (int)strlen(payload));
    
    size_t payload_len = strlen(payload);
    
    // Debug: Verifica se o buffer TCP comporta a resposta
    if (tcp_sndbuf(pcb) < strlen(header) + payload_len) {
        printf("API Warning: Response size (%d) > TCP Send Buffer (%d)\n", 
               (int)(strlen(header) + payload_len), tcp_sndbuf(pcb));
    }

    tcp_write(pcb, header, strlen(header), TCP_WRITE_FLAG_COPY);
    
    // Envia o payload em chunks para evitar erro de alocação (ERR_MEM) no lwIP
    size_t sent = 0;
    while (sent < payload_len) {
        size_t chunk = payload_len - sent;
        if (chunk > 1024) chunk = 1024; // Limita o chunk a 1KB
        
        err_t err = tcp_write(pcb, payload + sent, chunk, TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            printf("API Error: tcp_write failed at offset %d (err %d)\n", (int)sent, err);
            break;
        }
        sent += chunk;
    }
    
    tcp_output(pcb);
    
    return ERR_OK;
}

static err_t http_recv_callback(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(pcb);
        return ERR_OK;
    }

    char *rx_buffer = malloc(RX_BUFFER_SIZE);
    if (!rx_buffer) {
        pbuf_free(p);
        tcp_close(pcb);
        return ERR_MEM;
    }

    // Copy pbuf to buffer (limit to buffer size)
    u16_t len = (p->tot_len < RX_BUFFER_SIZE - 1) ? p->tot_len : RX_BUFFER_SIZE - 1;
    pbuf_copy_partial(p, rx_buffer, len, 0);
    rx_buffer[len] = '\0';
    
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);

    // Simple routing
    if (strncmp(rx_buffer, "GET / ", 6) == 0 || strncmp(rx_buffer, "GET / HTTP", 10) == 0) {
        // Handle GET /
        datetime_t t;
        if (!clock_get_time(&t)) {
            memset(&t, 0, sizeof(t));
        }

        char response[256];
        snprintf(response, sizeof(response), 
            "{\"hardwareVersion\": \"BitDogLab V6.3\", \"systemTime\": {"
            "\"year\": %d, \"month\": %d, \"day\": %d, "
            "\"dotw\": %d, \"hour\": %d, \"min\": %d, \"sec\": %d}}",
            t.year, t.month, t.day, t.dotw, t.hour, t.min, t.sec);
        
        http_send_response(pcb, response, 200);

    } else if (strncmp(rx_buffer, "POST /serial ", 13) == 0) {
        // Handle POST /serial
        char *body = strstr(rx_buffer, "\r\n\r\n");
        if (body) {
            body += 4; // Skip CRLFCRLF
            
            printf("DEBUG Body: %s\n", body);

            char author[32] = {0};
            char message[64] = {0};
            
            get_json_value(body, "author", author, sizeof(author));
            get_json_value(body, "message", message, sizeof(message));
            
            // Fallback: Se o parse falhou mas tem corpo, usa o corpo como mensagem
            if (author[0] == '\0' && message[0] == '\0' && strlen(body) > 0) {
                snprintf(message, sizeof(message), "%s", body);
                snprintf(author, sizeof(author), "RAW");
            }

            printf("API Serial: [%s] %s\n", author, message);
            http_send_response(pcb, "{\"status\": \"received\"}", 200);
        } else {
            http_send_response(pcb, "{\"error\": \"no body\"}", 400);
        }
    } else if (strncmp(rx_buffer, "POST /clock ", 12) == 0) {
        char *body = strstr(rx_buffer, "\r\n\r\n");
        if (body) {
            body += 4; // Skip CRLFCRLF
            datetime_t t = {0};
            t.year  = (int16_t)get_json_int_value(body, "year", 2024);
            t.month = (int8_t)get_json_int_value(body, "month", 1);
            t.day   = (int8_t)get_json_int_value(body, "day", 1);
            t.hour  = (int8_t)get_json_int_value(body, "hour", 12);
            t.min   = (int8_t)get_json_int_value(body, "min", 0);
            t.sec   = (int8_t)get_json_int_value(body, "sec", 0);

            if (rtc_set_datetime(&t)) {
                http_send_response(pcb, "{\"status\": \"clock updated\"}", 200);
            } else {
                http_send_response(pcb, "{\"status\": \"invalid datetime\"}", 400);
            }
        }
    } else if (strncmp(rx_buffer, "GET /schedule ", 14) == 0) {
        schedule_item_t schedules[4];
        irrigator_get_all_schedules(schedules);
        
        char response[512];
        int offset = snprintf(response, sizeof(response), "[");
        for (int i = 0; i < 4; i++) {
            if (i > 0) offset += snprintf(response + offset, sizeof(response) - offset, ",");
            offset += snprintf(response + offset, sizeof(response) - offset, 
                "{\"index\":%d,\"hour\":%d,\"minute\":%d,\"duration\":%d,\"active\":%d}",
                i, schedules[i].hour, schedules[i].minute, schedules[i].duration, schedules[i].active);
        }
        snprintf(response + offset, sizeof(response) - offset, "]");
        
        http_send_response(pcb, response, 200);
    } else if (strncmp(rx_buffer, "GET /status ", 12) == 0) {
        char *response = malloc(RX_BUFFER_SIZE);
        if (response) {
            int offset = 0;
            
            datetime_t t;
            if (!clock_get_time(&t)) memset(&t, 0, sizeof(t));
            
            float temp, hum;
            aht10_get_latest_readings(&temp, &hum);
            
            schedule_item_t schedules[IRRIGATOR_MAX_SCHEDULE_SIZE];
            irrigator_get_all_schedules(schedules);

            offset += snprintf(response + offset, RX_BUFFER_SIZE - offset, 
                "{"
                "\"clock\":{\"synchronizedNTP\":%s,\"time\":{\"year\":%d,\"month\":%d,\"day\":%d,\"dotw\":%d,\"hour\":%d,\"min\":%d,\"sec\":%d}},"
                "\"irrigator\":{\"active\":%s,\"schedule\":[",
                is_ntp_synchronized() ? "true" : "false",
                t.year, t.month, t.day, t.dotw, t.hour, t.min, t.sec,
                irrigator_is_on() ? "true" : "false"
            );

            for (int i = 0; i < IRRIGATOR_MAX_SCHEDULE_SIZE; i++) {
                if (i > 0) offset += snprintf(response + offset, RX_BUFFER_SIZE - offset, ",");
                offset += snprintf(response + offset, RX_BUFFER_SIZE - offset, 
                    "{\"index\":%d,\"hour\":%d,\"minute\":%d,\"duration\":%d,\"active\":%d}",
                    i, schedules[i].hour, schedules[i].minute, schedules[i].duration, schedules[i].active);
            }

            offset += snprintf(response + offset, RX_BUFFER_SIZE - offset, 
                "]},"
                "\"sensors\":{\"temperature\":%.2f,\"humidity\":%.2f},"
                "\"wifi\":{\"hasInternetConnection\":%s}"
                "}",
                temp, hum,
                wifi_has_internet() ? "true" : "false"
            );

            http_send_response(pcb, response, 200);
            free(response);
        } else {
            http_send_response(pcb, "{\"error\": \"memory\"}", 500);
        }
    } else if (strncmp(rx_buffer, "GET /data ", 10) == 0) {
        char *response = malloc(RX_BUFFER_SIZE);
        if (response) {
            int offset = 0;
            
            datetime_t t;
            if (!clock_get_time(&t)) memset(&t, 0, sizeof(t));
            
            float temp, hum;
            aht10_get_latest_readings(&temp, &hum);
            
            schedule_item_t schedules[IRRIGATOR_MAX_SCHEDULE_SIZE];
            irrigator_get_all_schedules(schedules);

            struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
            char ip_str[16];
            strncpy(ip_str, ip4addr_ntoa(netif_ip4_addr(n)), sizeof(ip_str));
            ip_str[sizeof(ip_str)-1] = '\0';

            offset += snprintf(response + offset, RX_BUFFER_SIZE - offset, 
                "{\"board\":{\"model\":\"BitDogLab\",\"version\":\"v6.3\",\"description\":\"BitDogLab - EmbarcaTech\"},"
                "\"module\":{"
                "\"buttons\":{\"name\":\"Botões A/B\",\"description\":\"(A) Ligar, (B) Desligar (Prioritário).\"},"
                "\"buzzer\":{\"name\":\"Buzzer\",\"description\":\"Feedback sonoro.\"},"
                "\"clock\":{\"name\":\"RTC\",\"description\":\"Relógio interno (sincroniza via NTP).\",\"synchronizedNTP\":%s,\"time\":{\"year\":%d,\"month\":%d,\"day\":%d,\"dotw\":%d,\"hour\":%d,\"min\":%d,\"sec\":%d}},"
                "\"irrigator\":{\"name\":\"Irrigador\",\"description\":\"Relé 5V para válvula solenoide.\",\"active\":%s,\"schedule\":[",
                is_ntp_synchronized() ? "true" : "false",
                t.year, t.month, t.day, t.dotw, t.hour, t.min, t.sec,
                irrigator_is_on() ? "true" : "false"
            );

            for (int i = 0; i < IRRIGATOR_MAX_SCHEDULE_SIZE; i++) {
                if (i > 0) offset += snprintf(response + offset, RX_BUFFER_SIZE - offset, ",");
                offset += snprintf(response + offset, RX_BUFFER_SIZE - offset, 
                    "{\"index\":%d,\"hour\":%d,\"minute\":%d,\"duration\":%d,\"active\":%d}",
                    i, schedules[i].hour, schedules[i].minute, schedules[i].duration, schedules[i].active);
            }

            offset += snprintf(response + offset, RX_BUFFER_SIZE - offset, 
                "]},"
                "\"led\":{\"name\":\"LED\",\"description\":\"Indica irrigação ativa.\"},"
                "\"oled\":{\"name\":\"OLED\",\"description\":\"Display de status SSD1306.\"},"
                "\"humidityAndTemperature\":{\"name\":\"AHT10\",\"description\":\"Sensor de Temp/Hum.\",\"humidity\":%.2f,\"temperature\":%.2f},"
                "\"wifi\":{\"name\":\"Wi-Fi\",\"description\":\"Conexão sem fio.\",\"internet\":{\"connected\":%s},\"ip\":\"%s\"}"
                "},"
                "\"system\":{\"os\":\"FreeRTOS\",\"version\":\"v1.0.1\"}"
                "}",
                hum, temp,
                wifi_has_internet() ? "true" : "false",
                ip_str
            );

            http_send_response(pcb, response, 200);
            free(response);
        } else {
            http_send_response(pcb, "{\"error\": \"memory\"}", 500);
        }
    } else if (strncmp(rx_buffer, "POST /irrigator ", 16) == 0) {
        char *body = strstr(rx_buffer, "\r\n\r\n");
        if (body) {
            body += 4; // Skip CRLFCRLF
            bool active = get_json_bool_value(body, "active", false);
            
            if (active) {
                int duration = get_json_int_value(body, "duration", 60);
                if (duration > 360) duration = 360; // Max 6 min
                
                irrigator_set_remote_duration(duration);
                xTaskNotify(irrigator_task_handle, IRRIGATOR_REMOTE_TURN_ON, eSetValueWithOverwrite);
                http_send_response(pcb, "{\"status\": \"irrigator on\"}", 200);
            } else {
                xTaskNotify(irrigator_task_handle, IRRIGATOR_REMOTE_TURN_OFF, eSetValueWithOverwrite);
                http_send_response(pcb, "{\"status\": \"irrigator off\"}", 200);
            }
        } else {
            http_send_response(pcb, "{\"error\": \"no body\"}", 400);
        }
    } else if (strncmp(rx_buffer, "POST /schedule ", 15) == 0) {
        char *body = strstr(rx_buffer, "\r\n\r\n");
        if (body) {
            body += 4; // Skip CRLFCRLF
            int index = get_json_int_value(body, "index", -1);
            
            if (index >= 0 && index < IRRIGATOR_MAX_SCHEDULE_SIZE) {
                int hour = get_json_int_value(body, "hour", 0);
                int minute = get_json_int_value(body, "minute", 0);
                int duration = get_json_int_value(body, "duration", 60);
                int active = get_json_int_value(body, "active", 1);

                irrigator_set_schedule(index, (uint8_t)hour, (uint8_t)minute, (uint8_t)duration, (uint8_t)active);
                http_send_response(pcb, "{\"status\": \"schedule updated\"}", 200);
            } else {
                http_send_response(pcb, "{\"error\": \"invalid index\"}", 400);
            }
        } else {
            http_send_response(pcb, "{\"error\": \"no body\"}", 400);
        }
    } else {
        http_send_response(pcb, "{\"error\": \"not found\"}", 404);
    }

    free(rx_buffer);
    tcp_close(pcb);
    return ERR_OK;
}

static err_t http_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_recv_callback);
    return ERR_OK;
}

void api_local_task(void *pvParameters) {
    while (!wifi_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    cyw43_arch_lwip_begin();
    server_pcb = tcp_new();
    tcp_bind(server_pcb, IP_ADDR_ANY, 80);
    server_pcb = tcp_listen(server_pcb);
    tcp_accept(server_pcb, http_accept_callback);
    cyw43_arch_lwip_end();

    vTaskDelete(NULL);
}