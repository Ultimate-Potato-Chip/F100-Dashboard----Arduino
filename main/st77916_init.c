/**
 * ST77916 Display Initialization Library
 * Contains the manufacturer's complete 193-command initialization sequence
 * Source: lcd_init.c from manufacturer's provided source code
 */

#include "st77916_init.h"
#include "esp_lcd_st77916.h"
#include "esp_log.h"

static const char *TAG = "ST77916_INIT";

// Manufacturer's initialization data arrays
static const uint8_t init_data_0x28[] = {0x28};
static const uint8_t init_data_0xF0[] = {0xF0};
static const uint8_t init_data_0xD1[] = {0xD1};
static const uint8_t init_data_0xE0[] = {0xE0};
static const uint8_t init_data_0x61[] = {0x61};
static const uint8_t init_data_0x82[] = {0x82};
static const uint8_t init_data_0x00[] = {0x00};
static const uint8_t init_data_0x10[] = {0x10};
static const uint8_t init_data_0x01[] = {0x01};
static const uint8_t init_data_0x08[] = {0x08};
static const uint8_t init_data_0x07[] = {0x07};
static const uint8_t init_data_0xFF[] = {0xFF};
static const uint8_t init_data_0x91[] = {0x91};
static const uint8_t init_data_0x68[] = {0x68};
static const uint8_t init_data_F5[] = {0x00, 0xA5};
static const uint8_t init_data_0x02[] = {0x02};
static const uint8_t init_data_gamma_pos[] = {0xF0, 0x10, 0x18, 0x0D, 0x0C, 0x38, 0x3E, 0x44, 0x51, 0x39, 0x15, 0x15, 0x30, 0x34};
static const uint8_t init_data_gamma_neg[] = {0xF0, 0x0F, 0x17, 0x0D, 0x0B, 0x07, 0x3E, 0x33, 0x51, 0x39, 0x15, 0x15, 0x30, 0x34};
static const uint8_t init_data_0x05[] = {0x05};
static const uint8_t init_data_0x06[] = {0x06};
static const uint8_t init_data_0x21[] = {0x21};
static const uint8_t init_data_0x03[] = {0x03};
static const uint8_t init_data_0xE9[] = {0xE9};
static const uint8_t init_data_0x14[] = {0x14};
static const uint8_t init_data_0x30[] = {0x30};
static const uint8_t init_data_0x40[] = {0x40};
static const uint8_t init_data_0x42[] = {0x42};
static const uint8_t init_data_0xDA[] = {0xDA};
static const uint8_t init_data_0x04[] = {0x04};
static const uint8_t init_data_0xD9[] = {0xD9};
static const uint8_t init_data_0x48[] = {0x48};
static const uint8_t init_data_0xD7[] = {0xD7};
static const uint8_t init_data_0x09[] = {0x09};
static const uint8_t init_data_0x0B[] = {0x0B};
static const uint8_t init_data_0xDB[] = {0xDB};
static const uint8_t init_data_0x0D[] = {0x0D};
static const uint8_t init_data_0xDD[] = {0xDD};
static const uint8_t init_data_0xD6[] = {0xD6};
static const uint8_t init_data_0xD8[] = {0xD8};
static const uint8_t init_data_0x0A[] = {0x0A};
static const uint8_t init_data_0x0C[] = {0x0C};
static const uint8_t init_data_0xDC[] = {0xDC};
static const uint8_t init_data_0x47[] = {0x47};
static const uint8_t init_data_0x56[] = {0x56};
static const uint8_t init_data_0x65[] = {0x65};
static const uint8_t init_data_0x74[] = {0x74};
static const uint8_t init_data_0x88[] = {0x88};
static const uint8_t init_data_0x99[] = {0x99};
static const uint8_t init_data_0xBB[] = {0xBB};
static const uint8_t init_data_0xAA[] = {0xAA};
static const uint8_t init_data_MADCTL_BGR[] = {0x08};  // Bit 3 = 1 for BGR order

