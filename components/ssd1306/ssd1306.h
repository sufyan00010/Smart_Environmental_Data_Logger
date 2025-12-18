/**
 * @file ssd1306.h
 * @brief SSD1306 OLED Display Driver (128x64 I2C)
 * 
 * Driver for monochrome OLED displays using SSD1306 controller
 */

#ifndef SSD1306_H
#define SSD1306_H

#include "driver/i2c.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// SSD1306 I2C Address (7-bit)
#define SSD1306_I2C_ADDRESS 0x3C    // Common address (can also be 0x3D)

// Display dimensions
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64

// Opaque handle to SSD1306 device
typedef void* ssd1306_handle_t;

/**
 * @brief Create SSD1306 device handle
 * 
 * Allocates memory for device structure but does not initialize hardware.
 * Must call ssd1306_init() after this.
 * 
 * @param i2c_port I2C port number (I2C_NUM_0 or I2C_NUM_1)
 * @param dev_addr I2C device address (typically 0x3C or 0x3D)
 * 
 * @return 
 *     - Valid handle on success
 *     - NULL on memory allocation failure
 * 
 * @note Remember to call ssd1306_delete() when done to free memory
 */
ssd1306_handle_t ssd1306_create(i2c_port_t i2c_port, uint8_t dev_addr);

/**
 * @brief Initialize SSD1306 display
 * 
 * Sends initialization sequence to configure display settings.
 * Must be called after ssd1306_create() and I2C bus initialization.
 * 
 * @param dev Device handle from ssd1306_create()
 * 
 * @return 
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if dev is NULL
 *     - ESP_FAIL on I2C communication error
 */
esp_err_t ssd1306_init(ssd1306_handle_t dev);

/**
 * @brief Clear entire display buffer
 * 
 * Fills the internal buffer with specified color.
 * Must call ssd1306_refresh_gram() to update physical display.
 * 
 * @param dev Device handle
 * @param color Fill color (0 = black/off, non-zero = white/on)
 * 
 * @note This only clears the buffer, not the display itself
 */
void ssd1306_clear_screen(ssd1306_handle_t dev, uint8_t color);

/**
 * @brief Refresh display (send buffer to OLED)
 * 
 * Transfers the internal frame buffer to the physical display.
 * This is when changes become visible.
 * 
 * @param dev Device handle
 * 
 * @return 
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG if dev is NULL
 *     - ESP_FAIL on I2C communication error
 * 
 * @note Takes approximately 20-30ms to complete
 */
esp_err_t ssd1306_refresh_gram(ssd1306_handle_t dev);

/**
 * @brief Draw a string on display
 * 
 * Draws text using built-in 8x8 font.
 * Text wrapping is not automatic - text beyond screen edge is clipped.
 * 
 * @param dev Device handle
 * @param x X coordinate (0-127, left to right)
 * @param y Y coordinate (0-63, top to bottom)
 * @param text Null-terminated ASCII string to display
 * @param size Font size (8, 12, 16, 24) - currently only 8x8 implemented
 * @param mode Display mode:
 *             - 1: White text on black background
 *             - 0: Black text on white background
 * 
 * @note Each character is 8 pixels wide, so max ~16 chars per line
 * @note Must call ssd1306_refresh_gram() to make text visible
 * 
 * @code
 * ssd1306_draw_string(dev, 0, 0, (uint8_t*)"Hello", 16, 1);
 * ssd1306_refresh_gram(dev);
 * @endcode
 */
void ssd1306_draw_string(ssd1306_handle_t dev, uint8_t x, uint8_t y, 
                         const uint8_t *text, uint8_t size, uint8_t mode);

/**
 * @brief Draw a single pixel
 * 
 * Sets or clears a single pixel in the frame buffer.
 * 
 * @param dev Device handle
 * @param x X coordinate (0-127)
 * @param y Y coordinate (0-63)
 * @param color Pixel color (0 = off/black, non-zero = on/white)
 * 
 * @note Coordinates outside bounds are silently ignored
 * @note Must call ssd1306_refresh_gram() to make change visible
 */
void ssd1306_draw_pixel(ssd1306_handle_t dev, uint8_t x, uint8_t y, uint8_t color);

/**
 * @brief Delete SSD1306 device handle and free memory
 * 
 * Should be called when display is no longer needed.
 * Does not de-initialize I2C bus.
 * 
 * @param dev Device handle to delete
 * 
 * @note After calling this, the handle becomes invalid
 */
void ssd1306_delete(ssd1306_handle_t dev);

#ifdef __cplusplus
}
#endif

#endif // SSD1306_H
