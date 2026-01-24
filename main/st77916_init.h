/**
 * ST77916 Display Initialization Library
 * Contains the manufacturer's complete 193-command initialization sequence
 */

#ifndef ST77916_INIT_H
#define ST77916_INIT_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize ST77916 display panel with manufacturer's sequence
 *
 * @param io_handle LCD panel IO handle (already configured for QSPI/SPI)
 * @param rst_gpio Reset GPIO pin number
 * @param out_panel Output parameter for the created panel handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t st77916_init_panel(esp_lcd_panel_io_handle_t io_handle,
                              gpio_num_t rst_gpio,
                              esp_lcd_panel_handle_t *out_panel);

#ifdef __cplusplus
}
#endif

#endif /* ST77916_INIT_H */