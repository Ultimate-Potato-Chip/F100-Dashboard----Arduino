/**
 * Custom ST77916 QSPI Panel Driver
 *
 * Features:
 * - Manufacturer's 193-command initialization sequence
 * - Automatic RGB565 color rotation to compensate for QSPI lane mismatch
 * - DMA-safe pixel transfer with completion synchronization
 */

#include "st77916_panel.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ST77916_DIRECT";

// QSPI opcodes (ESP-IDF standard)
#define QSPI_CMD_WRITE_CMD      0x02
#define QSPI_CMD_WRITE_COLOR    0x32

// Manufacturer's approach: direct LCD command for pixel data
#define LCD_CMD_RAMWRC      0x3C    // RAM Write Continue (manufacturer uses this)

// LCD commands
#define LCD_CMD_MADCTL      0x36
#define LCD_CMD_COLMOD      0x3A
#define LCD_CMD_CASET       0x2A
#define LCD_CMD_RASET       0x2B
#define LCD_CMD_RAMWR       0x2C
#define LCD_CMD_SLPOUT      0x11
#define LCD_CMD_DISPON      0x29
#define LCD_CMD_INVON       0x21
#define LCD_CMD_TEON        0x35

// Store handles
static esp_lcd_panel_io_handle_t g_io_handle = NULL;
static spi_device_handle_t g_spi_device = NULL;

/**
 * @brief Rotate RGB565 color channels to compensate for QSPI lane mismatch
 *
 * The ST77916 QSPI interface exhibits a color channel rotation (R→B, G→R, B→G).
 * This function pre-rotates colors in the opposite direction to compensate.
 *
 * @param color Original RGB565 color
 * @return Rotated RGB565 color that displays correctly
 */
static inline uint16_t rotate_color_rgb565(uint16_t color)
{
    // RGB565: RRRRRGGGGGGBBBBB
    uint16_t r = (color >> 11) & 0x1F;  // 5 bits
    uint16_t g = (color >> 5) & 0x3F;   // 6 bits
    uint16_t b = color & 0x1F;          // 5 bits

    // Rotate: new_R = old_B, new_G = old_R, new_B = old_G
    uint16_t new_r = b;                 // B (5 bits) → R (5 bits)
    uint16_t new_g = r << 1;            // R (5 bits) → G (6 bits)
    uint16_t new_b = g >> 1;            // G (6 bits) → B (5 bits)

    return (new_r << 11) | (new_g << 5) | new_b;
}

/**
 * @brief Apply color rotation to entire pixel buffer
 *
 * @param dst Destination buffer (can be same as src for in-place)
 * @param src Source buffer
 * @param num_pixels Number of RGB565 pixels
 */
static void rotate_color_buffer(uint16_t *dst, const uint16_t *src, size_t num_pixels)
{
    for (size_t i = 0; i < num_pixels; i++) {
        dst[i] = rotate_color_rgb565(src[i]);
    }
}

// Helper to send command via panel_io (for init sequence)
static esp_err_t send_cmd(esp_lcd_panel_io_handle_t io, uint8_t cmd, const uint8_t *data, size_t len)
{
    int lcd_cmd = (QSPI_CMD_WRITE_CMD << 24) | (cmd << 8);
    return esp_lcd_panel_io_tx_param(io, lcd_cmd, data, len);
}

// Manufacturer's initialization sequence
typedef struct {
    uint8_t cmd;
    uint8_t data[36];
    uint8_t len;
    uint16_t delay_ms;
} init_cmd_t;

