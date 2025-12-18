/**
 * @file sensor_task.h
 * @brief Sensor reading task interface
 */

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include <stdint.h>

/**
 * @brief Sensor data structure shared between tasks
 */
typedef struct {
    float temperature;      // Temperature in Celsius
    float humidity;         // Humidity in percentage
    int aqi;               // Air Quality Index (0-500)
    uint32_t timestamp;    // Timestamp in milliseconds
} sensor_data_t;

/**
 * @brief Initialize sensor hardware (DHT11, LDR, ADC)
 */
void sensor_init(void);

/**
 * @brief Main sensor task function
 * 
 * Periodically reads DHT11 and LDR sensors, calculates AQI,
 * and sends data to queue for other tasks to consume.
 * 
 * @param pvParameters Task parameters (unused)
 */
void sensor_task(void *pvParameters);

#endif // SENSOR_TASK_H
