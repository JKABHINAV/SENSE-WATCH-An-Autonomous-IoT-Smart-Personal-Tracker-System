#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_timer.h>

// Include specific tracker logic files
#include "tracker_wifi_cloud.h"
#include "tracker_gps.h"
#include "tracker_gsm.h"
#include "tracker_accel.h"

// Configuration Constants
#define SOS_PIN GPIO_NUM_13
#define TAG "TRACKER_SYSTEM"

// RECTIFIED TIMINGS
const int QUIET_TIME_SEC = 60;
const int HEARTBEAT_INTERVAL_SEC = 120; // Forced update every 2 minutes
const int SYSTEM_LOOP_DELAY_MS = 100;

// State Tracking
static uint64_t lastMovementTime_us = 0;
static uint64_t lastCloudTime_us = 0;
static uint64_t lastStatusPrint_us = 0;
static bool inactivityReported = false;

void tracker_main_task(void *pvParameters) {
    ESP_LOGI(TAG, "==== SMART PERSONAL TRACKER STARTED (IDF Port) ====");

    // 1. System Initializations
    ESP_ERROR_CHECK(wifi_init_sta());
    sntp_time_sync();
    ESP_ERROR_CHECK(accel_init());
    ESP_ERROR_CHECK(gps_init());
    ESP_ERROR_CHECK(gsm_init());

    // 2. SOS Pin Setup
    gpio_config_t sos_io_config = {
        .pin_bit_mask = (1ULL << SOS_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&sos_io_config);

    ESP_LOGI(TAG, "System ACTIVE, Monitoring... ✅");

    uint64_t now_us = esp_timer_get_time();
    lastMovementTime_us = now_us;
    lastCloudTime_us = now_us;
    char smsMessage[256];

    while (1) {
        // --- A. Update Peripherals ---
        process_gps_data();
        bool moving = check_movement();
        now_us = esp_timer_get_time();

        // --- B. TERMINAL MONITORING (Every 5 seconds) ---
        if ((now_us - lastStatusPrint_us) > 5000000) {
            ESP_LOGI("SYSTEM_CHECK", "GPS: %s | Lat: %s, Lon: %s | WiFi: %s", 
                     gps_fixed ? "FIXED ✅" : "SEARCHING 🛰", 
                     last_lat, last_lon,
                     wifi_connected ? "CONNECTED" : "DISCONNECTED");
            lastStatusPrint_us = now_us;
        }

        // --- C. Movement & Inactivity Logic ---
        if (moving) {
            lastMovementTime_us = now_us;
            if (inactivityReported) {
                ESP_LOGI(TAG, "STATUS: ACTIVITY DETECTED ⚡ (System Resumed)");
                send_to_cloud("ACTIVE", last_lat, last_lon);
                inactivityReported = false;
            }
        }

        uint64_t current_inactive_sec = (now_us - lastMovementTime_us) / 1000000;

        // --- D. TRIGGER: INACTIVITY (After 60s of stillness) ---
        if (!moving && (current_inactive_sec >= QUIET_TIME_SEC) && !inactivityReported) {
            ESP_LOGW(TAG, "⚠ INACTIVITY ALERT TRIGGERED!");
            
            const char* log_lat = gps_fixed ? last_lat : "0.0";
            const char* log_lon = gps_fixed ? last_lon : "0.0";

            snprintf(smsMessage, sizeof(smsMessage),
                     "ALERT! No Movement.\nLocation: %s,%s\nMaps: http://maps.google.com/maps?q=%s,%s",
                     log_lat, log_lon, log_lat, log_lon);
            
            gsm_send_sms(smsMessage);
            send_to_cloud("INACTIVITY", log_lat, log_lon);
            inactivityReported = true;
        }

        // --- E. TRIGGER: SOS (Immediate & Independent of GPS Fix) ---
        if (gpio_get_level(SOS_PIN) == 0) {
            ESP_LOGE(TAG, "🚨 EMERGENCY SOS PRESSED!");
            
            const char* log_lat = gps_fixed ? last_lat : "0.0";
            const char* log_lon = gps_fixed ? last_lon : "0.0";

            snprintf(smsMessage, sizeof(smsMessage),
                     "🚨 EMERGENCY SOS\nUser Alert!\nLocation: %s,%s\nMaps: http://maps.google.com/maps?q=%s,%s",
                     log_lat, log_lon, log_lat, log_lon);
            
            gsm_send_sms(smsMessage); 
            send_to_cloud("SOS", log_lat, log_lon);
            
            vTaskDelay(5000 / portTICK_PERIOD_MS); // Prevent multi-triggering
        }

        // --- F. TRIGGER: FORCED 2-MINUTE HEARTBEAT (Update Excel Regardless of State) ---
        uint64_t heartbeat_dur_sec = (now_us - lastCloudTime_us) / 1000000;
        if (heartbeat_dur_sec >= HEARTBEAT_INTERVAL_SEC) {
            const char* stat = inactivityReported ? "STILL" : "MOVING";
            const char* log_lat = gps_fixed ? last_lat : "0.0";
            const char* log_lon = gps_fixed ? last_lon : "0.0";

            ESP_LOGI(TAG, "⏰ 2-Min Update: Sending %s to cloud...", stat);
            
            if (send_to_cloud(stat, log_lat, log_lon) == ESP_OK) {
                lastCloudTime_us = esp_timer_get_time();
            }
        }

        vTaskDelay(SYSTEM_LOOP_DELAY_MS / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting Tracker Main Task...");
    xTaskCreate(tracker_main_task, "tracker_task", 4096 * 4, NULL, 5, NULL);
}