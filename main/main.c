/**
 * ST77916 QSPI Display - LVGL Meter Demo
 *
 * Pin Configuration:
 * CS    -> GPIO5
 * SCLK  -> GPIO6
 * RST   -> GPIO7
 * D0    -> GPIO8
 * D1    -> GPIO9
 * D2    -> GPIO10
 * D3    -> GPIO11
 * BL    -> GPIO4
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "st77916_panel.h"
#include "ui/ui.h"

static const char *TAG = "ST77916_LVGL";

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

// Display resolution
#define LCD_H_RES       360
#define LCD_V_RES       360
#define LCD_PIXEL_CLK   (20 * 1000 * 1000)

// Double-buffered draw buffers (40 lines each)
#define DRAW_BUF_LINES  40
static lv_color_t draw_buf1[LCD_H_RES * DRAW_BUF_LINES];
static lv_color_t draw_buf2[LCD_H_RES * DRAW_BUF_LINES];

static esp_lcd_panel_io_handle_t g_io_handle = NULL;

// LVGL flush callback — sends rendered tile to the display
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    st77916_panel_draw_bitmap(g_io_handle,
                              area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1,
                              color_p);
    lv_disp_flush_ready(drv);
}

// esp_timer ISR: advance LVGL's internal clock every 1 ms
static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(1);
}

// Single task owns all LVGL calls (thread-safety requirement)
static void lvgl_main_task(void *arg)
{
    ui_init();

    int32_t speed = 0;
    int32_t dir   = 1;
    uint32_t last_speed_ms = 0;

    while (1) {
        lv_timer_handler();

        // Advance simulated speed at ~30 ms intervals
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if ((now_ms - last_speed_ms) >= 30) {
            last_speed_ms = now_ms;
            ui_set_meter_value(speed);
            speed += dir;
            if (speed >= 100) dir = -1;
            if (speed <= 0)   dir =  1;
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ST77916 LVGL Meter Demo");

    // Backlight GPIO
    gpio_config_t bk_gpio_config = {
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BL,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_set_level(PIN_NUM_BL, 0);

    // Initialize QSPI bus
    spi_bus_config_t bus_config = {
        .sclk_io_num  = PIN_NUM_SCLK,
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

    // Create QSPI panel IO handle
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num       = PIN_NUM_CS,
        .dc_gpio_num       = -1,
        .spi_mode          = 3,
        .pclk_hz           = LCD_PIXEL_CLK,
        .trans_queue_depth = 1,
        .lcd_cmd_bits      = 32,
        .lcd_param_bits    = 8,
        .flags             = { .quad_mode = true },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    g_io_handle = io_handle;

    // Initialize ST77916 panel
    ESP_ERROR_CHECK(st77916_panel_init(io_handle, PIN_NUM_RST));
    gpio_set_level(PIN_NUM_BL, 1);

    // Initialize LVGL
    lv_init();

    // Register display driver with double-buffered draw buffers
    static lv_disp_draw_buf_t draw_buf_dsc;
    lv_disp_draw_buf_init(&draw_buf_dsc, draw_buf1, draw_buf2, LCD_H_RES * DRAW_BUF_LINES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = LCD_H_RES;
    disp_drv.ver_res  = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf_dsc;
    lv_disp_drv_register(&disp_drv);

    // Start 1 ms periodic timer to drive lv_tick_inc()
    const esp_timer_create_args_t tick_timer_args = {
        .callback = lvgl_tick_cb,
        .name     = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000));  // 1000 us = 1 ms

    // Launch combined LVGL handler + speed simulation task
    xTaskCreate(lvgl_main_task, "lvgl_main", 8192, NULL, 5, NULL);
}
