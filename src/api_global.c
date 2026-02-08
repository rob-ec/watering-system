/**
 * @file api_global.c
 * @brief Implementation of global API.
 *
 * @author Robson Gomes
 */

#include "api_global.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/api.h"
#include "pico/cyw43_arch.h"
#include "aht10.h"
#include "wifi_connection.h"
#include "irrigator.h"
#include "clock.h"
#include "hardware/rtc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define RECV_BUFFER_SIZE 4096
#define REQUEST_BUFFER_SIZE 2048

static char barear_token[512] = {0};
static char api_host[64] = {0};
static char api_base_path[64] = {0};
static struct tcp_pcb *client_pcb;
static ip_addr_t server_ip;
static TaskHandle_t task_handle;

// Buffer to store the response
static char response_buffer[RECV_BUFFER_SIZE];
static int response_pos = 0;

// Request parameters
static const char *current_method;
static const char *current_path;
static const char *current_body;
static bool is_login_request = false;

// Helper: Extract JSON value (Simplified version from api_local.c)
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
    quote_start++; 

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
    while (*val_start && (*val_start == ' ' || *val_start == '\t' || *val_start == '\r' || *val_start == '\n')) val_start++;
    
    if (*val_start == '\"') val_start++;
    
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
    while (*val_start && (*val_start == ' ' || *val_start == '\t' || *val_start == '\r' || *val_start == '\n' || *val_start == '\"')) val_start++;
    
    if (strncmp(val_start, "true", 4) == 0) return true;
    if (strncmp(val_start, "false", 5) == 0) return false;
    if (strncmp(val_start, "1", 1) == 0) return true;
    if (strncmp(val_start, "0", 1) == 0) return false;
    
    return default_val;
}

static void parse_schedules(const char *json) {
    const char *schedules_key = "\"schedules\"";
    const char *p = strstr(json, schedules_key);
    if (!p) return;
    
    p = strchr(p, '[');
    if (!p) return;
    p++; 

    while (*p) {
        const char *obj_start = strchr(p, '{');
        if (!obj_start) break;
        
        const char *obj_end = strchr(obj_start, '}');
        if (!obj_end) break;

        const char *array_end = strchr(p, ']');
        if (array_end && array_end < obj_start) break;

        int len = obj_end - obj_start + 1;
        if (len >= 256) len = 255;
        char obj_buf[256];
        strncpy(obj_buf, obj_start, len);
        obj_buf[len] = '\0';

        int index = get_json_int_value(obj_buf, "index", -1);

        if (index >= 0 && index < IRRIGATOR_MAX_SCHEDULE_SIZE) {
            int hour = get_json_int_value(obj_buf, "hour", 0);
            int minute = get_json_int_value(obj_buf, "minute", 0);
            int duration = get_json_int_value(obj_buf, "duration", 60);
            bool active = get_json_bool_value(obj_buf, "active", false);

            irrigator_set_schedule(index, (uint8_t)hour, (uint8_t)minute, (uint8_t)duration, (uint8_t)(active ? 1 : 0));
            printf("API Global: Synced schedule %d: %02d:%02d dur=%d act=%d\n", index, hour, minute, duration, active);
        }

        p = obj_end + 1;
    }
}

static void close_connection(struct tcp_pcb *pcb) {
    if (pcb) {
        tcp_arg(pcb, NULL);
        tcp_sent(pcb, NULL);
        tcp_recv(pcb, NULL);
        tcp_err(pcb, NULL);
        tcp_close(pcb);
    }
}

static void parse_url_if_needed(void) {
    if (api_host[0] != '\0') return;

    const char *url = API_GLOBAL_URL;
    const char *p = strstr(url, "://");
    if (p) p += 3;
    else p = url;

    const char *slash = strchr(p, '/');
    if (slash) {
        size_t host_len = slash - p;
        if (host_len >= sizeof(api_host)) host_len = sizeof(api_host) - 1;
        strncpy(api_host, p, host_len);
        api_host[host_len] = '\0';
        strncpy(api_base_path, slash, sizeof(api_base_path) - 1);
    } else {
        strncpy(api_host, p, sizeof(api_host) - 1);
        api_base_path[0] = '\0';
    }
    
    char *colon = strchr(api_host, ':');
    if (colon) *colon = '\0';
}

