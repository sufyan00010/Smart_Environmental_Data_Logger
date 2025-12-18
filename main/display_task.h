/**
 * @file display_task.h
 * @brief OLED display task interface
 */

#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

/**
 * @brief Initialize OLED display hardware
 */
void display_init(void);

/**
 * @brief Main display task function
 * 
 * Continuously updates OLED display with current sensor readings
 * and system status.
 * 
 * @param pvParameters Task parameters (unused)
 */
void display_task(void *pvParameters);

#endif // DISPLAY_TASK_H
