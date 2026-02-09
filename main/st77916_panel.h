/**
 * Custom ST77916 QSPI Panel Driver
 *
 * This driver implements the manufacturer's QSPI protocol directly,
 * bypassing the esp_lcd_st77916 component which has color rotation issues.
 */

#ifndef ST77916_PANEL_H
#define ST77916_PANEL_H

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and initialize ST77916 panel with manufacturer's settings
 *
 * @param io_handle LCD panel IO handle (QSPI)
 * @param rst_gpio Reset GPIO pin number
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st77916_panel_init(esp_lcd_panel_io_handle_t io_handle, gpio_num_t rst_gpio);

/**
 * @brief Draw bitmap to display
 *
 * @param io_handle LCD panel IO handle
 * @param x_start Start X coordinate
 * @param y_start Start Y coordinate
 * @param x_end End X coordinate (exclusive)
 * @param y_end End Y coordinate (exclusive)
 * @param color_data Pointer to RGB565 pixel data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st77916_panel_draw_bitmap(esp_lcd_panel_io_handle_t io_handle,
                                     int x_start, int y_start,
                                     int x_end, int y_end,
                                     const void *color_data);

/**
 * @brief Draw bitmap using manufacturer's command format (0x3C RAMWRC)
 *
 * This matches how the STM32 manufacturer code sends pixel data:
 * 1. Set address window with CASET/RASET
 * 2. Send RAMWR (0x2C)
 * 3. Send pixel data with RAMWRC (0x3C)
 *
 * @param io_handle LCD panel IO handle
 * @param x_start Start X coordinate
 * @param y_start Start Y coordinate
 * @param x_end End X coordinate (exclusive)
 * @param y_end End Y coordinate (exclusive)
 * @param color_data Pointer to RGB565 pixel data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st77916_panel_draw_bitmap_mfr(esp_lcd_panel_io_handle_t io_handle,
                                         int x_start, int y_start,
                                         int x_end, int y_end,
                                         const void *color_data);

#ifdef __cplusplus
}
#endif

#endif /* ST77916_PANEL_H */