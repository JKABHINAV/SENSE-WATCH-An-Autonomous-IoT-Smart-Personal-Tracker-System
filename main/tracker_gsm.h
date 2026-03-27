#ifndef TRACKER_GSM_H
#define TRACKER_GSM_H

#include <driver/uart.h>
#include <esp_err.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// UART Configuration for GSM (using UART1 to leave UART0 for console)
#define GSM_UART_NUM UART_NUM_1
#define GSM_TX_PIN 16
#define GSM_RX_PIN 17
#define GSM_BUF_SIZE (256)

// Configuration Constants
const char* targetNumber = "+919611337334"; // IMPORTANT: Use correct formatting with country code.

// Initialize GSM UART
esp_err_t gsm_init() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    esp_err_t err = uart_driver_install(GSM_UART_NUM, GSM_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) return err;

    err = uart_param_config(GSM_UART_NUM, &uart_config);
    if (err != ESP_OK) return err;

    err = uart_set_pin(GSM_UART_NUM, GSM_TX_PIN, GSM_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    ESP_LOGI("GSM", "GSM900A (UART1) Initialized ✅ Pins RX:%d, TX:%d",
             GSM_TX_PIN, GSM_RX_PIN);
    return ESP_OK;
}

// Function to send a standardized AT command sequence to send SMS
void gsm_send_sms(const char* message) {
    char cmd[128];
    ESP_LOGI("GSM", "Sending SMS sequence...");

    // Basic sequence: AT -> Check responsiveness, CMGF=1 -> Set SMS format to text, CMGS=<num> -> Specify recipient
    uart_write_bytes(GSM_UART_NUM, "AT\r\n", 4);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    // Simplified error check logic omitted for clarity/performance - assuming module responsive
    uart_write_bytes(GSM_UART_NUM, "AT+CMGF=1\r\n", 11);
    vTaskDelay(200 / portTICK_PERIOD_MS);

    snprintf(cmd, 127, "AT+CMGS=\"%s\"\r\n", targetNumber);
    uart_write_bytes(GSM_UART_NUM, cmd, strlen(cmd));
    vTaskDelay(500 / portTICK_PERIOD_MS); // Crucial delay before message text

    uart_write_bytes(GSM_UART_NUM, message, strlen(message));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    uint8_t ctrlZ = 0x1A; // Finalize and send character
    uart_write_bytes(GSM_UART_NUM, &ctrlZ, 1);

    ESP_LOGI("GSM", "AT command sequence sent. Waiting for network confirmation...");
    vTaskDelay(10000 / portTICK_PERIOD_MS); // Long delay for SMS transmission
    ESP_LOGI("GSM", "SMS Processed ✅ (Assuming sent successfully by module)");
}

#endif // TRACKER_GSM_H