#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_ota.h>
#include <app_insights.h>
#include <app_wifi.h>
#include <wifi_provisioning/manager.h>

static const char *TAG = "APP_MAIN";

// ============================================
// GLOBAL HANDLES & DEFINITIONS
// ============================================

// Task handles
TaskHandle_t sensor_task_handle = NULL;
TaskHandle_t cloud_task_handle = NULL;
TaskHandle_t display_task_handle = NULL;
TaskHandle_t alert_task_handle = NULL;
TaskHandle_t ota_task_handle = NULL;

// Queue for sensor data
QueueHandle_t sensor_data_queue = NULL;

// Semaphore for shared resource protection
SemaphoreHandle_t rainmaker_mutex = NULL;

// Event group for system events
EventGroupHandle_t system_events = NULL;
#define WIFI_CONNECTED_BIT BIT0
#define CLOUD_CONNECTED_BIT BIT1
#define ALERT_TRIGGERED_BIT BIT2

// RainMaker device handles
esp_rmaker_device_t *temp_sensor_device = NULL;
esp_rmaker_device_t *humidity_sensor_device = NULL;
esp_rmaker_device_t *aqi_sensor_device = NULL;
esp_rmaker_device_t *alert_device = NULL;

// Sensor data structure
typedef struct {
    float temperature;
    float humidity;
    int aqi;
    uint32_t timestamp;
} sensor_data_t;

// Alert thresholds (can be modified via RainMaker)
typedef struct {
    float temp_high;
    float temp_low;
    float humidity_high;
    float humidity_low;
    int aqi_threshold;
    bool buzzer_enabled;
} alert_config_t;

alert_config_t alert_config = {
    .temp_high = 35.0,
    .temp_low = 15.0,
    .humidity_high = 80.0,
    .humidity_low = 30.0,
    .aqi_threshold = 150,
    .buzzer_enabled = true
};

// ============================================
// EXTERNAL FUNCTION DECLARATIONS
// ============================================

// From sensor_task.h
#include "sensor_task.h"

// From cloud_task.h
#include "cloud_task.h"

// From display_task.h
#include "display_task.h"

// From alert_task.h
#include "alert_task.h"

// From ota_task.h
#include "ota_task.h"

// From app_driver.h
#include "app_driver.h"

// From project_config.h
#include "project_config.h"

// ============================================
// RAINMAKER CALLBACK FUNCTIONS
// ============================================

