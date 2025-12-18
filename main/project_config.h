/**
 * @file project_config.h
 * @brief Global project configuration and GPIO definitions
 */

#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include "driver/gpio.h"
#include "driver/adc.h"

// ============================================
// GPIO PIN DEFINITIONS
// ============================================

// Sensors
#define DHT11_GPIO              GPIO_NUM_4
#define LDR_ADC_CHANNEL         ADC1_CHANNEL_0      // GPIO0
#define LDR_GPIO                GPIO_NUM_0

// I2C for OLED Display
#define I2C_MASTER_SCL_IO       GPIO_NUM_8
#define I2C_MASTER_SDA_IO       GPIO_NUM_9
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_FREQ_HZ      100000
#define I2C_MASTER_TX_BUF_DISABLE 0
#define I2C_MASTER_RX_BUF_DISABLE 0
#define I2C_MASTER_TIMEOUT_MS   1000

// Output Indicators
#define LED_GREEN_GPIO          GPIO_NUM_2
#define LED_RED_GPIO            GPIO_NUM_3
#define BUZZER_GPIO             GPIO_NUM_10

// Input Controls
#define BUTTON_GPIO             GPIO_NUM_5

// ============================================
// APPLICATION CONFIGURATION
// ============================================

// Task Stack Sizes
#define SENSOR_TASK_STACK_SIZE      4096
#define CLOUD_TASK_STACK_SIZE       4096
#define DISPLAY_TASK_STACK_SIZE     4096
#define ALERT_TASK_STACK_SIZE       4096
#define OTA_TASK_STACK_SIZE         4096

// Task Priorities (higher number = higher priority)
#define SENSOR_TASK_PRIORITY        5
#define CLOUD_TASK_PRIORITY         4
#define DISPLAY_TASK_PRIORITY       3
#define ALERT_TASK_PRIORITY         6       // Highest priority
#define OTA_TASK_PRIORITY           2       // Lowest priority

// Task Core Assignments (ESP32-C3 is single core, but kept for compatibility)
#define SENSOR_TASK_CORE            0
#define CLOUD_TASK_CORE             0
#define DISPLAY_TASK_CORE           0
#define ALERT_TASK_CORE             0
#define OTA_TASK_CORE               0

// Queue Sizes
#define SENSOR_DATA_QUEUE_SIZE      10

// Timing Configuration (in milliseconds)
#define SENSOR_READ_INTERVAL_MS     10000   // 10 seconds
#define DISPLAY_UPDATE_INTERVAL_MS  2000    // 2 seconds
#define ALERT_CHECK_INTERVAL_MS     2000    // 2 seconds
#define OTA_CHECK_INTERVAL_MS       60000   // 60 seconds

// Alert Configuration
#define NOTIFICATION_COOLDOWN_MS    60000   // 1 minute between notifications

// Default Alert Thresholds
#define DEFAULT_TEMP_HIGH           35.0f   // °C
#define DEFAULT_TEMP_LOW            15.0f   // °C
#define DEFAULT_HUMIDITY_HIGH       80.0f   // %
#define DEFAULT_HUMIDITY_LOW        30.0f   // %
#define DEFAULT_AQI_THRESHOLD       150     // AQI value

// Sensor Configuration
#define DHT11_MAX_RETRIES           3
#define LDR_SAMPLE_COUNT            10

// ADC Configuration
#define ADC_ATTEN                   ADC_ATTEN_DB_11
#define ADC_WIDTH                   ADC_WIDTH_BIT_12

// ============================================
// RAINMAKER DEVICE NAMES
// ============================================

#define RMAKER_NODE_NAME            "Environmental Logger"
#define RMAKER_NODE_TYPE            "Sensor"

#define DEVICE_TEMP_NAME            "Temperature"
#define DEVICE_HUMIDITY_NAME        "Humidity"
#define DEVICE_AQI_NAME             "Air Quality"
#define DEVICE_ALERT_NAME           "Alert System"

// ============================================
// DEBUG CONFIGURATION
// ============================================

#define ENABLE_SERIAL_DEBUG         1
#define ENABLE_SENSOR_DEBUG         1
#define ENABLE_CLOUD_DEBUG          1
#define ENABLE_DISPLAY_DEBUG        0
#define ENABLE_ALERT_DEBUG          1

#endif // PROJECT_CONFIG_H
