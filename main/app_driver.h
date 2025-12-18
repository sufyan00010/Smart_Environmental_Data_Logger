/**
 * @file app_driver.h
 * @brief Hardware driver initialization functions
 */

#ifndef APP_DRIVER_H
#define APP_DRIVER_H

#include "esp_err.h"

/**
 * @brief Initialize all hardware drivers
 * 
 * This function initializes:
 * - I2C bus for OLED
 * - GPIO pins for LEDs, buzzer, button
 * - ADC for LDR sensor
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_driver_init(void);

/**
 * @brief Initialize I2C bus for OLED display
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_driver_init_i2c(void);

/**
 * @brief Initialize GPIO pins
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_driver_init_gpio(void);

/**
 * @brief Initialize ADC for LDR sensor
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t app_driver_init_adc(void);

#endif // APP_DRIVER_H
