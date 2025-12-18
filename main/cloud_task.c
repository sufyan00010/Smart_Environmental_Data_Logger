#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>

static const char *TAG = "CLOUD_TASK";

// External references
extern QueueHandle_t sensor_data_queue;
extern SemaphoreHandle_t rainmaker_mutex;
extern EventGroupHandle_t system_events;
extern esp_rmaker_device_t *temp_sensor_device;
extern esp_rmaker_device_t *humidity_sensor_device;
extern esp_rmaker_device_t *aqi_sensor_device;

#define WIFI_CONNECTED_BIT BIT0
#define CLOUD_CONNECTED_BIT BIT1

// Sensor data structure
typedef struct {
    float temperature;
    float humidity;
    int aqi;
    uint32_t timestamp;
} sensor_data_t;

// ============================================
// AQI STATUS STRING CONVERTER
// ============================================

static const char* get_aqi_status_string(int aqi)
{
    if (aqi <= 50) return "Good";
    else if (aqi <= 100) return "Moderate";
    else if (aqi <= 150) return "Unhealthy for Sensitive";
    else if (aqi <= 200) return "Unhealthy";
    else if (aqi <= 300) return "Very Unhealthy";
    else return "Hazardous";
}

// ============================================
// RAINMAKER UPDATE FUNCTION
// ============================================

static void update_rainmaker_params(sensor_data_t *data)
{
    esp_err_t err;
    
    // Take mutex to protect RainMaker API calls
    if (xSemaphoreTake(rainmaker_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        
        // Update Temperature
        if (temp_sensor_device) {
            esp_rmaker_param_t *temp_param = 
                esp_rmaker_device_get_param_by_type(temp_sensor_device, 
                    ESP_RMAKER_PARAM_TEMPERATURE);
            if (temp_param) {
                err = esp_rmaker_param_update_and_report(temp_param, 
                    esp_rmaker_float(data->temperature));
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Updated temperature: %.1f°C", data->temperature);
                } else {
                    ESP_LOGW(TAG, "Failed to update temperature: %s", 
                             esp_err_to_name(err));
                }
            }
        }
        
        // Update Humidity
        if (humidity_sensor_device) {
            esp_rmaker_param_t *hum_param = 
                esp_rmaker_device_get_param_by_name(humidity_sensor_device, 
                    ESP_RMAKER_DEF_HUMIDITY_NAME);
            if (hum_param) {
                err = esp_rmaker_param_update_and_report(hum_param, 
                    esp_rmaker_float(data->humidity));
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Updated humidity: %.1f%%", data->humidity);
                } else {
                    ESP_LOGW(TAG, "Failed to update humidity: %s", 
                             esp_err_to_name(err));
                }
            }
        }
        
        // Update AQI
        if (aqi_sensor_device) {
            esp_rmaker_param_t *aqi_param = 
                esp_rmaker_device_get_param_by_name(aqi_sensor_device, "AQI");
            if (aqi_param) {
                err = esp_rmaker_param_update_and_report(aqi_param, 
                    esp_rmaker_int(data->aqi));
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Updated AQI: %d", data->aqi);
                } else {
                    ESP_LOGW(TAG, "Failed to update AQI: %s", 
                             esp_err_to_name(err));
                }
            }
            
            // Update AQI Status String
            esp_rmaker_param_t *aqi_status_param = 
                esp_rmaker_device_get_param_by_name(aqi_sensor_device, 
                    "Air Quality Status");
            if (aqi_status_param) {
                const char *status_str = get_aqi_status_string(data->aqi);
                err = esp_rmaker_param_update_and_report(aqi_status_param, 
                    esp_rmaker_str(status_str));
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "Updated AQI status: %s", status_str);
                }
            }
        }
        
        xSemaphoreGive(rainmaker_mutex);
        
    } else {
        ESP_LOGW(TAG, "Failed to acquire RainMaker mutex");
    }
}

