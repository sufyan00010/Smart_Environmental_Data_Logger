/**
 * @file dht11.h
 * @brief DHT11 Temperature and Humidity Sensor Driver
 * 
 * Simple driver for DHT11 sensor using single-wire protocol
 */

#ifndef DHT11_H
#define DHT11_H

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize DHT11 sensor
 * 
 * Configures the GPIO pin for DHT11 data communication.
 * Must be called before any read operations.
 * 
 * @param gpio_num GPIO pin number connected to DHT11 data pin
 * @return 
 *     - ESP_OK on success
 *     - ESP_FAIL on GPIO configuration failure
 */
esp_err_t dht11_init(gpio_num_t gpio_num);

/**
 * @brief Read temperature and humidity from DHT11
 * 
 * Reads both temperature and humidity values from the sensor.
 * Reading takes approximately 18-20ms to complete.
 * 
 * @note Wait at least 2 seconds between consecutive reads
 * @note This function disables interrupts briefly for timing accuracy
 * 
 * @param[out] temperature Pointer to store temperature value in Celsius (0-50°C)
 * @param[out] humidity Pointer to store relative humidity percentage (20-90%)
 * 
 * @return 
 *     - ESP_OK on successful read
 *     - ESP_FAIL on communication error or checksum mismatch
 * 
 * @code
 * float temp, hum;
 * esp_err_t ret = dht11_read(&temp, &hum);
 * if (ret == ESP_OK) {
 *     printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temp, hum);
 * }
 * @endcode
 */
esp_err_t dht11_read(float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif

#endif // DHT11_H