static err_t http_client_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (!p) {
        // Connection closed by server
        close_connection(pcb);
        
        // Process response
        response_buffer[response_pos] = '\0';
        
        // Check for 401 Unauthorized
        if (strstr(response_buffer, "HTTP/1.1 401") || strstr(response_buffer, "HTTP/1.0 401")) {
            printf("API Global: 401 Unauthorized. Clearing token.\n");
            memset(barear_token, 0, sizeof(barear_token));
        } else if (is_login_request) {
            // Try to extract token from body
            char *body = strstr(response_buffer, "\r\n\r\n");
            if (body) {
                body += 4;
                printf("API Global: Login Response Body: %s\n", body);

                char new_token[512] = {0};
                get_json_value(body, "token", new_token, sizeof(new_token));
                if (strlen(new_token) > 0) {
                    strncpy(barear_token, new_token, sizeof(barear_token) - 1);
                    printf("API Global: Login successful. Token acquired.\n");
                } else {
                    printf("API Global: Login failed. No token in response.\n");
                }
            }
        } else if (strstr(current_path, "/device/sync")) {
             char *body = strstr(response_buffer, "\r\n\r\n");
             if (body) {
                 body += 4;
                 parse_schedules(body);
             }
        } else {
            // Telemetry response
            if (strstr(response_buffer, "200 OK")) {
                printf("API Global: Telemetry sent successfully.\n");
            }
        }

        xTaskNotifyGive(task_handle);
        return ERR_OK;
    }

    if (response_pos + p->tot_len < RECV_BUFFER_SIZE - 1) {
        pbuf_copy_partial(p, response_buffer + response_pos, p->tot_len, 0);
        response_pos += p->tot_len;
    }
    
    tcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static err_t http_client_connected(void *arg, struct tcp_pcb *pcb, err_t err) {
    if (err != ERR_OK) {
        printf("API Global: Connection failed %d\n", err);
        close_connection(pcb);
        xTaskNotifyGive(task_handle);
        return err;
    }

    char *request = malloc(REQUEST_BUFFER_SIZE);
    if (!request) {
        close_connection(pcb);
        xTaskNotifyGive(task_handle);
        return ERR_MEM;
    }

    int len;
    if (is_login_request) {
        len = snprintf(request, REQUEST_BUFFER_SIZE,
            "%s %s%s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            current_method, api_base_path, current_path, api_host, API_PORT, (int)strlen(current_body), current_body);
    } else {
        len = snprintf(request, REQUEST_BUFFER_SIZE,
            "%s %s%s HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "Authorization: Bearer %s\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            current_method, api_base_path, current_path, api_host, API_PORT, barear_token, (int)strlen(current_body), current_body);
    }

    tcp_write(pcb, request, len, TCP_WRITE_FLAG_COPY);
    tcp_output(pcb);
    free(request);

    return ERR_OK;
}

static void http_client_err(void *arg, err_t err) {
    printf("API Global: TCP Error %d\n", err);
    xTaskNotifyGive(task_handle);
}

static void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr) {
        server_ip = *ipaddr;
        
        client_pcb = tcp_new();
        if (client_pcb) {
            tcp_arg(client_pcb, NULL);
            tcp_recv(client_pcb, http_client_recv);
            tcp_err(client_pcb, http_client_err);
            tcp_connect(client_pcb, &server_ip, API_PORT, http_client_connected);
        } else {
            xTaskNotifyGive(task_handle);
        }
    } else {
        printf("API Global: DNS Failed\n");
        xTaskNotifyGive(task_handle);
    }
}

