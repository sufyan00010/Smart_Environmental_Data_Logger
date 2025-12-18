/**
 * @file ssd1306.c
 * @brief SSD1306 OLED Display Driver Implementation
 */

#include "ssd1306.h"
#include "font8x8_basic.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"

static const char *TAG = "SSD1306";

// SSD1306 Commands
#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_SET_SEGMENT_REMAP   0xA1
#define SSD1306_CMD_SET_COM_SCAN_DEC    0xC8
#define SSD1306_CMD_SET_MULTIPLEX       0xA8
#define SSD1306_CMD_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_CMD_SET_START_LINE      0x40
#define SSD1306_CMD_CHARGE_PUMP         0x8D
#define SSD1306_CMD_SET_COM_PINS        0xDA
#define SSD1306_CMD_SET_PRECHARGE       0xD9
#define SSD1306_CMD_SET_VCOMH           0xDB
#define SSD1306_CMD_NORMAL_DISPLAY      0xA6
#define SSD1306_CMD_SET_MEMORY_MODE     0x20
#define SSD1306_CMD_SET_COLUMN_ADDR     0x21
#define SSD1306_CMD_SET_PAGE_ADDR       0x22

typedef struct {
    i2c_port_t i2c_port;
    uint8_t dev_addr;
    uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
} ssd1306_dev_t;

static esp_err_t ssd1306_write_cmd(ssd1306_dev_t *dev, uint8_t cmd)
{
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (dev->dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, 0x00, true);  // Command mode
    i2c_master_write_byte(i2c_cmd, cmd, true);
    i2c_master_stop(i2c_cmd);
    esp_err_t ret = i2c_master_cmd_begin(dev->i2c_port, i2c_cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(i2c_cmd);
    return ret;
}

ssd1306_handle_t ssd1306_create(i2c_port_t i2c_port, uint8_t dev_addr)
{
    ssd1306_dev_t *dev = (ssd1306_dev_t *)calloc(1, sizeof(ssd1306_dev_t));
    if (dev == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for SSD1306 device");
        return NULL;
    }
    
    dev->i2c_port = i2c_port;
    dev->dev_addr = dev_addr;
    memset(dev->buffer, 0, sizeof(dev->buffer));
    
    return (ssd1306_handle_t)dev;
}

esp_err_t ssd1306_init(ssd1306_handle_t handle)
{
    ssd1306_dev_t *dev = (ssd1306_dev_t *)handle;
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Initialization sequence
    ssd1306_write_cmd(dev, SSD1306_CMD_DISPLAY_OFF);
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_MULTIPLEX);
    ssd1306_write_cmd(dev, 0x3F);  // 1/64 duty
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_DISPLAY_OFFSET);
    ssd1306_write_cmd(dev, 0x00);
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_START_LINE | 0x00);
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_SEGMENT_REMAP);
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_COM_SCAN_DEC);
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_COM_PINS);
    ssd1306_write_cmd(dev, 0x12);
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_CONTRAST);
    ssd1306_write_cmd(dev, 0x7F);
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_PRECHARGE);
    ssd1306_write_cmd(dev, 0xF1);
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_VCOMH);
    ssd1306_write_cmd(dev, 0x40);
    ssd1306_write_cmd(dev, SSD1306_CMD_NORMAL_DISPLAY);
    ssd1306_write_cmd(dev, SSD1306_CMD_CHARGE_PUMP);
    ssd1306_write_cmd(dev, 0x14);  // Enable charge pump
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_MEMORY_MODE);
    ssd1306_write_cmd(dev, 0x00);  // Horizontal addressing mode
    ssd1306_write_cmd(dev, SSD1306_CMD_DISPLAY_ON);
    
    ESP_LOGI(TAG, "SSD1306 initialized successfully");
    return ESP_OK;
}

void ssd1306_clear_screen(ssd1306_handle_t handle, uint8_t color)
{
    ssd1306_dev_t *dev = (ssd1306_dev_t *)handle;
    if (dev == NULL) return;
    
    memset(dev->buffer, color ? 0xFF : 0x00, sizeof(dev->buffer));
}

esp_err_t ssd1306_refresh_gram(ssd1306_handle_t handle)
{
    ssd1306_dev_t *dev = (ssd1306_dev_t *)handle;
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Set column address
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_COLUMN_ADDR);
    ssd1306_write_cmd(dev, 0);     // Start column
    ssd1306_write_cmd(dev, 127);   // End column
    
    // Set page address
    ssd1306_write_cmd(dev, SSD1306_CMD_SET_PAGE_ADDR);
    ssd1306_write_cmd(dev, 0);     // Start page
    ssd1306_write_cmd(dev, 7);     // End page
    
    // Send display buffer
    i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
    i2c_master_start(i2c_cmd);
    i2c_master_write_byte(i2c_cmd, (dev->dev_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(i2c_cmd, 0x40, true);  // Data mode
    i2c_master_write(i2c_cmd, dev->buffer, sizeof(dev->buffer), true);
    i2c_master_stop(i2c_cmd);
    esp_err_t ret = i2c_master_cmd_begin(dev->i2c_port, i2c_cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(i2c_cmd);
    
    return ret;
}

void ssd1306_draw_pixel(ssd1306_handle_t handle, uint8_t x, uint8_t y, uint8_t color)
{
    ssd1306_dev_t *dev = (ssd1306_dev_t *)handle;
    if (dev == NULL || x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    
    uint16_t index = x + (y / 8) * SSD1306_WIDTH;
    uint8_t bit = y % 8;
    
    if (color) {
        dev->buffer[index] |= (1 << bit);
    } else {
        dev->buffer[index] &= ~(1 << bit);
    }
}

void ssd1306_draw_string(ssd1306_handle_t handle, uint8_t x, uint8_t y, 
                         const uint8_t *text, uint8_t size, uint8_t mode)
{
    ssd1306_dev_t *dev = (ssd1306_dev_t *)handle;
    if (dev == NULL) return;
    
    uint8_t char_x = x;
    
    while (*text) {
        if (char_x + 8 > SSD1306_WIDTH) break;
        
        // Draw 8x8 character
        for (int i = 0; i < 8; i++) {
            uint8_t line = font8x8_basic[*text][i];
            for (int j = 0; j < 8; j++) {
                if (line & (1 << j)) {
                    ssd1306_draw_pixel(handle, char_x + i, y + j, mode);
                } else {
                    ssd1306_draw_pixel(handle, char_x + i, y + j, !mode);
                }
            }
        }
        
        char_x += 8;
        text++;
    }
}

void ssd1306_delete(ssd1306_handle_t handle)
{
    if (handle != NULL) {
        free(handle);
    }
}
