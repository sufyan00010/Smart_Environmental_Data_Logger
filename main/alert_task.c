#include "alert_task.h"
#include "sensor_task.h"
#include "project_config.h"
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_rmaker_core.h>
#include <string.h>

static const char *TAG = "ALERT_TASK";

// GPIO definitions
#define LED_GREEN_GPIO GPIO_NUM_2
#define LED_RED_GPIO GPIO_NUM_3
#define BUZZER_GPIO GPIO_NUM_10

// External references
extern QueueHandle_t sensor_data_queue;
extern SemaphoreHandle_t rainmaker_mutex;
extern esp_rmaker_device_t *alert_device;

// Alert configuration (shared with app_main)
typedef struct {
    float temp_high;
    float temp_low;
    float humidity_high;
    float humidity_low;
    int aqi_threshold;
    bool buzzer_enabled;
} alert_config_t;

extern alert_config_t alert_config;

// Sensor data structure
typedef struct {
    float temperature;
    float humidity;
    int aqi;
    uint32_t timestamp;
} sensor_data_t;

// Alert state tracking
typedef enum {
    ALERT_NONE = 0,
    ALERT_TEMP_HIGH,
    ALERT_TEMP_LOW,
    ALERT_HUMIDITY_HIGH,
    ALERT_HUMIDITY_LOW,
    ALERT_AQI_HIGH
} alert_type_t;

static alert_type_t current_alert = ALERT_NONE;
static uint32_t last_notification_time = 0;
#define NOTIFICATION_COOLDOWN_MS 60000  // 1 minute between same notifications

// ============================================
// HARDWARE INITIALIZATION
// ============================================

// Note: GPIO initialization is handled in app_driver.c
// This task only controls the GPIOs

// ============================================
// ALERT CONTROL FUNCTIONS
// ============================================

static void set_normal_status(void)
{
    gpio_set_level(LED_GREEN_GPIO, 1);  // Green ON
    gpio_set_level(LED_RED_GPIO, 0);    // Red OFF
    gpio_set_level(BUZZER_GPIO, 0);     // Buzzer OFF
}

static void set_alert_status(bool buzzer_on)
{
    gpio_set_level(LED_GREEN_GPIO, 0);  // Green OFF
    gpio_set_level(LED_RED_GPIO, 1);    // Red ON
    
    if (buzzer_on && alert_config.buzzer_enabled) {
        gpio_set_level(BUZZER_GPIO, 1); // Buzzer ON
    } else {
        gpio_set_level(BUZZER_GPIO, 0);
    }
}