static void perform_request(const char *method, const char *path, const char *body, bool is_login) {
    current_method = method;
    current_path = path;
    current_body = body;
    is_login_request = is_login;
    response_pos = 0;

    parse_url_if_needed();

    cyw43_arch_lwip_begin();
    
    int err = dns_gethostbyname(api_host, &server_ip, dns_found, NULL);
    
    if (err == ERR_OK) {
        // IP already cached
        dns_found(api_host, &server_ip, NULL);
    } else if (err != ERR_INPROGRESS) {
        printf("API Global: DNS Error %d\n", err);
        xTaskNotifyGive(task_handle); // Unlock task immediately on error
    }
    
    cyw43_arch_lwip_end();

    // Wait for request to complete (timeout 10s)
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));
}

static void generate_telemetry_json(char *buffer, size_t size) {
    datetime_t t;
    if (!clock_get_time(&t)) memset(&t, 0, sizeof(t));
    
    float temp, hum;
    aht10_get_latest_readings(&temp, &hum);
    
    schedule_item_t schedules[IRRIGATOR_MAX_SCHEDULE_SIZE];
    irrigator_get_all_schedules(schedules);

    int offset = 0;
    offset += snprintf(buffer + offset, size - offset, 
        "{"
        "\"clock\":{\"synchronizedNTP\":%s,\"time\":{\"year\":%d,\"month\":%d,\"day\":%d,\"dotw\":%d,\"hour\":%d,\"min\":%d,\"sec\":%d}},"
        "\"irrigator\":{\"active\":%s,\"schedule\":[",
        is_ntp_synchronized() ? "true" : "false",
        t.year, t.month, t.day, t.dotw, t.hour, t.min, t.sec,
        irrigator_is_on() ? "true" : "false"
    );

    for (int i = 0; i < IRRIGATOR_MAX_SCHEDULE_SIZE; i++) {
        if (i > 0) offset += snprintf(buffer + offset, size - offset, ",");
        offset += snprintf(buffer + offset, size - offset, 
            "{\"index\":%d,\"hour\":%d,\"minute\":%d,\"duration\":%d,\"active\":%d}",
            i, schedules[i].hour, schedules[i].minute, schedules[i].duration, schedules[i].active);
    }

    offset += snprintf(buffer + offset, size - offset, 
        "]},"
        "\"sensors\":{\"temperature\":%.2f,\"humidity\":%.2f},"
        "\"wifi\":{\"hasInternetConnection\":%s}"
        "}",
        temp, hum,
        wifi_has_internet() ? "true" : "false"
    );
}

void api_global_task(void *pvParameters) {
    task_handle = xTaskGetCurrentTaskHandle();
    char *payload_buffer = malloc(2048);

    if (!payload_buffer) {
        printf("API Global: Failed to allocate payload buffer\n");
        vTaskDelete(NULL);
    }

    while (1) {
        if (wifi_has_internet()) {
            
            // 1. Login if needed
            if (strlen(barear_token) == 0) {
                printf("API Global: Authenticating...\n");
                snprintf(payload_buffer, 2048, 
                    "{\"serial_number\": \"%s\", \"secret_token\": \"%s\"}", 
                    API_CONNECTION_SERIAL_NUMBER, API_CONNECTION_SECRET_TOKEN);
                
                perform_request("POST", "/device/login", payload_buffer, true);
                
                if (strlen(barear_token) == 0) {
                    // Login failed, wait before retry
                    vTaskDelay(pdMS_TO_TICKS(10000));
                    continue;
                }
            }

            // 2. Sync Schedules
            if (strlen(barear_token) > 0) {
                printf("API Global: Syncing schedules...\n");
                perform_request("GET", "/device/sync", "", false);
            }

            // 3. Send Telemetry
            printf("API Global: Sending telemetry...\n");
            generate_telemetry_json(payload_buffer, 2048);
            perform_request("POST", "/telemetry", payload_buffer, false);

            // Wait 60 seconds before next telemetry
            vTaskDelay(pdMS_TO_TICKS(60000));

        } else {
            // Wait for internet
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
    
    free(payload_buffer);
}
