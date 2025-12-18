/**
 * @file display_task.c
 * @brief OLED display task implementation
 */

#include "display_task.h"
#include "sensor_task.h"
#include "project_config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <string.h>
#include <stdio.h>
#include "ssd1306.h"

static const char *TAG = "DISPLAY_TASK";

// External references
extern QueueHandle_t sensor_data_queue;
extern EventGroupHandle_t system_events;

#define WIFI_CONNECTED_BIT BIT0
#define CLOUD_CONNECTED_BIT BIT1

// Display state
static ssd1306_handle_t display_handle = NULL;
static bool display_initialized = false;

void display_init(void)
{
    ESP_LOGI(TAG, "Initializing OLED display...");
    
    // Create SSD1306 device
    display_handle = ssd1306_create(I2C_MASTER_NUM, SSD1306_I2C_ADDRESS);
    if (display_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create SSD1306 handle");
        return;
    }
    
    // Initialize display
    esp_err_t err = ssd1306_init(display_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SSD1306: %s", esp_err_to_name(err));
        return;
    }
    
    // Clear display
    ssd1306_clear_screen(display_handle, 0x00);
    ssd1306_refresh_gram(display_handle);
    
    // Display startup message
    ssd1306_draw_string(display_handle, 0, 0, (const uint8_t *)"Smart Env Logger", 16, 1);
    ssd1306_draw_string(display_handle, 0, 16, (const uint8_t *)"Initializing...", 16, 1);
    ssd1306_refresh_gram(display_handle);
    
    display_initialized = true;
    ESP_LOGI(TAG, "OLED display initialized successfully");
}

static const char* get_aqi_status_str(int aqi)
{
    if (aqi <= 50) return "Good";
    else if (aqi <= 100) return "Moderate";
    else if (aqi <= 150) return "Unhealthy*";
    else if (aqi <= 200) return "Unhealthy";
    else if (aqi <= 300) return "Very Bad";
    else return "Hazardous";
}

static void display_sensor_data(sensor_data_t *data)
{
    if (!display_initialized || display_handle == NULL) {
        return;
    }
    
    char line_buf[32];
    
    // Clear display
    ssd1306_clear_screen(display_handle, 0x00);
    
    // Line 0: Title
    ssd1306_draw_string(display_handle, 0, 0, (const uint8_t *)"Env. Monitor", 12, 1);
    
    // Check connection status
    EventBits_t bits = xEventGroupGetBits(system_events);
    bool wifi_connected = (bits & WIFI_CONNECTED_BIT) != 0;
    bool cloud_connected = (bits & CLOUD_CONNECTED_BIT) != 0;
    
    if (cloud_connected) {
        ssd1306_draw_string(display_handle, 100, 0, (const uint8_t *)"[C]", 12, 1);
    } else if (wifi_connected) {
        ssd1306_draw_string(display_handle, 100, 0, (const uint8_t *)"[W]", 12, 1);
    }
    
    // Line 1: Temperature
    snprintf(line_buf, sizeof(line_buf), "Temp: %.1f C", data->temperature);
    ssd1306_draw_string(display_handle, 0, 16, (const uint8_t *)line_buf, 16, 1);
    
    // Line 2: Humidity
    snprintf(line_buf, sizeof(line_buf), "Humid: %.1f%%", data->humidity);
    ssd1306_draw_string(display_handle, 0, 32, (const uint8_t *)line_buf, 16, 1);
    
    // Line 3: AQI with status
    snprintf(line_buf, sizeof(line_buf), "AQI: %d", data->aqi);
    ssd1306_draw_string(display_handle, 0, 48, (const uint8_t *)line_buf, 16, 1);
    
    // Line 4: AQI Status
    const char *aqi_status = get_aqi_status_str(data->aqi);
    ssd1306_draw_string(display_handle, 60, 48, (const uint8_t *)aqi_status, 16, 1);
    
    // Refresh display
    ssd1306_refresh_gram(display_handle);
}

static void display_error_message(const char *message)
{
    if (!display_initialized || display_handle == NULL) {
        return;
    }
    
    ssd1306_clear_screen(display_handle, 0x00);
    ssd1306_draw_string(display_handle, 0, 0, (const uint8_t *)"ERROR:", 16, 1);
    ssd1306_draw_string(display_handle, 0, 16, (const uint8_t *)message, 16, 1);
    ssd1306_refresh_gram(display_handle);
}

void display_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Display task started");
    
    sensor_data_t sensor_data;
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t update_interval = pdMS_TO_TICKS(DISPLAY_UPDATE_INTERVAL_MS);
    
    // Wait for display initialization
    if (!display_initialized) {
        ESP_LOGW(TAG, "Display not initialized, attempting init...");
        display_init();
        
        if (!display_initialized) {
            ESP_LOGE(TAG, "Display initialization failed, task will exit");
            vTaskDelete(NULL);
            return;
        }
    }
    
    // Display "Waiting for data..." message
    ssd1306_clear_screen(display_handle, 0x00);
    ssd1306_draw_string(display_handle, 0, 0, (const uint8_t *)"Waiting for", 16, 1);
    ssd1306_draw_string(display_handle, 0, 16, (const uint8_t *)"sensor data...", 16, 1);
    ssd1306_refresh_gram(display_handle);
    
    uint32_t no_data_count = 0;
    
    while (1) {
        // Peek at sensor data queue (non-destructive read)
        if (xQueuePeek(sensor_data_queue, &sensor_data, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Valid data received
            display_sensor_data(&sensor_data);
            no_data_count = 0;
            
#if ENABLE_DISPLAY_DEBUG
            ESP_LOGD(TAG, "Display updated: T=%.1f H=%.1f AQI=%d", 
                     sensor_data.temperature, sensor_data.humidity, sensor_data.aqi);
#endif
        } else {
            // No data available
            no_data_count++;
            
            if (no_data_count > 5) {  // After 10 seconds of no data
                ESP_LOGW(TAG, "No sensor data received for extended period");
                display_error_message("No sensor data");
            }
        }
        
        // Update at fixed interval
        vTaskDelayUntil(&last_wake_time, update_interval);
    }
}