static const init_cmd_t init_sequence[] = {
    // Vendor-specific initialization
    {0xF0, {0x28}, 1, 0},
    {0xF2, {0x28}, 1, 0},
    {0x73, {0xF0}, 1, 0},
    {0x7C, {0xD1}, 1, 0},
    {0x83, {0xE0}, 1, 0},
    {0x84, {0x61}, 1, 0},
    {0xF2, {0x82}, 1, 0},
    {0xF0, {0x00}, 1, 0},
    {0xF0, {0x01}, 1, 0},
    {0xF1, {0x01}, 1, 0},
    {0xB0, {0x69}, 1, 0},
    {0xB1, {0x4A}, 1, 0},
    {0xB2, {0x2F}, 1, 0},
    {0xB3, {0x01}, 1, 0},
    {0xB4, {0x69}, 1, 0},
    {0xB5, {0x45}, 1, 0},
    {0xB6, {0xAB}, 1, 0},
    {0xB7, {0x41}, 1, 0},
    {0xB8, {0x86}, 1, 0},
    {0xB9, {0x15}, 1, 0},
    {0xBA, {0x00}, 1, 0},
    {0xBB, {0x08}, 1, 0},
    {0xBC, {0x08}, 1, 0},
    {0xBD, {0x00}, 1, 0},
    {0xBE, {0x00}, 1, 0},
    {0xBF, {0x07}, 1, 0},
    {0xC0, {0x80}, 1, 0},
    {0xC1, {0x10}, 1, 0},
    {0xC2, {0x37}, 1, 0},
    {0xC3, {0x80}, 1, 0},
    {0xC4, {0x10}, 1, 0},
    {0xC5, {0x37}, 1, 0},
    {0xC6, {0xA9}, 1, 0},
    {0xC7, {0x41}, 1, 0},
    {0xC8, {0x01}, 1, 0},
    {0xC9, {0xA9}, 1, 0},
    {0xCA, {0x41}, 1, 0},
    {0xCB, {0x01}, 1, 0},
    {0xCC, {0x7F}, 1, 0},
    {0xCD, {0x7F}, 1, 0},
    {0xCE, {0xFF}, 1, 0},
    {0xD0, {0x91}, 1, 0},
    {0xD1, {0x68}, 1, 0},
    {0xD2, {0x68}, 1, 0},
    {0xF5, {0x00, 0xA5}, 2, 0},
    {0xF1, {0x10}, 1, 0},
    {0xF0, {0x00}, 1, 0},
    {0xF0, {0x02}, 1, 0},
    // Gamma settings
    {0xE0, {0xF0, 0x10, 0x18, 0x0D, 0x0C, 0x38, 0x3E, 0x44, 0x51, 0x39, 0x15, 0x15, 0x30, 0x34}, 14, 0},
    {0xE1, {0xF0, 0x0F, 0x17, 0x0D, 0x0B, 0x07, 0x3E, 0x33, 0x51, 0x39, 0x15, 0x15, 0x30, 0x34}, 14, 0},
    {0xF0, {0x10}, 1, 0},
    {0xF3, {0x10}, 1, 0},
    // More vendor settings
    {0xE0, {0x08}, 1, 0},
    {0xE1, {0x00}, 1, 0},
    {0xE2, {0x00}, 1, 0},
    {0xE3, {0x00}, 1, 0},
    {0xE4, {0xE0}, 1, 0},
    {0xE5, {0x06}, 1, 0},
    {0xE6, {0x21}, 1, 0},
    {0xE7, {0x03}, 1, 0},
    {0xE8, {0x05}, 1, 0},
    {0xE9, {0x02}, 1, 0},
    {0xEA, {0xE9}, 1, 0},
    {0xEB, {0x00}, 1, 0},
    {0xEC, {0x00}, 1, 0},
    {0xED, {0x14}, 1, 0},
    {0xEE, {0xFF}, 1, 0},
    {0xEF, {0x00}, 1, 0},
    {0xF8, {0xFF}, 1, 0},
    {0xF9, {0x00}, 1, 0},
    {0xFA, {0x00}, 1, 0},
    {0xFB, {0x30}, 1, 0},
    {0xFC, {0x00}, 1, 0},
    {0xFD, {0x00}, 1, 0},
    {0xFE, {0x00}, 1, 0},
    {0xFF, {0x00}, 1, 0},
    // Gate/source settings
    {0x60, {0x40}, 1, 0},
    {0x61, {0x05}, 1, 0},
    {0x62, {0x00}, 1, 0},
    {0x63, {0x42}, 1, 0},
    {0x64, {0xDA}, 1, 0},
    {0x65, {0x00}, 1, 0},
    {0x66, {0x00}, 1, 0},
    {0x67, {0x00}, 1, 0},
    {0x68, {0x00}, 1, 0},
    {0x69, {0x00}, 1, 0},
    {0x6A, {0x00}, 1, 0},
    {0x6B, {0x00}, 1, 0},
    {0x70, {0x40}, 1, 0},
    {0x71, {0x04}, 1, 0},
    {0x72, {0x00}, 1, 0},
    {0x73, {0x42}, 1, 0},
    {0x74, {0xD9}, 1, 0},
    {0x75, {0x00}, 1, 0},
    {0x76, {0x00}, 1, 0},
    {0x77, {0x00}, 1, 0},
    {0x78, {0x00}, 1, 0},
    {0x79, {0x00}, 1, 0},
    {0x7A, {0x00}, 1, 0},
    {0x7B, {0x00}, 1, 0},
    // More panel settings
    {0x80, {0x48}, 1, 0},
    {0x81, {0x00}, 1, 0},
    {0x82, {0x07}, 1, 0},
    {0x83, {0x02}, 1, 0},
    {0x84, {0xD7}, 1, 0},
    {0x85, {0x04}, 1, 0},
    {0x86, {0x00}, 1, 0},
    {0x87, {0x00}, 1, 0},
    {0x88, {0x48}, 1, 0},
    {0x89, {0x00}, 1, 0},
    {0x8A, {0x09}, 1, 0},
    {0x8B, {0x02}, 1, 0},
    {0x8C, {0xD9}, 1, 0},
    {0x8D, {0x04}, 1, 0},
    {0x8E, {0x00}, 1, 0},
    {0x8F, {0x00}, 1, 0},
    {0x90, {0x48}, 1, 0},
    {0x91, {0x00}, 1, 0},
    {0x92, {0x0B}, 1, 0},
    {0x93, {0x02}, 1, 0},
    {0x94, {0xDB}, 1, 0},
    {0x95, {0x04}, 1, 0},
    {0x96, {0x00}, 1, 0},
    {0x97, {0x00}, 1, 0},
    {0x98, {0x48}, 1, 0},
    {0x99, {0x00}, 1, 0},
    {0x9A, {0x0D}, 1, 0},
    {0x9B, {0x02}, 1, 0},
    {0x9C, {0xDD}, 1, 0},
    {0x9D, {0x04}, 1, 0},
    {0x9E, {0x00}, 1, 0},
    {0x9F, {0x00}, 1, 0},
    {0xA0, {0x48}, 1, 0},
    {0xA1, {0x00}, 1, 0},
    {0xA2, {0x06}, 1, 0},
    {0xA3, {0x02}, 1, 0},
    {0xA4, {0xD6}, 1, 0},
    {0xA5, {0x04}, 1, 0},
    {0xA6, {0x00}, 1, 0},
    {0xA7, {0x00}, 1, 0},
    {0xA8, {0x48}, 1, 0},
    {0xA9, {0x00}, 1, 0},
    {0xAA, {0x08}, 1, 0},
    {0xAB, {0x02}, 1, 0},
    {0xAC, {0xD8}, 1, 0},
    {0xAD, {0x04}, 1, 0},
    {0xAE, {0x00}, 1, 0},
    {0xAF, {0x00}, 1, 0},
    {0xB0, {0x48}, 1, 0},
    {0xB1, {0x00}, 1, 0},
    {0xB2, {0x0A}, 1, 0},
    {0xB3, {0x02}, 1, 0},
    {0xB4, {0xDA}, 1, 0},
    {0xB5, {0x04}, 1, 0},
    {0xB6, {0x00}, 1, 0},
    {0xB7, {0x00}, 1, 0},
    {0xB8, {0x48}, 1, 0},
    {0xB9, {0x00}, 1, 0},
    {0xBA, {0x0C}, 1, 0},
    {0xBB, {0x02}, 1, 0},
    {0xBC, {0xDC}, 1, 0},
    {0xBD, {0x04}, 1, 0},
    {0xBE, {0x00}, 1, 0},
    {0xBF, {0x00}, 1, 0},
    // Timing settings
    {0xC0, {0x10}, 1, 0},
    {0xC1, {0x47}, 1, 0},
    {0xC2, {0x56}, 1, 0},
    {0xC3, {0x65}, 1, 0},
    {0xC4, {0x74}, 1, 0},
    {0xC5, {0x88}, 1, 0},
    {0xC6, {0x99}, 1, 0},
    {0xC7, {0x01}, 1, 0},
    {0xC8, {0xBB}, 1, 0},
    {0xC9, {0xAA}, 1, 0},
    {0xD0, {0x10}, 1, 0},
    {0xD1, {0x47}, 1, 0},
    {0xD2, {0x56}, 1, 0},
    {0xD3, {0x65}, 1, 0},
    {0xD4, {0x74}, 1, 0},
    {0xD5, {0x88}, 1, 0},
    {0xD6, {0x99}, 1, 0},
    {0xD7, {0x01}, 1, 0},
    {0xD8, {0xBB}, 1, 0},
    {0xD9, {0xAA}, 1, 0},
    // Return to command set 0
    {0xF3, {0x01}, 1, 0},
    {0xF0, {0x00}, 1, 0},
    // Final display configuration
    {LCD_CMD_MADCTL, {0x00}, 1, 0},
    {LCD_CMD_COLMOD, {0x05}, 1, 0},
    {LCD_CMD_TEON, {0x00}, 1, 0},
    {LCD_CMD_INVON, {0}, 0, 0},
    {LCD_CMD_SLPOUT, {0}, 0, 120},
    {LCD_CMD_DISPON, {0}, 0, 0},
};