static void buzzer_beep_pattern(int beeps, int duration_ms)
{
    if (!alert_config.buzzer_enabled) return;
    
    for (int i = 0; i < beeps; i++) {
        gpio_set_level(BUZZER_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        gpio_set_level(BUZZER_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
    }
}

// ============================================
// ALERT DETECTION
// ============================================

static alert_type_t detect_alert(sensor_data_t *data)
{
    // Check temperature thresholds
    if (data->temperature > alert_config.temp_high) {
        ESP_LOGW(TAG, "ALERT: Temperature too high! %.1f > %.1f", 
                 data->temperature, alert_config.temp_high);
        return ALERT_TEMP_HIGH;
    }
    
    if (data->temperature < alert_config.temp_low) {
        ESP_LOGW(TAG, "ALERT: Temperature too low! %.1f < %.1f", 
                 data->temperature, alert_config.temp_low);
        return ALERT_TEMP_LOW;
    }
    
    // Check humidity thresholds
    if (data->humidity > alert_config.humidity_high) {
        ESP_LOGW(TAG, "ALERT: Humidity too high! %.1f > %.1f", 
                 data->humidity, alert_config.humidity_high);
        return ALERT_HUMIDITY_HIGH;
    }
    
    if (data->humidity < alert_config.humidity_low) {
        ESP_LOGW(TAG, "ALERT: Humidity too low! %.1f < %.1f", 
                 data->humidity, alert_config.humidity_low);
        return ALERT_HUMIDITY_LOW;
    }
    
    // Check AQI threshold
    if (data->aqi > alert_config.aqi_threshold) {
        ESP_LOGW(TAG, "ALERT: Air quality poor! AQI=%d > %d", 
                 data->aqi, alert_config.aqi_threshold);
        return ALERT_AQI_HIGH;
    }
    
    return ALERT_NONE;
}

// ============================================
// PUSH NOTIFICATION VIA RAINMAKER
// ============================================

static void send_push_notification(alert_type_t alert_type, sensor_data_t *data)
{
    // Check cooldown period to avoid notification spam
    uint32_t current_time = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    
    if ((current_time - last_notification_time) < NOTIFICATION_COOLDOWN_MS) {
        ESP_LOGD(TAG, "Notification cooldown active, skipping");
        return;
    }
    
    char alert_message[128];
    const char *alert_title = "Environmental Alert!";
    
    // Construct alert message based on type
    switch (alert_type) {
        case ALERT_TEMP_HIGH:
            snprintf(alert_message, sizeof(alert_message),
                     "⚠️ High Temperature Detected: %.1f°C (Threshold: %.1f°C)",
                     data->temperature, alert_config.temp_high);
            break;
            
        case ALERT_TEMP_LOW:
            snprintf(alert_message, sizeof(alert_message),
                     "❄️ Low Temperature Detected: %.1f°C (Threshold: %.1f°C)",
                     data->temperature, alert_config.temp_low);
            break;
            
        case ALERT_HUMIDITY_HIGH:
            snprintf(alert_message, sizeof(alert_message),
                     "💧 High Humidity Detected: %.1f%% (Threshold: %.1f%%)",
                     data->humidity, alert_config.humidity_high);
            break;
            
        case ALERT_HUMIDITY_LOW:
            snprintf(alert_message, sizeof(alert_message),
                     "🏜️ Low Humidity Detected: %.1f%% (Threshold: %.1f%%)",
                     data->humidity, alert_config.humidity_low);
            break;
            
        case ALERT_AQI_HIGH:
            snprintf(alert_message, sizeof(alert_message),
                     "🌫️ Poor Air Quality: AQI=%d (Threshold: %d)",
                     data->aqi, alert_config.aqi_threshold);
            break;
            
        default:
            return;
    }
    
    ESP_LOGW(TAG, "Sending push notification: %s", alert_message);
    
    // Update RainMaker alert status (this appears in the app)
    if (xSemaphoreTake(rainmaker_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        
        if (alert_device) {
            esp_rmaker_param_t *alert_status_param = 
                esp_rmaker_device_get_param_by_name(alert_device, "Alert Status");
            
            if (alert_status_param) {
                // Update alert status parameter (will trigger app notification)
                esp_rmaker_param_update_and_report(alert_status_param, 
                    esp_rmaker_str(alert_message));
                
                ESP_LOGI(TAG, "Push notification sent via RainMaker");
            }
            
            // Also raise an alert event (if you've configured RainMaker alerts)
            // esp_rmaker_raise_alert(alert_title, alert_message);
        }
        
        xSemaphoreGive(rainmaker_mutex);
    }
    
    last_notification_time = current_time;
}

// ============================================
// MAIN ALERT TASK
// ============================================

void alert_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Alert monitoring task started");
    
    sensor_data_t sensor_data;
    alert_type_t detected_alert;
    
    // Create a copy of the queue for monitoring
    QueueHandle_t alert_queue = sensor_data_queue;
    
    // Initial status: normal
    set_normal_status();
    
    while (1) {
        // Peek at sensor data queue (non-blocking)
        if (xQueuePeek(alert_queue, &sensor_data, pdMS_TO_TICKS(1000)) == pdTRUE) {
            
            // Detect if any alert condition is met
            detected_alert = detect_alert(&sensor_data);
            
            if (detected_alert != ALERT_NONE) {
                // Alert condition detected!
                
                if (current_alert != detected_alert) {
                    // New alert type
                    ESP_LOGW(TAG, "New alert detected: %d", detected_alert);
                    
                    // Send push notification
                    send_push_notification(detected_alert, &sensor_data);
                    
                    // Update hardware status
                    set_alert_status(true);
                    buzzer_beep_pattern(3, 200);  // 3 beeps
                    
                    current_alert = detected_alert;
                }
                
                // Keep alert status active
                set_alert_status(false);  // Buzzer off after initial beeps
                
            } else {
                // No alert conditions
                if (current_alert != ALERT_NONE) {
                    ESP_LOGI(TAG, "Alert condition cleared");
                    
                    // Update RainMaker
                    if (xSemaphoreTake(rainmaker_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                        if (alert_device) {
                            esp_rmaker_param_t *alert_status_param = 
                                esp_rmaker_device_get_param_by_name(alert_device, 
                                    "Alert Status");
                            if (alert_status_param) {
                                esp_rmaker_param_update_and_report(alert_status_param, 
                                    esp_rmaker_str("Normal"));
                            }
                        }
                        xSemaphoreGive(rainmaker_mutex);
                    }
                }
                
                // Return to normal status
                set_normal_status();
                current_alert = ALERT_NONE;
            }
        }
        
        // Check at reasonable intervals
        vTaskDelay(pdMS_TO_TICKS(2000));  // Every 2 seconds
    }
}