// Write callback for temperature sensor device
static esp_err_t temp_sensor_write_cb(const esp_rmaker_device_t *device, 
                                       const esp_rmaker_param_t *param,
                                       const esp_rmaker_param_val_t val, 
                                       void *priv_data,
                                       esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via: %s", 
                 esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    const char *param_name = esp_rmaker_param_get_name(param);
    
    if (strcmp(param_name, "Temp High Threshold") == 0) {
        alert_config.temp_high = val.val.f;
        ESP_LOGI(TAG, "Updated temp_high threshold: %.1f", alert_config.temp_high);
    } else if (strcmp(param_name, "Temp Low Threshold") == 0) {
        alert_config.temp_low = val.val.f;
        ESP_LOGI(TAG, "Updated temp_low threshold: %.1f", alert_config.temp_low);
    }
    
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

// Write callback for humidity sensor device
static esp_err_t humidity_sensor_write_cb(const esp_rmaker_device_t *device, 
                                          const esp_rmaker_param_t *param,
                                          const esp_rmaker_param_val_t val, 
                                          void *priv_data,
                                          esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);
    
    if (strcmp(param_name, "Humidity High Threshold") == 0) {
        alert_config.humidity_high = val.val.f;
        ESP_LOGI(TAG, "Updated humidity_high threshold: %.1f", alert_config.humidity_high);
    } else if (strcmp(param_name, "Humidity Low Threshold") == 0) {
        alert_config.humidity_low = val.val.f;
        ESP_LOGI(TAG, "Updated humidity_low threshold: %.1f", alert_config.humidity_low);
    }
    
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

// Write callback for alert device
static esp_err_t alert_device_write_cb(const esp_rmaker_device_t *device, 
                                       const esp_rmaker_param_t *param,
                                       const esp_rmaker_param_val_t val, 
                                       void *priv_data,
                                       esp_rmaker_write_ctx_t *ctx)
{
    const char *param_name = esp_rmaker_param_get_name(param);
    
    if (strcmp(param_name, "Buzzer") == 0) {
        alert_config.buzzer_enabled = val.val.b;
        ESP_LOGI(TAG, "Buzzer %s", alert_config.buzzer_enabled ? "ENABLED" : "DISABLED");
    }
    
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

// ============================================
// RAINMAKER DEVICE CREATION
// ============================================

static void create_rainmaker_devices(esp_rmaker_node_t *node)
{
    // 1. Temperature Sensor Device
    temp_sensor_device = esp_rmaker_temp_sensor_device_create("Temperature", NULL, 25.0);
    esp_rmaker_device_add_cb(temp_sensor_device, temp_sensor_write_cb, NULL);
    
    // Add threshold parameters
    esp_rmaker_param_t *temp_high_param = esp_rmaker_param_create(
        "Temp High Threshold", NULL, esp_rmaker_float(35.0),
        PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(temp_high_param, ESP_RMAKER_UI_SLIDER);
    esp_rmaker_param_add_bounds(temp_high_param, esp_rmaker_float(25.0), 
                                 esp_rmaker_float(50.0), esp_rmaker_float(1.0));
    esp_rmaker_device_add_param(temp_sensor_device, temp_high_param);
    
    esp_rmaker_param_t *temp_low_param = esp_rmaker_param_create(
        "Temp Low Threshold", NULL, esp_rmaker_float(15.0),
        PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(temp_low_param, ESP_RMAKER_UI_SLIDER);
    esp_rmaker_param_add_bounds(temp_low_param, esp_rmaker_float(0.0), 
                                 esp_rmaker_float(25.0), esp_rmaker_float(1.0));
    esp_rmaker_device_add_param(temp_sensor_device, temp_low_param);
    
    esp_rmaker_node_add_device(node, temp_sensor_device);

    // 2. Humidity Sensor Device
    humidity_sensor_device = esp_rmaker_device_create("Humidity", 
        ESP_RMAKER_DEVICE_TEMP_SENSOR, NULL);
    esp_rmaker_device_add_cb(humidity_sensor_device, humidity_sensor_write_cb, NULL);
    
    esp_rmaker_param_t *humidity_param = esp_rmaker_param_create(
        ESP_RMAKER_DEF_HUMIDITY_NAME, ESP_RMAKER_PARAM_HUMIDITY,
        esp_rmaker_float(50.0), PROP_FLAG_READ);
    esp_rmaker_device_add_param(humidity_sensor_device, humidity_param);
    esp_rmaker_device_assign_primary_param(humidity_sensor_device, humidity_param);
    
    // Humidity thresholds
    esp_rmaker_param_t *hum_high_param = esp_rmaker_param_create(
        "Humidity High Threshold", NULL, esp_rmaker_float(80.0),
        PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(hum_high_param, ESP_RMAKER_UI_SLIDER);
    esp_rmaker_param_add_bounds(hum_high_param, esp_rmaker_float(60.0), 
                                 esp_rmaker_float(100.0), esp_rmaker_float(5.0));
    esp_rmaker_device_add_param(humidity_sensor_device, hum_high_param);
    
    esp_rmaker_param_t *hum_low_param = esp_rmaker_param_create(
        "Humidity Low Threshold", NULL, esp_rmaker_float(30.0),
        PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(hum_low_param, ESP_RMAKER_UI_SLIDER);
    esp_rmaker_param_add_bounds(hum_low_param, esp_rmaker_float(0.0), 
                                 esp_rmaker_float(40.0), esp_rmaker_float(5.0));
    esp_rmaker_device_add_param(humidity_sensor_device, hum_low_param);
    
    esp_rmaker_node_add_device(node, humidity_sensor_device);

    // 3. Air Quality Index Device
    aqi_sensor_device = esp_rmaker_device_create("Air Quality", 
        ESP_RMAKER_DEVICE_TEMP_SENSOR, NULL);
    
    esp_rmaker_param_t *aqi_param = esp_rmaker_param_create(
        "AQI", NULL, esp_rmaker_int(50), PROP_FLAG_READ);
    esp_rmaker_param_add_ui_type(aqi_param, ESP_RMAKER_UI_TEXT);
    esp_rmaker_device_add_param(aqi_sensor_device, aqi_param);
    esp_rmaker_device_assign_primary_param(aqi_sensor_device, aqi_param);
    
    esp_rmaker_param_t *aqi_status_param = esp_rmaker_param_create(
        "Air Quality Status", NULL, esp_rmaker_str("Good"), PROP_FLAG_READ);
    esp_rmaker_device_add_param(aqi_sensor_device, aqi_status_param);
    
    esp_rmaker_node_add_device(node, aqi_sensor_device);

    // 4. Alert Control Device
    alert_device = esp_rmaker_switch_device_create("Alert System", NULL, false);
    esp_rmaker_device_add_cb(alert_device, alert_device_write_cb, NULL);
    
    esp_rmaker_param_t *buzzer_param = esp_rmaker_param_create(
        "Buzzer", NULL, esp_rmaker_bool(true),
        PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(buzzer_param, ESP_RMAKER_UI_TOGGLE);
    esp_rmaker_device_add_param(alert_device, buzzer_param);
    
    esp_rmaker_param_t *alert_status_param = esp_rmaker_param_create(
        "Alert Status", NULL, esp_rmaker_str("Normal"), PROP_FLAG_READ);
    esp_rmaker_device_add_param(alert_device, alert_status_param);
    
    esp_rmaker_node_add_device(node, alert_device);
}

// ============================================
// MAIN APPLICATION
// ============================================

void app_main(void)
{
    ESP_LOGI(TAG, "=== Smart Environmental Data Logger ===");
    ESP_LOGI(TAG, "Version: %s", PROJECT_VER);

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Initialize hardware drivers
    app_driver_init();
    sensor_init();
    display_init();

    // Create FreeRTOS synchronization objects
    sensor_data_queue = xQueueCreate(10, sizeof(sensor_data_t));
    rainmaker_mutex = xSemaphoreCreateMutex();
    system_events = xEventGroupCreate();

    if (!sensor_data_queue || !rainmaker_mutex || !system_events) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS objects!");
        abort();
    }

    // Initialize Wi-Fi
    app_wifi_init();

    // Initialize RainMaker
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = true,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, 
        "Environmental Logger", "Sensor");
    
    if (!node) {
        ESP_LOGE(TAG, "Failed to initialize RainMaker node!");
        abort();
    }

    // Create all devices
    create_rainmaker_devices(node);

    // Enable RainMaker services
    esp_rmaker_ota_enable_default();
    esp_rmaker_timezone_service_enable();
    esp_rmaker_schedule_enable();
    esp_rmaker_scenes_enable();
    
    // Enable ESP Insights for dashboard
    app_insights_enable();

    // Start RainMaker
    ESP_ERROR_CHECK(esp_rmaker_start());

    // Start Wi-Fi (provisioning if needed)
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi!");
    }

    // Create FreeRTOS tasks
    xTaskCreatePinnedToCore(sensor_task, "Sensor", 4096, NULL, 5, 
                           &sensor_task_handle, 1);
    xTaskCreatePinnedToCore(cloud_task, "Cloud", 4096, NULL, 4, 
                           &cloud_task_handle, 0);
    xTaskCreatePinnedToCore(display_task, "Display", 4096, NULL, 3, 
                           &display_task_handle, 1);
    xTaskCreatePinnedToCore(alert_task, "Alert", 4096, NULL, 6, 
                           &alert_task_handle, 1);
    xTaskCreatePinnedToCore(ota_task, "OTA", 4096, NULL, 2, 
                           &ota_task_handle, 0);

    ESP_LOGI(TAG, "All tasks created successfully!");
}
