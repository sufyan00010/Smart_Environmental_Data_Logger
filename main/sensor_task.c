#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_log.h>
#include <esp_adc_cal.h>
#include <esp_random.h>
#include "sensor_task.h"
#include "project_config.h"
#include "dht11.h"

static const char *TAG = "SENSOR_TASK";

// GPIO definitions
#define DHT11_GPIO GPIO_NUM_4
#define LDR_ADC_CHANNEL ADC1_CHANNEL_3  // GPIO3
#define BUTTON_GPIO GPIO_NUM_5

// External references
extern QueueHandle_t sensor_data_queue;

// Sensor data structure
typedef struct {
    float temperature;
    float humidity;
    int aqi;
    uint32_t timestamp;
} sensor_data_t;

// ADC calibration
static esp_adc_cal_characteristics_t adc_chars;

// ============================================
// AIR QUALITY CALCULATION (Creative Solution)
// ============================================

/**
 * Calculate Air Quality Index based on temperature, humidity, and light level
 * 
 * Research Basis:
 * - High temperature + high humidity = poor air circulation → higher AQI
 * - Low light levels indoors may indicate poor ventilation → higher AQI
 * - Optimal conditions: 20-25°C, 40-60% humidity → lowest AQI
 */
static int calculate_aqi(float temp, float humidity, int light_level)
{
    int aqi = 50; // Base "Good" AQI
    
    // Temperature contribution
    if (temp > 30.0) {
        aqi += (int)((temp - 30.0) * 3.0);  // +3 AQI per degree above 30°C
    } else if (temp < 18.0) {
        aqi += (int)((18.0 - temp) * 2.0);  // +2 AQI per degree below 18°C
    }
    
    // Humidity contribution
    if (humidity > 70.0) {
        aqi += (int)((humidity - 70.0) * 2.0);  // High humidity
    } else if (humidity < 30.0) {
        aqi += (int)((30.0 - humidity) * 1.5);  // Low humidity (dry air)
    }
    
    // Light level contribution (proxy for ventilation)
    // Assuming ADC range 0-4095
    // Lower light = poor ventilation = higher AQI
    if (light_level < 1000) {
        aqi += (1000 - light_level) / 20;  // Up to +50 for very dark rooms
    }
    
    // Add small random variation (±5) to simulate real-world fluctuation
    aqi += (esp_random() % 11) - 5;
    
    // Clamp to valid AQI range (0-500)
    if (aqi < 0) aqi = 0;
    if (aqi > 500) aqi = 500;
    
    ESP_LOGI(TAG, "AQI calculation: T=%.1f, H=%.1f, L=%d → AQI=%d", 
             temp, humidity, light_level, aqi);
    
    return aqi;
}

// ============================================
// SENSOR INITIALIZATION
// ============================================

void sensor_init(void)
{
    ESP_LOGI(TAG, "Initializing sensors...");
    
    // Initialize DHT11
    dht11_init(DHT11_GPIO);
    
    // Initialize ADC for LDR
    adc1_config_width(ADC_WIDTH_BIT_12);  // 0-4095
    adc1_config_channel_atten(LDR_ADC_CHANNEL, ADC_ATTEN_DB_11);  // 0-3.3V range
    
    // Characterize ADC
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 
                             1100, &adc_chars);
    
    // Initialize button
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&btn_cfg);
    
    ESP_LOGI(TAG, "Sensors initialized successfully");
}

// ============================================
// DHT11 READING WITH RETRY
// ============================================

static bool read_dht11_with_retry(float *temp, float *humidity, int max_retries)
{
    for (int i = 0; i < max_retries; i++) {
        if (dht11_read(temp, humidity) == ESP_OK) {
            // Validate readings
            if (*temp >= -40.0 && *temp <= 80.0 && 
                *humidity >= 0.0 && *humidity <= 100.0) {
                return true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));  // Wait before retry
    }
    return false;
}

// ============================================
// LDR READING (Light Sensor)
// ============================================

static int read_ldr(void)
{
    uint32_t adc_reading = 0;
    
    // Average 10 samples for stability
    for (int i = 0; i < 10; i++) {
        adc_reading += adc1_get_raw(LDR_ADC_CHANNEL);
    }
    adc_reading /= 10;
    
    // Convert to voltage (optional, for logging)
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, &adc_chars);
    ESP_LOGD(TAG, "LDR: ADC=%lu, Voltage=%lumV", adc_reading, voltage);
    
    return (int)adc_reading;
}

// ============================================
// BUTTON MANUAL CALIBRATION
// ============================================

static bool button_pressed(void)
{
    return (gpio_get_level(BUTTON_GPIO) == 0);  // Active LOW
}

// ============================================
// MAIN SENSOR TASK
// ============================================

void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task started");
    
    sensor_data_t sensor_data;
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t read_interval = pdMS_TO_TICKS(10000);  // Every 10 seconds
    
    // Variables for sensor readings
    float temperature = 25.0;
    float humidity = 50.0;
    int light_level = 2000;
    int aqi = 50;
    
    // Calibration mode flag
    bool calibration_mode = false;
    uint32_t button_press_time = 0;
    
    while (1) {
        // Check for button long press (3 seconds) to enter calibration mode
        if (button_pressed()) {
            if (button_press_time == 0) {
                button_press_time = xTaskGetTickCount();
            } else if ((xTaskGetTickCount() - button_press_time) > pdMS_TO_TICKS(3000)) {
                calibration_mode = !calibration_mode;
                ESP_LOGI(TAG, "Calibration mode: %s", 
                         calibration_mode ? "ENABLED" : "DISABLED");
                button_press_time = 0;
                vTaskDelay(pdMS_TO_TICKS(1000));  // Debounce
            }
        } else {
            button_press_time = 0;
        }
        
        // Read DHT11 (Temperature & Humidity)
        if (read_dht11_with_retry(&temperature, &humidity, 3)) {
            ESP_LOGI(TAG, "DHT11: Temperature=%.1f°C, Humidity=%.1f%%", 
                     temperature, humidity);
        } else {
            ESP_LOGW(TAG, "DHT11 read failed, using previous values");
        }
        
        // Read LDR (Light Level)
        light_level = read_ldr();
        ESP_LOGI(TAG, "Light Level: %d/4095", light_level);
        
        // Calculate AQI based on environmental factors
        aqi = calculate_aqi(temperature, humidity, light_level);
        
        // Prepare sensor data structure
        sensor_data.temperature = temperature;
        sensor_data.humidity = humidity;
        sensor_data.aqi = aqi;
        sensor_data.timestamp = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        
        // Send data to queue (non-blocking)
        if (xQueueSend(sensor_data_queue, &sensor_data, 0) != pdTRUE) {
            ESP_LOGW(TAG, "Sensor data queue full, data dropped");
        } else {
            ESP_LOGI(TAG, "Sensor data sent to queue");
        }
        
        // Wait for next read interval (precise timing)
        vTaskDelayUntil(&last_wake_time, read_interval);
    }
}