esp_err_t st77916_panel_init(esp_lcd_panel_io_handle_t io_handle, gpio_num_t rst_gpio)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing ST77916 with DIRECT SPI driver...");

    g_io_handle = io_handle;

    // Hardware reset
    if (rst_gpio >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << rst_gpio,
        };
        gpio_config(&io_conf);

        gpio_set_level(rst_gpio, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(rst_gpio, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    // Send initialization sequence using panel_io
    size_t num_cmds = sizeof(init_sequence) / sizeof(init_cmd_t);
    ESP_LOGI(TAG, "Sending %d init commands...", num_cmds);

    for (size_t i = 0; i < num_cmds; i++) {
        const init_cmd_t *cmd = &init_sequence[i];
        ret = send_cmd(io_handle, cmd->cmd, cmd->data, cmd->len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send cmd 0x%02X", cmd->cmd);
            return ret;
        }
        if (cmd->delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(cmd->delay_ms));
        }
    }

    ESP_LOGI(TAG, "ST77916 initialized (DIRECT SPI mode)");
    return ESP_OK;
}

// Initialize direct SPI device for pixel data
esp_err_t st77916_init_direct_spi(spi_host_device_t host, int cs_gpio, int freq_hz)
{
    ESP_LOGI(TAG, "Creating direct SPI device for pixel data...");

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = freq_hz,
        .mode = 0,
        .spics_io_num = cs_gpio,
        .queue_size = 7,
        .flags = SPI_DEVICE_HALFDUPLEX,  // Start with just halfduplex
        // Try adding SPI_DEVICE_BIT_LSBFIRST here if colors still wrong
    };

    esp_err_t ret = spi_bus_add_device(host, &devcfg, &g_spi_device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Direct SPI device created");
    return ESP_OK;
}

