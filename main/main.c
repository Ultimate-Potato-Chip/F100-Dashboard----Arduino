/**
 * ST77916 QSPI Display - Simple Color Test
 * Uses custom st77916_panel driver (bypasses esp_lcd_st77916 component)
 *
 * Pin Configuration:
 * CS    -> GPIO5
 * SCLK  -> GPIO6
 * RST   -> GPIO7
 * D0    -> GPIO8  (MOSI)
 * D1    -> GPIO9  (MISO)
 * D2    -> GPIO10
 * D3    -> GPIO11
 * BL    -> GPIO4
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "st77916_panel.h"  // Custom panel driver (bypasses esp_lcd_st77916 component)

static const char *TAG = "ST77916_COLOR_TEST";

// Pin definitions
#define LCD_HOST        SPI2_HOST
#define PIN_NUM_CS      5
#define PIN_NUM_SCLK    6
#define PIN_NUM_RST     7
#define PIN_NUM_IO0     8
#define PIN_NUM_IO1     9
#define PIN_NUM_IO2     10
#define PIN_NUM_IO3     11
#define PIN_NUM_BL      4

// Display specs
#define LCD_H_RES       360
#define LCD_V_RES       360
#define LCD_PIXEL_CLK   (20 * 1000 * 1000)

// RGB565 color definitions
#define RGB565_BLACK    0x0000
#define RGB565_WHITE    0xFFFF
#define RGB565_RED      0xF800
#define RGB565_GREEN    0x07E0
#define RGB565_BLUE     0x001F
#define RGB565_YELLOW   0xFFE0
#define RGB565_CYAN     0x07FF
#define RGB565_MAGENTA  0xF81F
#define RGB565_ORANGE   0xFD20
#define RGB565_PURPLE   0xF01F  // Purple (high R, no G, full B) - uses value that survives QSPI rotation

// Global io_handle for draw functions
static esp_lcd_panel_io_handle_t g_io_handle = NULL;

// Helper: Fill a rectangle with a solid color
static void fill_rect(int x, int y, int w, int h, uint16_t color)
{
    size_t buf_size = w * h;
    uint16_t *buf = heap_caps_malloc(buf_size * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate buffer for fill_rect");
        return;
    }

    for (size_t i = 0; i < buf_size; i++) {
        buf[i] = color;
    }

    // Panel driver handles color rotation internally
    st77916_panel_draw_bitmap(g_io_handle, x, y, x + w, y + h, buf);

    heap_caps_free(buf);
}

// Helper: Fill entire screen with a color
static void fill_screen(uint16_t color)
{
    // Do it in strips to avoid massive allocation
    int strip_height = 40;
    size_t buf_size = LCD_H_RES * strip_height;
    uint16_t *buf = heap_caps_malloc(buf_size * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate buffer for fill_screen");
        return;
    }

    for (size_t i = 0; i < buf_size; i++) {
        buf[i] = color;
    }

    // Panel driver handles color rotation internally
    for (int y = 0; y < LCD_V_RES; y += strip_height) {
        int h = (y + strip_height > LCD_V_RES) ? (LCD_V_RES - y) : strip_height;
        st77916_panel_draw_bitmap(g_io_handle, 0, y, LCD_H_RES, y + h, buf);
    }

    heap_caps_free(buf);
}

void app_main(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "ST77916 QSPI Color Test - NO LVGL");
    ESP_LOGI(TAG, "===========================================");

    // Configure backlight GPIO
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(PIN_NUM_BL, 0);

    // Initialize QSPI bus
    ESP_LOGI(TAG, "Initializing QSPI bus...");
    spi_bus_config_t bus_config = {
        .sclk_io_num = PIN_NUM_SCLK,
        .data0_io_num = PIN_NUM_IO0,
        .data1_io_num = PIN_NUM_IO1,
        .data2_io_num = PIN_NUM_IO2,
        .data3_io_num = PIN_NUM_IO3,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    // Create panel IO handle (QSPI mode)
    ESP_LOGI(TAG, "Creating QSPI panel IO...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = PIN_NUM_CS,
        .dc_gpio_num = -1,
        .spi_mode = 3,
        .pclk_hz = LCD_PIXEL_CLK,
        .trans_queue_depth = 1,  // Force synchronous transactions to fix line artifacts
        .lcd_cmd_bits = 32,
        .lcd_param_bits = 8,
        .flags = {
            .quad_mode = true,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // Store io_handle globally for draw functions
    g_io_handle = io_handle;

    // Initialize ST77916 panel using custom driver (bypasses esp_lcd_st77916 component)
    ESP_LOGI(TAG, "Initializing ST77916 with custom driver...");
    ESP_ERROR_CHECK(st77916_panel_init(io_handle, PIN_NUM_RST));

    // Turn on backlight
    gpio_set_level(PIN_NUM_BL, 1);
    ESP_LOGI(TAG, "Backlight ON");

    // ========== COLOR TEST ==========
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Starting color test...");
    ESP_LOGI(TAG, "RGB565 values being sent:");
    ESP_LOGI(TAG, "  RED     = 0x%04X", RGB565_RED);
    ESP_LOGI(TAG, "  GREEN   = 0x%04X", RGB565_GREEN);
    ESP_LOGI(TAG, "  BLUE    = 0x%04X", RGB565_BLUE);
    ESP_LOGI(TAG, "  CYAN    = 0x%04X", RGB565_CYAN);
    ESP_LOGI(TAG, "  MAGENTA = 0x%04X", RGB565_MAGENTA);
    ESP_LOGI(TAG, "  YELLOW  = 0x%04X", RGB565_YELLOW);
    ESP_LOGI(TAG, "  WHITE   = 0x%04X", RGB565_WHITE);
    ESP_LOGI(TAG, "  ORANGE  = 0x%04X", RGB565_ORANGE);
    ESP_LOGI(TAG, "");

    // Clear screen to black - using standard RAMWR (0x2C)
    ESP_LOGI(TAG, "Using standard RAMWR (0x2C) command...");
    fill_screen(RGB565_BLACK);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Draw color test pattern - 3x3 grid
    int box_w = 100;
    int box_h = 100;
    int gap = 20;
    int start_x = (LCD_H_RES - (3 * box_w + 2 * gap)) / 2;
    int start_y = (LCD_V_RES - (3 * box_h + 2 * gap)) / 2;

    ESP_LOGI(TAG, "Drawing color grid...");

    // Row 1: RED, GREEN, BLUE (primary colors)
    fill_rect(start_x + 0*(box_w+gap), start_y + 0*(box_h+gap), box_w, box_h, RGB565_RED);
    fill_rect(start_x + 1*(box_w+gap), start_y + 0*(box_h+gap), box_w, box_h, RGB565_GREEN);
    fill_rect(start_x + 2*(box_w+gap), start_y + 0*(box_h+gap), box_w, box_h, RGB565_BLUE);

    // Row 2: CYAN, MAGENTA, YELLOW (secondary/mixed colors)
    fill_rect(start_x + 0*(box_w+gap), start_y + 1*(box_h+gap), box_w, box_h, RGB565_CYAN);
    fill_rect(start_x + 1*(box_w+gap), start_y + 1*(box_h+gap), box_w, box_h, RGB565_MAGENTA);
    fill_rect(start_x + 2*(box_w+gap), start_y + 1*(box_h+gap), box_w, box_h, RGB565_YELLOW);

    // Row 3: WHITE, ORANGE, PURPLE (more mixed colors)
    fill_rect(start_x + 0*(box_w+gap), start_y + 2*(box_h+gap), box_w, box_h, RGB565_WHITE);
    fill_rect(start_x + 1*(box_w+gap), start_y + 2*(box_h+gap), box_w, box_h, RGB565_ORANGE);
    fill_rect(start_x + 2*(box_w+gap), start_y + 2*(box_h+gap), box_w, box_h, RGB565_PURPLE);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Color grid displayed!");
    ESP_LOGI(TAG, "Expected layout:");
    ESP_LOGI(TAG, "  Row 1: RED | GREEN | BLUE");
    ESP_LOGI(TAG, "  Row 2: CYAN | MAGENTA | YELLOW");
    ESP_LOGI(TAG, "  Row 3: WHITE | ORANGE | PURPLE");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Please report what colors you actually see!");

    // Stay displaying the test pattern
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
