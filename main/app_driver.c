/**
 * @file app_driver.c
 * @brief Hardware driver initialization implementation
 */

#include "app_driver.h"
#include "project_config.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

static const char *TAG = "APP_DRIVER";

esp_err_t app_driver_init_i2c(void)
{
    ESP_LOGI(TAG, "Initializing I2C bus...");
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 
                            I2C_MASTER_RX_BUF_DISABLE, 
                            I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "I2C initialized successfully (SDA: GPIO%d, SCL: GPIO%d)", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    
    return ESP_OK;
}

esp_err_t app_driver_init_gpio(void)
{
    ESP_LOGI(TAG, "Initializing GPIO pins...");
    
    // Configure LED GPIOs
    gpio_config_t led_cfg = {
        .pin_bit_mask = (1ULL << LED_GREEN_GPIO) | (1ULL << LED_RED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    esp_err_t err = gpio_config(&led_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LED GPIO config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // Configure buzzer GPIO
    gpio_config_t buzzer_cfg = {
        .pin_bit_mask = (1ULL << BUZZER_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    err = gpio_config(&buzzer_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Buzzer GPIO config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // Configure button GPIO
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    err = gpio_config(&btn_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Button GPIO config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // Initialize all outputs to OFF
    gpio_set_level(LED_GREEN_GPIO, 0);
    gpio_set_level(LED_RED_GPIO, 0);
    gpio_set_level(BUZZER_GPIO, 0);
    
    ESP_LOGI(TAG, "GPIO initialized successfully");
    ESP_LOGI(TAG, "  LED Green: GPIO%d", LED_GREEN_GPIO);
    ESP_LOGI(TAG, "  LED Red:   GPIO%d", LED_RED_GPIO);
    ESP_LOGI(TAG, "  Buzzer:    GPIO%d", BUZZER_GPIO);
    ESP_LOGI(TAG, "  Button:    GPIO%d", BUTTON_GPIO);
    
    return ESP_OK;
}

esp_err_t app_driver_init_adc(void)
{
    ESP_LOGI(TAG, "Initializing ADC for LDR sensor...");
    
    // Configure ADC width
    esp_err_t err = adc1_config_width(ADC_WIDTH);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC width config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // Configure ADC attenuation for the channel
    err = adc1_config_channel_atten(LDR_ADC_CHANNEL, ADC_ATTEN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC channel config failed: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "ADC initialized successfully (GPIO%d, Channel %d)", 
             LDR_GPIO, LDR_ADC_CHANNEL);
    
    return ESP_OK;
}

esp_err_t app_driver_init(void)
{
    ESP_LOGI(TAG, "=== Initializing Hardware Drivers ===");
    
    esp_err_t err;
    
    // Initialize I2C
    err = app_driver_init_i2c();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed!");
        return err;
    }
    
    // Initialize GPIO
    err = app_driver_init_gpio();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO initialization failed!");
        return err;
    }
    
    // Initialize ADC
    err = app_driver_init_adc();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC initialization failed!");
        return err;
    }
    
    ESP_LOGI(TAG, "=== All Hardware Drivers Initialized Successfully ===");
    
    return ESP_OK;
}
