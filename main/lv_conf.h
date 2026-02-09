/**
 * LVGL 8 Configuration for ST77916 Display
 * 360x360 Round Display with QSPI Interface
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color. Useful if the display has a BGR interface */
#define LV_COLOR_16_SWAP 0  // Back to no swap

/* Enable dithering for better color quality */
#define LV_DITHER_GRADIENT 0  // TEST: Disable dithering

/*====================
   MEMORY SETTINGS
 *====================*/

/* Size of the memory used by `lv_malloc()` in bytes (>= 2kB) */
#define LV_MEM_SIZE (64 * 1024U)  // 64KB for ESP32-S3

/* Use ESP32 malloc/free */
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC   malloc
#define LV_MEM_CUSTOM_FREE    free
#define LV_MEM_CUSTOM_REALLOC realloc

/*====================
   DRAWING SETTINGS
 *====================*/

/* Enable complex drawing (anti-aliasing, shadows, etc.) */
#define LV_DRAW_COMPLEX 1

/* Enable anti-aliasing for smooth edges */
#define LV_ANTIALIAS 1

/* Default VDB (draw buffer) size - more pixels = smoother updates */
#define LV_VDB_SIZE (360 * 40)  // 40 lines at a time

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period in milliseconds */
#define LV_DISP_DEF_REFR_PERIOD 16  // ~60 FPS (was 50)

/* Default input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30

/*====================
   FEATURE USAGE
 *====================*/

/* Enable logging */
#define LV_USE_LOG 1
#if LV_USE_LOG
  #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
  #define LV_LOG_PRINTF 1
#endif

/* Enable animations */
#define LV_USE_ANIMATION 1

/*====================
   WIDGETS
 *====================*/

#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1
#define LV_USE_METER 1  // For gauge widget with needle support

/*====================
   THEMES
 *====================*/

#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
  #define LV_THEME_DEFAULT_DARK 1
  #define LV_THEME_DEFAULT_GROW 1
#endif

/*====================
   FONTS
 *====================*/

/* Montserrat fonts */
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 1
#define LV_FONT_MONTSERRAT_32 1

/* Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*====================
   OTHERS
 *====================*/

/* Enable assertions for debugging */
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

/* Enable performance monitoring */
#define LV_USE_PERF_MONITOR 0

/* Enable memory monitoring */
#define LV_USE_MEM_MONITOR 0

/* User data for objects */
#define LV_USE_USER_DATA 1

#endif /* LV_CONF_H */