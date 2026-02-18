#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

static lv_meter_scale_t * scale0;
static lv_meter_indicator_t * indicator1;

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffdbe4ea), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_meter_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, -20, 81);
            lv_obj_set_size(obj, 400, 400);
            {
                lv_meter_scale_t *scale = lv_meter_add_scale(obj);
                scale0 = scale;
                lv_meter_set_scale_ticks(obj, scale, 5, 6, 12, lv_color_hex(0xffffffff));
                lv_meter_set_scale_major_ticks(obj, scale, 4, 7, 26, lv_color_hex(0xffffffff), 300);
                lv_meter_set_scale_range(obj, scale, 0, 100, 45, 248);
                {
                    lv_meter_indicator_t *indicator = lv_meter_add_needle_line(obj, scale, 9, lv_color_hex(0xffffb046), -2);
                    indicator1 = indicator;
                    lv_meter_set_indicator_value(obj, indicator, 15);
                }
            }
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffdbe4ea), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffdbe4ea), LV_PART_MAIN | LV_STATE_DEFAULT);
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

void ui_set_meter_value(int32_t value) {
    if (objects.obj0 && indicator1) {
        lv_meter_set_indicator_value(objects.obj0, indicator1, value);
    }
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
}
