#ifndef TRACKER_GPS_H
#define TRACKER_GPS_H

#include <driver/uart.h>
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

// UART Configuration - Use UART2 to avoid conflicts with console
#define GPS_UART_NUM UART_NUM_2
#define GPS_TX_PIN 12 
#define GPS_RX_PIN 14 
#define GPS_BUF_SIZE (1024)

// Global state variables - These provide data to your Cloud and SMS logic
static char last_lat[16] = "0.0";
static char last_lon[16] = "0.0";
static bool gps_fixed = false;

// Initialize UART for GPS
esp_err_t gps_init() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    
    esp_err_t err = uart_driver_install(GPS_UART_NUM, GPS_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) return err;
    
    err = uart_param_config(GPS_UART_NUM, &uart_config);
    if (err != ESP_OK) return err;
    
    err = uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    ESP_LOGI("GPS", "SIM28 GPS Initialized ✅ (Pins RX:%d, TX:%d)", GPS_TX_PIN, GPS_RX_PIN);
    return ESP_OK;
}

// Function to read and parse NMEA $GPGGA data.
void process_gps_data() {
    uint8_t data[GPS_BUF_SIZE];
    int length = uart_read_bytes(GPS_UART_NUM, data, GPS_BUF_SIZE, 20 / portTICK_PERIOD_MS);

    if (length > 0) {
        data[length] = '\0';
        char* token = strtok((char*)data, "\n"); 

        while (token != NULL) {
            if (strstr(token, "$GPGGA") != NULL) {
                // Hardened Parser for 15 GPGGA fields
                char ggga_fields[15][32]; 
                int field_idx = 0;
                char temp_line[128];
                
                strncpy(temp_line, token, sizeof(temp_line) - 1);
                temp_line[sizeof(temp_line) - 1] = '\0';

                char* field_start = temp_line;
                char* comma;

                // Split CSV fields safely
                while (field_idx < 15) {
                    comma = strchr(field_start, ',');
                    if (comma == NULL) {
                        strncpy(ggga_fields[field_idx], field_start, sizeof(ggga_fields[field_idx]) - 1);
                        ggga_fields[field_idx][sizeof(ggga_fields[field_idx]) - 1] = '\0';
                        field_idx++;
                        break;
                    }

                    size_t field_len = comma - field_start;
                    if (field_len > 31) field_len = 31;

                    memcpy(ggga_fields[field_idx], field_start, field_len);
                    ggga_fields[field_idx][field_len] = '\0';

                    field_idx++;
                    field_start = comma + 1;
                }

                // Check for valid fix (Field 6: 0=Invalid, 1=GPS fix, 2=DGPS fix)
                if (field_idx > 6 && ggga_fields[6][0] != '0') {
                    gps_fixed = true;

                    char* latRaw = ggga_fields[2]; // Format: DDMM.MMMM
                    char* latDir = ggga_fields[3]; // N or S
                    char* lonRaw = ggga_fields[4]; // Format: DDDMM.MMMM
                    char* lonDir = ggga_fields[5]; // E or W

                    if (strlen(latRaw) > 4 && strlen(lonRaw) > 4) {
                        double lat, lon;
                        char degs[4];

                        // Convert Latitude to Decimal Degrees
                        memset(degs, 0, sizeof(degs));
                        strncpy(degs, latRaw, 2);
                        lat = atof(degs) + (atof(latRaw + 2) / 60.0);
                        if (latDir[0] == 'S') lat = -lat;
                        snprintf(last_lat, sizeof(last_lat), "%.6f", lat);

                        // Convert Longitude to Decimal Degrees
                        memset(degs, 0, sizeof(degs));
                        strncpy(degs, lonRaw, 3);
                        lon = atof(degs) + (atof(lonRaw + 3) / 60.0);
                        if (lonDir[0] == 'W') lon = -lon;
                        snprintf(last_lon, sizeof(last_lon), "%.6f", lon);

                        ESP_LOGI("GPS", "Updated Fix ✅: %s, %s", last_lat, last_lon);
                    }
                } else {
                    gps_fixed = false; 
                }
            }
            token = strtok(NULL, "\n"); 
            if (token != NULL && token[0] != '$') break;
        }
    }
}

#endif // TRACKER_GPS_H