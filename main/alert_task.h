/**
 * @file alert_task.h
 * @brief Alert monitoring task interface
 */

#ifndef ALERT_TASK_H
#define ALERT_TASK_H

#include <stdbool.h>

// Alert configuration structure (shared with app_main)
typedef struct {
    float temp_high;
    float temp_low;
    float humidity_high;
    float humidity_low;
    int aqi_threshold;
    bool buzzer_enabled;
} alert_config_t;

/**
 * @brief Main alert monitoring task
 * 
 * Monitors sensor data for threshold violations and triggers alerts
 * 
 * @param pvParameters Task parameters (unused)
 */
void alert_task(void *pvParameters);

#endif // ALERT_TASK_H
