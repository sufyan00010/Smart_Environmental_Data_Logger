/**
 * @file cloud_task.h
 * @brief Cloud communication task interface
 */

#ifndef CLOUD_TASK_H
#define CLOUD_TASK_H

/**
 * @brief Main cloud communication task
 * 
 * Receives sensor data from queue and updates RainMaker parameters
 * 
 * @param pvParameters Task parameters (unused)
 */
void cloud_task(void *pvParameters);

/**
 * @brief Event handlers for Wi-Fi and cloud connection status
 */
void cloud_task_wifi_connected(void);
void cloud_task_wifi_disconnected(void);
void cloud_task_cloud_connected(void);
void cloud_task_cloud_disconnected(void);

#endif // CLOUD_TASK_H
