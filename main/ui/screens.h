#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *obj0;  // meter object
} objects_t;

extern objects_t objects;

// Expose meter components for animation
extern lv_obj_t *ui_meter;
extern lv_meter_indicator_t *ui_meter_indicator;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
};

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

// Helper function to set meter value
void ui_set_meter_value(int32_t value);

// Night mode for headlights-on driving (dimmed with red hue)
void ui_set_night_mode(bool enabled);
bool ui_get_night_mode(void);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/