// Complete manufacturer's initialization command sequence (193 commands)
static const st77916_lcd_init_cmd_t manufacturer_init_cmds[] = {
    // Vendor-specific initialization (lines 17-64 from lcd_init.c)
    {0xF0, init_data_0x28, 1, 0},
    {0xF2, init_data_0x28, 1, 0},
    {0x73, init_data_0xF0, 1, 0},
    {0x7C, init_data_0xD1, 1, 0},
    {0x83, init_data_0xE0, 1, 0},
    {0x84, init_data_0x61, 1, 0},
    {0xF2, init_data_0x82, 1, 0},
    {0xF0, init_data_0x00, 1, 0},

    // Power control (lines 47-55 from lcd_init.c)
    {0xC0, init_data_0xFF, 1, 0},
    {0xD0, init_data_0x91, 1, 0},
    {0xD1, init_data_0x68, 1, 0},
    {0xD2, init_data_0x68, 1, 0},
    {0xF5, init_data_F5, 2, 0},
    {0xF1, init_data_0x10, 1, 0},
    {0xF0, init_data_0x00, 1, 0},
    {0xF0, init_data_0x02, 1, 0},

    // Gamma curves (lines 65-66 from lcd_init.c)
    {0xE0, init_data_gamma_pos, 14, 0},
    {0xE1, init_data_gamma_neg, 14, 0},

    // Switch to command set 0x10 for extended timing and driver config
    {0xF0, init_data_0x10, 1, 0},
    {0xF3, init_data_0x10, 1, 0},

    // Extended timing parameters (0xE0-0xFF)
    {0xE0, init_data_0x08, 1, 0},
    {0xE1, init_data_0x00, 1, 0},
    {0xE2, init_data_0x00, 1, 0},
    {0xE3, init_data_0x00, 1, 0},
    {0xE4, init_data_0xE0, 1, 0},
    {0xE5, init_data_0x06, 1, 0},
    {0xE6, init_data_0x21, 1, 0},
    {0xE7, init_data_0x03, 1, 0},
    {0xE8, init_data_0x05, 1, 0},
    {0xE9, init_data_0x02, 1, 0},
    {0xEA, init_data_0xE9, 1, 0},
    {0xEB, init_data_0x00, 1, 0},
    {0xEC, init_data_0x00, 1, 0},
    {0xED, init_data_0x14, 1, 0},
    {0xEE, init_data_0xFF, 1, 0},
    {0xEF, init_data_0x00, 1, 0},
    {0xF8, init_data_0xFF, 1, 0},
    {0xF9, init_data_0x00, 1, 0},
    {0xFA, init_data_0x00, 1, 0},
    {0xFB, init_data_0x30, 1, 0},
    {0xFC, init_data_0x00, 1, 0},
    {0xFD, init_data_0x00, 1, 0},
    {0xFE, init_data_0x00, 1, 0},
    {0xFF, init_data_0x00, 1, 0},

    // Gate driver configuration (0x60-0x6B)
    {0x60, init_data_0x40, 1, 0},
    {0x61, init_data_0x05, 1, 0},
    {0x62, init_data_0x00, 1, 0},
    {0x63, init_data_0x42, 1, 0},
    {0x64, init_data_0xDA, 1, 0},
    {0x65, init_data_0x00, 1, 0},
    {0x66, init_data_0x00, 1, 0},
    {0x67, init_data_0x00, 1, 0},
    {0x68, init_data_0x00, 1, 0},
    {0x69, init_data_0x00, 1, 0},
    {0x6A, init_data_0x00, 1, 0},
    {0x6B, init_data_0x00, 1, 0},

    // More gate driver configuration (0x70-0x7B)
    {0x70, init_data_0x40, 1, 0},
    {0x71, init_data_0x04, 1, 0},
    {0x72, init_data_0x00, 1, 0},
    {0x73, init_data_0x42, 1, 0},
    {0x74, init_data_0xD9, 1, 0},
    {0x75, init_data_0x00, 1, 0},
    {0x76, init_data_0x00, 1, 0},
    {0x77, init_data_0x00, 1, 0},
    {0x78, init_data_0x00, 1, 0},
    {0x79, init_data_0x00, 1, 0},
    {0x7A, init_data_0x00, 1, 0},
    {0x7B, init_data_0x00, 1, 0},

    // Source driver configuration (0x80-0xBF)
    {0x80, init_data_0x48, 1, 0},
    {0x81, init_data_0x00, 1, 0},
    {0x82, init_data_0x07, 1, 0},
    {0x83, init_data_0x02, 1, 0},
    {0x84, init_data_0xD7, 1, 0},
    {0x85, init_data_0x04, 1, 0},
    {0x86, init_data_0x00, 1, 0},
    {0x87, init_data_0x00, 1, 0},
    {0x88, init_data_0x48, 1, 0},
    {0x89, init_data_0x00, 1, 0},
    {0x8A, init_data_0x09, 1, 0},
    {0x8B, init_data_0x02, 1, 0},
    {0x8C, init_data_0xD9, 1, 0},
    {0x8D, init_data_0x04, 1, 0},
    {0x8E, init_data_0x00, 1, 0},
    {0x8F, init_data_0x00, 1, 0},
    {0x90, init_data_0x48, 1, 0},
    {0x91, init_data_0x00, 1, 0},
    {0x92, init_data_0x0B, 1, 0},
    {0x93, init_data_0x02, 1, 0},
    {0x94, init_data_0xDB, 1, 0},
    {0x95, init_data_0x04, 1, 0},
    {0x96, init_data_0x00, 1, 0},
    {0x97, init_data_0x00, 1, 0},
    {0x98, init_data_0x48, 1, 0},
    {0x99, init_data_0x00, 1, 0},
    {0x9A, init_data_0x0D, 1, 0},
    {0x9B, init_data_0x02, 1, 0},
    {0x9C, init_data_0xDD, 1, 0},
    {0x9D, init_data_0x04, 1, 0},
    {0x9E, init_data_0x00, 1, 0},
    {0x9F, init_data_0x00, 1, 0},
    {0xA0, init_data_0x48, 1, 0},
    {0xA1, init_data_0x00, 1, 0},
    {0xA2, init_data_0x06, 1, 0},
    {0xA3, init_data_0x02, 1, 0},
    {0xA4, init_data_0xD6, 1, 0},
    {0xA5, init_data_0x04, 1, 0},
    {0xA6, init_data_0x00, 1, 0},
    {0xA7, init_data_0x00, 1, 0},
    {0xA8, init_data_0x48, 1, 0},
    {0xA9, init_data_0x00, 1, 0},
    {0xAA, init_data_0x08, 1, 0},
    {0xAB, init_data_0x02, 1, 0},
    {0xAC, init_data_0xD8, 1, 0},
    {0xAD, init_data_0x04, 1, 0},
    {0xAE, init_data_0x00, 1, 0},
    {0xAF, init_data_0x00, 1, 0},
    {0xB0, init_data_0x48, 1, 0},
    {0xB1, init_data_0x00, 1, 0},
    {0xB2, init_data_0x0A, 1, 0},
    {0xB3, init_data_0x02, 1, 0},
    {0xB4, init_data_0xDA, 1, 0},
    {0xB5, init_data_0x04, 1, 0},
    {0xB6, init_data_0x00, 1, 0},
    {0xB7, init_data_0x00, 1, 0},
    {0xB8, init_data_0x48, 1, 0},
    {0xB9, init_data_0x00, 1, 0},
    {0xBA, init_data_0x0C, 1, 0},
    {0xBB, init_data_0x02, 1, 0},
    {0xBC, init_data_0xDC, 1, 0},
    {0xBD, init_data_0x04, 1, 0},
    {0xBE, init_data_0x00, 1, 0},
    {0xBF, init_data_0x00, 1, 0},

    // MUX configuration (0xC0-0xD9)
    {0xC0, init_data_0x10, 1, 0},
    {0xC1, init_data_0x47, 1, 0},
    {0xC2, init_data_0x56, 1, 0},
    {0xC3, init_data_0x65, 1, 0},
    {0xC4, init_data_0x74, 1, 0},
    {0xC5, init_data_0x88, 1, 0},
    {0xC6, init_data_0x99, 1, 0},
    {0xC7, init_data_0x01, 1, 0},
    {0xC8, init_data_0xBB, 1, 0},
    {0xC9, init_data_0xAA, 1, 0},
    {0xD0, init_data_0x10, 1, 0},
    {0xD1, init_data_0x47, 1, 0},
    {0xD2, init_data_0x56, 1, 0},
    {0xD3, init_data_0x65, 1, 0},
    {0xD4, init_data_0x74, 1, 0},
    {0xD5, init_data_0x88, 1, 0},
    {0xD6, init_data_0x99, 1, 0},
    {0xD7, init_data_0x01, 1, 0},
    {0xD8, init_data_0xBB, 1, 0},
    {0xD9, init_data_0xAA, 1, 0},

    // Return to command set 0x00
    {0xF3, init_data_0x01, 1, 0},
    {0xF0, init_data_0x00, 1, 0},

    // Final display setup (lines 203-218 from lcd_init.c)
    {0x36, init_data_MADCTL_BGR, 1, 0},  // MADCTL - bit 3 = 1 for BGR order
    {0x3A, init_data_0x05, 1, 0},  // COLMOD - 16-bit color (RGB565)
    {0x35, init_data_0x00, 1, 0},  // Tearing effect line ON
    {0x21, NULL, 0, 0},             // Display Inversion ON
    {0x11, NULL, 0, 120},           // Sleep Out - must wait 120ms
    {0x29, NULL, 0, 0},             // Display ON
    {0x2C, NULL, 0, 0},             // Memory Write
};

