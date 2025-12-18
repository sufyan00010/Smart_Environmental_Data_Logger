/**
 * @file ota_task.c
 * @brief OTA update monitoring task implementation
 */

#include "ota_task.h"
#include "project_config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_app_desc.h>
#include <driver/gpio.h>

static const char *TAG = "OTA_TASK";

static void log_firmware_info(void)
{
    const esp_app_desc_t *app_desc = esp_app_get_description();
    
    ESP_LOGI(TAG, "=== Firmware Information ===");
    ESP_LOGI(TAG, "Project Name: %s", app_desc->project_name);
    ESP_LOGI(TAG, "Version: %s", app_desc->version);
    ESP_LOGI(TAG, "Compile Date: %s", app_desc->date);
    ESP_LOGI(TAG, "Compile Time: %s", app_desc->time);
    ESP_LOGI(TAG, "IDF Version: %s", app_desc->idf_ver);
    
    // Get running partition info
    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition: %s (offset: 0x%lx, size: 0x%lx)", 
             running->label, running->address, running->size);
    
    // Get boot partition info
    const esp_partition_t *boot = esp_ota_get_boot_partition();
    ESP_LOGI(TAG, "Boot partition: %s", boot->label);
    
    // Check if we're running from factory or OTA partition
    if (running == boot) {
        ESP_LOGI(TAG, "Running from boot partition (normal boot)");
    } else {
        ESP_LOGW(TAG, "Running partition differs from boot partition (OTA update pending?)");
    }
}

static void blink_led_ota_pattern(void)
{
    // Blink green LED in a specific pattern during OTA
    for (int i = 0; i < 3; i++) {
        gpio_set_level(LED_GREEN_GPIO, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(LED_GREEN_GPIO, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void ota_task(void *pvParameters)
{
    ESP_LOGI(TAG, "OTA monitoring task started");
    
    // Log firmware information at startup
    log_firmware_info();
    
    // Check if this is first boot after OTA update
    esp_ota_img_states_t ota_state;
    const esp_partition_t *running = esp_ota_get_running_partition();
    
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "First boot after OTA update detected!");
            ESP_LOGI(TAG, "Firmware appears to be working correctly");
            
            // Mark the new firmware as valid
            esp_ota_mark_app_valid_cancel_rollback();
            ESP_LOGI(TAG, "OTA update marked as successful");
            
            // Visual indication
            blink_led_ota_pattern();
        }
    }
    
    uint32_t check_count = 0;
    
    while (1) {
        // Periodic monitoring
        check_count++;
        
        // Every 10 checks (10 minutes), log status
        if (check_count % 10 == 0) {
            ESP_LOGI(TAG, "OTA monitoring active (checks: %lu)", check_count);
            
            // Check partition state
            if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
                switch (ota_state) {
                    case ESP_OTA_IMG_VALID:
                        ESP_LOGD(TAG, "Current firmware validated");
                        break;
                    case ESP_OTA_IMG_UNDEFINED:
                        ESP_LOGW(TAG, "Firmware state undefined");
                        break;
                    case ESP_OTA_IMG_INVALID:
                        ESP_LOGE(TAG, "Current firmware marked as invalid!");
                        break;
                    case ESP_OTA_IMG_ABORTED:
                        ESP_LOGW(TAG, "Previous OTA update was aborted");
                        break;
                    case ESP_OTA_IMG_NEW:
                        ESP_LOGI(TAG, "Running new firmware (first boot)");
                        break;
                    case ESP_OTA_IMG_PENDING_VERIFY:
                        ESP_LOGW(TAG, "Firmware pending verification");
                        break;
                    default:
                        break;
                }
            }
        }
        
        // Note: OTA update itself is handled by esp_rmaker_ota_enable_default()
        // This task just monitors the process
        
        // Check every minute
        vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
    }
}
