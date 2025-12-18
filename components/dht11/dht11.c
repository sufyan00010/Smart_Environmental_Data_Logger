/**
 * @file dht11.c
 * @brief DHT11 Temperature and Humidity Sensor Driver Implementation
 */

#include "dht11.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

static const char *TAG = "DHT11";
static gpio_num_t dht_gpio;

// Timing constants (microseconds)
#define DHT_START_SIGNAL_LOW_TIME   18000  // 18ms
#define DHT_START_SIGNAL_HIGH_TIME  40     // 20-40us
#define DHT_RESPONSE_TIMEOUT        100    // 100us timeout for response
#define DHT_BIT_TIMEOUT             100    // 100us timeout for bit reading

esp_err_t dht11_init(gpio_num_t gpio_num)
{
    dht_gpio = gpio_num;
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << dht_gpio),
        .mode = GPIO_MODE_OUTPUT_OD,  // Open drain mode
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed");
        return ret;
    }
    
    gpio_set_level(dht_gpio, 1);  // Set high initially
    
    ESP_LOGI(TAG, "DHT11 initialized on GPIO%d", dht_gpio);
    return ESP_OK;
}

static int wait_for_level(int level, int timeout_us)
{
    int elapsed = 0;
    
    while (gpio_get_level(dht_gpio) != level) {
        if (elapsed > timeout_us) {
            return -1;  // Timeout
        }
        ets_delay_us(1);
        elapsed++;
    }
    
    return elapsed;
}

esp_err_t dht11_read(float *temperature, float *humidity)
{
    uint8_t data[5] = {0};
    
    // Disable interrupts for precise timing
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);
    
    // Send start signal
    gpio_set_direction(dht_gpio, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_gpio, 0);
    ets_delay_us(DHT_START_SIGNAL_LOW_TIME);
    
    gpio_set_level(dht_gpio, 1);
    ets_delay_us(DHT_START_SIGNAL_HIGH_TIME);
    
    // Switch to input mode
    gpio_set_direction(dht_gpio, GPIO_MODE_INPUT);
    
    // Wait for DHT11 response (LOW)
    if (wait_for_level(0, DHT_RESPONSE_TIMEOUT) < 0) {
        portEXIT_CRITICAL(&mux);
        ESP_LOGW(TAG, "No response from DHT11 (low)");
        return ESP_FAIL;
    }
    
    // Wait for DHT11 response (HIGH)
    if (wait_for_level(1, DHT_RESPONSE_TIMEOUT) < 0) {
        portEXIT_CRITICAL(&mux);
        ESP_LOGW(TAG, "No response from DHT11 (high)");
        return ESP_FAIL;
    }
    
    // Wait for DHT11 to start sending data (LOW)
    if (wait_for_level(0, DHT_RESPONSE_TIMEOUT) < 0) {
        portEXIT_CRITICAL(&mux);
        ESP_LOGW(TAG, "No data start signal from DHT11");
        return ESP_FAIL;
    }
    
    // Read 40 bits (5 bytes)
    for (int i = 0; i < 40; i++) {
        // Wait for bit to start (HIGH)
        if (wait_for_level(1, DHT_BIT_TIMEOUT) < 0) {
            portEXIT_CRITICAL(&mux);
            ESP_LOGW(TAG, "Timeout reading bit %d", i);
            return ESP_FAIL;
        }
        
        // Measure HIGH pulse duration
        ets_delay_us(30);  // Wait 30us
        
        int level = gpio_get_level(dht_gpio);
        
        // Wait for bit to end (LOW)
        if (wait_for_level(0, DHT_BIT_TIMEOUT) < 0) {
            portEXIT_CRITICAL(&mux);
            ESP_LOGW(TAG, "Timeout ending bit %d", i);
            return ESP_FAIL;
        }
        
        // If still HIGH after 30us, it's a '1', otherwise '0'
        data[i / 8] <<= 1;
        if (level == 1) {
            data[i / 8] |= 1;
        }
    }
    
    portEXIT_CRITICAL(&mux);
    
    // Verify checksum
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGW(TAG, "Checksum error: calc=0x%02X, recv=0x%02X", checksum, data[4]);
        return ESP_FAIL;
    }
    
    // Parse data
    *humidity = (float)data[0] + (float)data[1] / 10.0f;
    *temperature = (float)data[2] + (float)data[3] / 10.0f;
    
    // Validate ranges
    if (*temperature < -40.0f || *temperature > 80.0f || 
        *humidity < 0.0f || *humidity > 100.0f) {
        ESP_LOGW(TAG, "Invalid readings: T=%.1f, H=%.1f", *temperature, *humidity);
        return ESP_FAIL;
    }
    
    ESP_LOGD(TAG, "Read success: T=%.1f°C, H=%.1f%%", *temperature, *humidity);
    
    return ESP_OK;
}