// ============================================
// CUSTOM METRICS FOR ESP INSIGHTS
// ============================================

static void send_custom_metrics(sensor_data_t *data)
{
    // Send custom metrics to ESP Insights dashboard
    // These will appear in the Insights dashboard for analytics
    
    // Example: Track how often AQI exceeds thresholds
    static uint32_t aqi_good_count = 0;
    static uint32_t aqi_moderate_count = 0;
    static uint32_t aqi_unhealthy_count = 0;
    
    if (data->aqi <= 50) {
        aqi_good_count++;
    } else if (data->aqi <= 100) {
        aqi_moderate_count++;
    } else {
        aqi_unhealthy_count++;
    }
    
    // Log metrics (ESP Insights captures these)
    ESP_LOGI(TAG, "AQI Metrics - Good:%lu Moderate:%lu Unhealthy:%lu",
             aqi_good_count, aqi_moderate_count, aqi_unhealthy_count);
    
    // You can also use esp_diag_metrics_register() and esp_diag_metrics_add()
    // for structured metrics if you've set up ESP Insights properly
}

// ============================================
// CONNECTION STATUS MONITOR
// ============================================

static bool check_cloud_connection(void)
{
    EventBits_t bits = xEventGroupGetBits(system_events);
    bool wifi_connected = (bits & WIFI_CONNECTED_BIT) != 0;
    bool cloud_connected = (bits & CLOUD_CONNECTED_BIT) != 0;
    
    if (!wifi_connected) {
        ESP_LOGW(TAG, "Wi-Fi not connected");
        return false;
    }
    
    if (!cloud_connected) {
        ESP_LOGW(TAG, "RainMaker cloud not connected");
        return false;
    }
    
    return true;
}

// ============================================
// MAIN CLOUD TASK
// ============================================

void cloud_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Cloud communication task started");
    
    sensor_data_t sensor_data;
    uint32_t update_count = 0;
    
    // Wait a bit for system initialization
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    while (1) {
        // Wait for sensor data from queue (blocking wait)
        if (xQueueReceive(sensor_data_queue, &sensor_data, portMAX_DELAY) == pdTRUE) {
            
            ESP_LOGI(TAG, "Received sensor data - T:%.1f H:%.1f AQI:%d", 
                     sensor_data.temperature, sensor_data.humidity, sensor_data.aqi);
            
            // Check connection status
            if (check_cloud_connection()) {
                
                // Update RainMaker parameters
                update_rainmaker_params(&sensor_data);
                
                // Send custom metrics to Insights
                send_custom_metrics(&sensor_data);
                
                update_count++;
                ESP_LOGI(TAG, "Cloud update #%lu successful", update_count);
                
            } else {
                ESP_LOGW(TAG, "Cloud not connected, data not sent");
                // Could implement local storage/buffering here
            }
            
            // Small delay to avoid flooding the cloud
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

// ============================================
// EVENT HANDLERS (Called from app_main)
// ============================================

// Call this from Wi-Fi event handler
void cloud_task_wifi_connected(void)
{
    xEventGroupSetBits(system_events, WIFI_CONNECTED_BIT);
    ESP_LOGI(TAG, "Wi-Fi connected event received");
}

void cloud_task_wifi_disconnected(void)
{
    xEventGroupClearBits(system_events, WIFI_CONNECTED_BIT);
    xEventGroupClearBits(system_events, CLOUD_CONNECTED_BIT);
    ESP_LOGW(TAG, "Wi-Fi disconnected event received");
}

void cloud_task_cloud_connected(void)
{
    xEventGroupSetBits(system_events, CLOUD_CONNECTED_BIT);
    ESP_LOGI(TAG, "RainMaker cloud connected");
}

void cloud_task_cloud_disconnected(void)
{
    xEventGroupClearBits(system_events, CLOUD_CONNECTED_BIT);
    ESP_LOGW(TAG, "RainMaker cloud disconnected");
}
