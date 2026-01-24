/**
 * ST77916 QSPI Display Test with LVGL 8 Meter Gauge
 * ESP32-S3 DevKitC-1
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "st77916_init.h"

static const char *TAG = "ST77916";

// LVGL callback function prototypes
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
static void lvgl_tick_timer_cb(void *arg);

// Pin definitions
#define LCD_HOST        SPI2_HOST
#define PIN_NUM_CS      5
#define PIN_NUM_SCLK    6
#define PIN_NUM_RST     7
#define PIN_NUM_IO0     8   // Display pin 10 (IO0)
#define PIN_NUM_IO1     9   // Display pin 9  (IO1)
#define PIN_NUM_IO2     10  // Display pin 8  (IO2)
#define PIN_NUM_IO3     11  // Display pin 7  (IO3)
#define PIN_NUM_BL      4

// Display specs
#define LCD_H_RES       360
#define LCD_V_RES       360
#define LCD_PIXEL_CLK   (20 * 1000 * 1000)  // 20MHz

// Global panel handle for LVGL flush callback
static esp_lcd_panel_handle_t g_panel_handle = NULL;

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ST77916 QSPI Display Test with LVGL 8");

    // Configure backlight GPIO
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(PIN_NUM_BL, 0);  // Off initially

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
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
        .flags = SPICOMMON_BUSFLAG_MASTER,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    // Create panel IO handle (QSPI mode)
    ESP_LOGI(TAG, "Creating QSPI panel IO...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = PIN_NUM_CS,
        .dc_gpio_num = -1,  // No DC pin in QSPI mode
        .spi_mode = 0,
        .pclk_hz = LCD_PIXEL_CLK,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 32,  // QSPI uses 32-bit commands
        .lcd_param_bits = 8,
        .flags = {
            .quad_mode = true,  // Enable QSPI
        },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // Initialize ST77916 panel with manufacturer's sequence
    ESP_LOGI(TAG, "Initializing ST77916 display...");
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(st77916_init_panel(io_handle, PIN_NUM_RST, &panel_handle));
    g_panel_handle = panel_handle;

    // Turn on backlight
    gpio_set_level(PIN_NUM_BL, 1);
    ESP_LOGI(TAG, "Backlight ON");

    // ========== LVGL 8 INITIALIZATION ==========
    ESP_LOGI(TAG, "Initializing LVGL 8...");
    lv_init();

    // Allocate LVGL draw buffers
    static lv_color_t *buf1 = NULL;
    static lv_color_t *buf2 = NULL;
    size_t buf_size = LCD_H_RES * LCD_V_RES / 10;  // 1/10th of screen
    buf1 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    buf2 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!buf1 || !buf2) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffers!");
        return;
    }
    ESP_LOGI(TAG, "LVGL buffers allocated: %d bytes each", buf_size * sizeof(lv_color_t));

    // Initialize LVGL draw buffer
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, buf_size);

    // Register display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    // LVGL tick timer
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_tick_timer_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 2000)); // 2ms tick

    // ========== CREATE METER GAUGE UI ==========
    ESP_LOGI(TAG, "Creating meter gauge UI...");

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // Create meter widget for gauge
    lv_obj_t *meter = lv_meter_create(scr);
    lv_obj_set_size(meter, 300, 300);
    lv_obj_center(meter);

    // Add scale for 0-120 MPH
    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 31, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter, scale, 5, 4, 15, lv_color_white(), 15);
    lv_meter_set_scale_range(meter, scale, 0, 120, 270, 135);

    // Add red needle indicator
    lv_meter_indicator_t *indic_needle = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_RED), -10);

    // Create value label below gauge
    lv_obj_t *value_label = lv_label_create(scr);
    lv_obj_set_style_text_color(value_label, lv_color_white(), 0);
    lv_label_set_text(value_label, "0 MPH");
    lv_obj_align(value_label, LV_ALIGN_CENTER, 0, 100);

    ESP_LOGI(TAG, "Gauge created successfully!");

    // ========== ANIMATION LOOP ==========
    ESP_LOGI(TAG, "Starting gauge animation...");

    int32_t speed = 0;
    int32_t direction = 1;

    while (1) {
        // Update speed
        speed += direction * 1;

        // Reverse at limits
        if (speed >= 120) {
            speed = 120;
            direction = -1;
        } else if (speed <= 0) {
            speed = 0;
            direction = 1;
        }

        // Update needle position
        lv_meter_set_indicator_value(meter, indic_needle, speed);

        // Update label every 5 MPH to reduce redraws
        if (speed % 5 == 0) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%ld MPH", speed);
            lv_label_set_text(value_label, buf);
        }

        // LVGL task handler
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(40)); // 25 FPS
    }
}

// LVGL flush callback - sends framebuffer to display
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2 + 1;
    int y2 = area->y2 + 1;

    // Draw bitmap to display
    esp_lcd_panel_draw_bitmap(g_panel_handle, x1, y1, x2, y2, color_map);

    // Notify LVGL flush is complete
    lv_disp_flush_ready(drv);
}

// LVGL tick timer callback
static void lvgl_tick_timer_cb(void *arg)
{
    lv_tick_inc(2); // 2ms tick
}