esp_err_t st77916_init_panel(esp_lcd_panel_io_handle_t io_handle,
                              gpio_num_t rst_gpio,
                              esp_lcd_panel_handle_t *out_panel)
{
    if (!io_handle || !out_panel) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing ST77916 with %d commands from manufacturer",
             sizeof(manufacturer_init_cmds) / sizeof(st77916_lcd_init_cmd_t));

    // ST77916 vendor configuration with manufacturer's init sequence
    st77916_vendor_config_t vendor_config = {
        .init_cmds = manufacturer_init_cmds,
        .init_cmds_size = sizeof(manufacturer_init_cmds) / sizeof(st77916_lcd_init_cmd_t),
        .flags = {
            .use_qspi_interface = 1,  // Enable QSPI mode
        },
    };

    // Panel configuration
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = rst_gpio,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,  // MADCTL controls actual order
        .bits_per_pixel = 16,
        .vendor_config = &vendor_config,
    };

    // Create ST77916 panel
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_err_t ret = esp_lcd_new_panel_st77916(io_handle, &panel_config, &panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ST77916 panel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Reset and init panel
    ESP_LOGI(TAG, "Resetting panel...");
    ret = esp_lcd_panel_reset(panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel reset failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Initializing panel with manufacturer's sequence...");
    ret = esp_lcd_panel_init(panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Setting display options...");
    ret = esp_lcd_panel_mirror(panel_handle, false, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel mirror failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_lcd_panel_swap_xy(panel_handle, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel swap_xy failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Turning display on...");
    ret = esp_lcd_panel_disp_on_off(panel_handle, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display on failed: %s", esp_err_to_name(ret));
        return ret;
    }

    *out_panel = panel_handle;
    ESP_LOGI(TAG, "ST77916 initialization complete!");
    return ESP_OK;
}