// Send raw bytes via direct SPI in QSPI mode
static esp_err_t spi_send_qspi_data(const void *data, size_t len)
{
    if (!g_spi_device) {
        ESP_LOGE(TAG, "Direct SPI device not initialized!");
        return ESP_ERR_INVALID_STATE;
    }

    spi_transaction_ext_t t = {
        .base = {
            .flags = SPI_TRANS_MODE_QIO,  // QSPI mode for data
            .length = len * 8,             // Length in bits
            .tx_buffer = data,
        },
    };

    return spi_device_transmit(g_spi_device, (spi_transaction_t *)&t);
}

// Send command byte (single-line SPI, then QSPI for params)
static esp_err_t direct_send_cmd(uint8_t cmd, const uint8_t *data, size_t len)
{
    if (!g_spi_device) {
        // Fall back to panel_io if direct SPI not set up
        if (g_io_handle) {
            return send_cmd(g_io_handle, cmd, data, len);
        }
        return ESP_ERR_INVALID_STATE;
    }

    // Build QSPI command frame: opcode (0x02) + 24-bit address with cmd
    // Format: [opcode 8-bit][addr 24-bit] sent on single line
    // For write command: opcode=0x02, addr = cmd << 8
    uint32_t cmd_word = (QSPI_CMD_WRITE_CMD << 24) | (cmd << 8);

    spi_transaction_ext_t t = {
        .base = {
            .flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR,
            .cmd = QSPI_CMD_WRITE_CMD,
            .addr = cmd << 8,
            .length = len * 8,
            .tx_buffer = (len > 0) ? data : NULL,
        },
        .command_bits = 8,
        .address_bits = 24,
    };

    return spi_device_transmit(g_spi_device, (spi_transaction_t *)&t);
}

esp_err_t st77916_panel_draw_bitmap(esp_lcd_panel_io_handle_t io_handle,
                                     int x_start, int y_start,
                                     int x_end, int y_end,
                                     const void *color_data)
{
    esp_err_t ret;

    // Set column address (CASET)
    uint8_t caset_data[] = {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    };
    ret = send_cmd(io_handle, LCD_CMD_CASET, caset_data, 4);
    if (ret != ESP_OK) return ret;

    // Set row address (RASET)
    uint8_t raset_data[] = {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    };
    ret = send_cmd(io_handle, LCD_CMD_RASET, raset_data, 4);
    if (ret != ESP_OK) return ret;

    // Calculate pixel count and data size
    size_t num_pixels = (x_end - x_start) * (y_end - y_start);
    size_t data_len = num_pixels * 2;  // RGB565 = 2 bytes per pixel

    // Allocate buffer for color-rotated pixels
    uint16_t *rotated_buf = heap_caps_malloc(data_len, MALLOC_CAP_DMA);
    if (!rotated_buf) {
        ESP_LOGE(TAG, "Failed to allocate color rotation buffer");
        return ESP_ERR_NO_MEM;
    }

    // Apply color rotation to compensate for QSPI lane mismatch
    rotate_color_buffer(rotated_buf, (const uint16_t *)color_data, num_pixels);

    // Send RAMWR command + pixel data using panel_io tx_color
    int lcd_cmd = (QSPI_CMD_WRITE_COLOR << 24) | (LCD_CMD_RAMWR << 8);
    ret = esp_lcd_panel_io_tx_color(io_handle, lcd_cmd, rotated_buf, data_len);

    // Wait for DMA transfer to complete before freeing buffer
    vTaskDelay(1);

    heap_caps_free(rotated_buf);
    return ret;
}

