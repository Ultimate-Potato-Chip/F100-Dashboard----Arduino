#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"
#include "../st77916_colors.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

// Exposed for animation from main.c
lv_obj_t *ui_meter = NULL;
lv_meter_indicator_t *ui_meter_indicator = NULL;

static lv_meter_scale_t * scale0;
static bool night_mode_enabled = false;

// Night mode colors - warm amber tint like 80s/90s BMW gauges
#define NIGHT_BG_COLOR      ST77916_FIX_COLOR(0x0d0805)  // Very dark warm black (subtle amber tint)
#define NIGHT_TICK_COLOR    ST77916_FIX_COLOR(0xcc8855)  // Warm amber ticks (dimmed)
#define NIGHT_NEEDLE_COLOR  ST77916_FIX_COLOR(0xaa6030)  // Dimmed warm orange needle

// Day mode colors
#define DAY_BG_COLOR        0x000000                     // Black (no fix needed)
#define DAY_TICK_COLOR      0xffffff                     // White (no fix needed)
#define DAY_NEEDLE_COLOR    ST77916_FIX_COLOR(0xffb046)  // Amber/orange

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    // Use black background - no color fix needed for black
    lv_obj_set_style_bg_color(obj, lv_color_hex(DAY_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_meter_create(parent_obj);
            objects.obj0 = obj;
            ui_meter = obj;  // Store reference for external access
            lv_obj_set_pos(obj, -20, 81);
            lv_obj_set_size(obj, 400, 400);
            {
                lv_meter_scale_t *scale = lv_meter_add_scale(obj);
                scale0 = scale;
                lv_meter_set_scale_ticks(obj, scale, 5, 6, 12, lv_color_hex(DAY_TICK_COLOR));
                lv_meter_set_scale_major_ticks(obj, scale, 4, 7, 26, lv_color_hex(DAY_TICK_COLOR), 300);
                lv_meter_set_scale_range(obj, scale, 0, 100, 45, 248);
                {
                    // Needle: amber/orange
                    lv_meter_indicator_t *indicator = lv_meter_add_needle_line(obj, scale, 9, lv_color_hex(DAY_NEEDLE_COLOR), -2);
                    ui_meter_indicator = indicator;  // Store reference for external access
                    lv_meter_set_indicator_value(obj, indicator, 15);
                }
            }
            // Meter background - black, no fix needed
            lv_obj_set_style_bg_color(obj, lv_color_hex(DAY_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(DAY_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    tick_screen_main();
}

void tick_screen_main() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    create_screen_main();
}

// Helper function to set meter value from main.c
void ui_set_meter_value(int32_t value) {
    if (ui_meter && ui_meter_indicator) {
        lv_meter_set_indicator_value(ui_meter, ui_meter_indicator, value);
    }
}

// Night mode: dim display with warm amber hue for night driving
void ui_set_night_mode(bool enabled) {
    if (!ui_meter || !objects.main) return;

    night_mode_enabled = enabled;

    if (enabled) {
        // Night mode: warm dark background with dimmed gauge
        lv_obj_set_style_bg_color(objects.main, lv_color_hex(NIGHT_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_meter, lv_color_hex(NIGHT_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_meter, lv_color_hex(NIGHT_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);

        // Dim the entire meter using opacity (gives warm dim effect)
        lv_obj_set_style_opa(ui_meter, LV_OPA_60, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(ui_meter, LV_OPA_60, LV_PART_TICKS | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(ui_meter, LV_OPA_70, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    } else {
        // Day mode: black background, full brightness
        lv_obj_set_style_bg_color(objects.main, lv_color_hex(DAY_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_meter, lv_color_hex(DAY_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(ui_meter, lv_color_hex(DAY_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);

        // Restore full opacity
        lv_obj_set_style_opa(ui_meter, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(ui_meter, LV_OPA_COVER, LV_PART_TICKS | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(ui_meter, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
}

// Get current night mode state
bool ui_get_night_mode(void) {
    return night_mode_enabled;
}