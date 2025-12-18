/**
 * @file ota_task.h
 * @brief OTA update monitoring task interface
 */

#ifndef OTA_TASK_H
#define OTA_TASK_H

/**
 * @brief Main OTA monitoring task function
 * 
 * Monitors OTA update status and handles firmware update events
 * 
 * @param pvParameters Task parameters (unused)
 */
void ota_task(void *pvParameters);

#endif // OTA_TASK_H