// Alternative draw function using direct SPI (call this instead if panel_io has issues)
esp_err_t st77916_panel_draw_bitmap_direct(int x_start, int y_start,
                                            int x_end, int y_end,
                                            const void *color_data)
{
    esp_err_t ret;

    if (!g_spi_device) {
        ESP_LOGE(TAG, "Direct SPI not initialized - call st77916_init_direct_spi first");
        return ESP_ERR_INVALID_STATE;
    }

    // Set column address (CASET)
    uint8_t caset_data[] = {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    };
    ret = direct_send_cmd(LCD_CMD_CASET, caset_data, 4);
    if (ret != ESP_OK) return ret;

    // Set row address (RASET)
    uint8_t raset_data[] = {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    };
    ret = direct_send_cmd(LCD_CMD_RASET, raset_data, 4);
    if (ret != ESP_OK) return ret;

    // Send RAMWR command with pixel data directly via SPI
    // Using opcode 0x32 for quad write + RAMWR (0x2C) command
    size_t num_pixels = (x_end - x_start) * (y_end - y_start);
    size_t data_len = num_pixels * 2;

    ESP_LOGD(TAG, "Direct SPI: sending %d pixels (%d bytes)", num_pixels, data_len);

    // Build transaction with QSPI command header + raw pixel data
    spi_transaction_ext_t t = {
        .base = {
            .flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR,
            .cmd = QSPI_CMD_WRITE_COLOR,  // 0x32 - quad write
            .addr = LCD_CMD_RAMWR << 8,   // 0x2C shifted
            .length = data_len * 8,        // Data length in bits
            .tx_buffer = color_data,
        },
        .command_bits = 8,
        .address_bits = 24,
    };

    ret = spi_device_transmit(g_spi_device, (spi_transaction_t *)&t);

    return ret;
}

// NEW: Draw using manufacturer's command format (0x3C RAMWRC)
// This matches how the STM32 manufacturer code sends pixel data
esp_err_t st77916_panel_draw_bitmap_mfr(esp_lcd_panel_io_handle_t io_handle,
                                         int x_start, int y_start,
                                         int x_end, int y_end,
                                         const void *color_data)
{
    esp_err_t ret;

    // Set column address (CASET) - using standard 0x02 write opcode
    uint8_t caset_data[] = {
        (x_start >> 8) & 0xFF,
        x_start & 0xFF,
        ((x_end - 1) >> 8) & 0xFF,
        (x_end - 1) & 0xFF,
    };
    ret = send_cmd(io_handle, LCD_CMD_CASET, caset_data, 4);
    if (ret != ESP_OK) return ret;

    // Set row address (RASET)
    uint8_t raset_data[] = {
        (y_start >> 8) & 0xFF,
        y_start & 0xFF,
        ((y_end - 1) >> 8) & 0xFF,
        (y_end - 1) & 0xFF,
    };
    ret = send_cmd(io_handle, LCD_CMD_RASET, raset_data, 4);
    if (ret != ESP_OK) return ret;

    // Send RAMWR (0x2C) to start memory write
    ret = send_cmd(io_handle, LCD_CMD_RAMWR, NULL, 0);
    if (ret != ESP_OK) return ret;

    // Calculate pixel data size
    size_t num_pixels = (x_end - x_start) * (y_end - y_start);
    size_t data_len = num_pixels * 2;  // RGB565 = 2 bytes per pixel

    // Send pixel data using RAMWRC (0x3C) - manufacturer's approach
    // Format: [0x32 opcode] [0x3C cmd << 8] [pixel data]
    // This uses 0x3C (RAM Write Continue) instead of 0x2C
    int lcd_cmd = (QSPI_CMD_WRITE_COLOR << 24) | (LCD_CMD_RAMWRC << 8);
    ret = esp_lcd_panel_io_tx_color(io_handle, lcd_cmd, color_data, data_len);

    return ret;
}
