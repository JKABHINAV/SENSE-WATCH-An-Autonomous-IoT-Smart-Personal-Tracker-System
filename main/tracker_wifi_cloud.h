#ifndef TRACKER_WIFI_CLOUD_H
#define TRACKER_WIFI_CLOUD_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_http_client.h>
#include <esp_sntp.h>
#include <nvs_flash.h>
#include <string.h>
#include <time.h>

// Configuration Constants
#define WIFI_SSID "YOUR_WIFI_SSID_HERE"
#define WIFI_PASS "YOUR_WIFI_PASSWORD_HERE"

// Updated Deployment URL
#define CLOUD_URL "YOUR_GOOGLE_SCRIPT_URL_HERE"
#define NTP_SERVER "time.google.com"

static bool wifi_connected = false;

// Basic WiFi State Machine Logic
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI("WIFI", "Disconnected. Retrying...");
        esp_wifi_connect();
        wifi_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WIFI", "WiFi Connected ✅ IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}

// Initialize NVS and WiFi in Station Mode
esp_err_t wifi_init_sta() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) return ret;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("WIFI", "WiFi (Station) Initialized ✅");
    return ESP_OK;
}

// 1. RECTIFIED TIME SYNC: Proper IST and Formatting
void sntp_time_sync() {
    ESP_LOGI("TIME", "Initializing SNTP for IST (India)...");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, NTP_SERVER);
    sntp_init();

    // Set Timezone to India (IST)
    setenv("TZ", "IST-5:30", 1);
    tzset();

    int retry = 0;
    const int retry_count = 15;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry <= retry_count) {
        ESP_LOGI("TIME", "Waiting for sync... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (retry <= retry_count) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        char buf[64];
        strftime(buf, sizeof(buf), "%d-%m-%Y %H:%M:%S", &timeinfo);
        ESP_LOGI("TIME", "Time Synchronized ✅: %s", buf);
    } else {
        ESP_LOGE("TIME", "NTP Sync Failed. Logs may have wrong timestamps.");
    }
}

// 2. RECTIFIED CLOUD SYNC: SSL Bypass and URL Formatting
esp_err_t send_to_cloud(const char* status, const char* lat, const char* lon) {
    if (!wifi_connected || lat[0] == '0' || strcmp(lat, "0.000000") == 0) {
        ESP_LOGW("CLOUD", "Skipping Cloud Sync: No GPS Fix 🛰");
        return ESP_FAIL;
    }

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    char date_buf[16], time_buf[16];
    strftime(date_buf, sizeof(date_buf), "%d-%m-%Y", &timeinfo);
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &timeinfo);

    // Create a clickable Google Maps Link
    char maps_link[128];
    snprintf(maps_link, sizeof(maps_link), "https://www.google.com/maps?q=%s,%s", lat, lon);

    char full_url[1024];
    // We send 'maps_link' to the 'link' parameter in your Google Script
    snprintf(full_url, sizeof(full_url),
             "%s?date=%s&time=%s&lat=%s&lon=%s&status=%s&link=%s",
             CLOUD_URL, date_buf, time_buf, lat, lon, status, maps_link);

    ESP_LOGI("CLOUD", "Sending Data... Lat:%s, Lon:%s", lat, lon);

    esp_http_client_config_t config = {
        .url = full_url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
        .skip_cert_common_name_check = true,
        .crt_bundle_attach = NULL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int code = esp_http_client_get_status_code(client);
        if (code == 200 || code == 302) {
            ESP_LOGI("CLOUD", "Stored [%s] ✅", status);
        }
    } else {
        ESP_LOGE("CLOUD", "Failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}

#endif // TRACKER_WIFI_CLOUD_H
