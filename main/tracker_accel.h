#ifndef TRACKER_ACCEL_H
#define TRACKER_ACCEL_H

#include <driver/adc.h>
#include <math.h>
#include <esp_log.h>

#define ADXL_X_ADC_CHANNEL ADC1_CHANNEL_6 
#define ADXL_Y_ADC_CHANNEL ADC1_CHANNEL_7 
#define ADXL_Z_ADC_CHANNEL ADC1_CHANNEL_4 

// INCREASED THRESHOLD to ignore 0.2V noise spikes seen in logs
#define MOVEMENT_THRESHOLD 0.50f 

static const char* ACCEL_TAG = "ACCEL";

esp_err_t accel_init() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADXL_X_ADC_CHANNEL, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADXL_Y_ADC_CHANNEL, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(ADXL_Z_ADC_CHANNEL, ADC_ATTEN_DB_11);
    return ESP_OK;
}

bool check_movement() {
    static float lastX = 0, lastY = 0, lastZ = 0;
    float sumX = 0, sumY = 0, sumZ = 0;
    const int SAMPLES = 10;

    for (int i = 0; i < SAMPLES; i++) {
        sumX += (adc1_get_raw(ADXL_X_ADC_CHANNEL) * 3.3f) / 4095.0f;
        sumY += (adc1_get_raw(ADXL_Y_ADC_CHANNEL) * 3.3f) / 4095.0f;
        sumZ += (adc1_get_raw(ADXL_Z_ADC_CHANNEL) * 3.3f) / 4095.0f;
    }

    float currX = sumX / SAMPLES;
    float currY = sumY / SAMPLES;
    float currZ = sumZ / SAMPLES;

    // FILTER: Ignore impossible low-voltage drops (Loose wire/Noise)
    if (currX < 0.8f || currY < 0.8f || currZ < 0.8f) {
        return false; 
    }

    float delta = fabsf(currX - lastX) + fabsf(currY - lastY) + fabsf(currZ - lastZ);
    
    lastX = currX; lastY = currY; lastZ = currZ;

    if (delta > MOVEMENT_THRESHOLD) {
        ESP_LOGI(ACCEL_TAG, "Movement! Delta: %.3f", delta);
        return true;
    }
    return false;
}
#endif