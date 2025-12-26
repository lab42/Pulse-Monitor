#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
 * COLOR SETTINGS
 *====================*/

/* RGB565 is optimal for this display */
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

/*====================
 * MEMORY SETTINGS
 *====================*/

/* Use system malloc (ESP32 heap / PSRAM aware) */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM
  #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
  #define LV_MEM_CUSTOM_ALLOC   malloc
  #define LV_MEM_CUSTOM_FREE    free
  #define LV_MEM_CUSTOM_REALLOC realloc
#endif

/* Use optimized libc memcpy/memset */
#define LV_MEMCPY_MEMSET_STD 1

/*====================
 * HAL SETTINGS
 *====================*/

/* Display refresh rate (ms)
 * 10 = 100 FPS max (LVGL redraws only dirty areas)
 */
#define LV_DISP_DEF_REFR_PERIOD 10

/* Input device polling */
#define LV_INDEV_DEF_READ_PERIOD 30

/* Arduino millis() tick source */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
  #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
  #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

/* Default DPI for widget scaling */
#define LV_DPI_DEF 130

/*====================
 * DRAWING SETTINGS
 *====================*/

/* Software renderer (ESP32-S3 has no LVGL GPU backend) */
#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW
  #define LV_DRAW_SW_COMPLEX 0   /* No shadows, gradients, transforms */
#endif

/* Disable GPU backends */
#define LV_USE_DRAW_ARM2D 0
#define LV_USE_DRAW_STM32_DMA2D 0
#define LV_USE_DRAW_SWM341_DMA2D 0
#define LV_USE_DRAW_VG_LITE 0

/*====================
 * LOGGING
 *====================*/

#define LV_USE_LOG 0

/*====================
 * ASSERT
 *====================*/

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);

/*====================
 * FONT SETTINGS
 *====================*/

#define LV_FONT_FMT_TXT_LARGE 0
#define LV_USE_FONT_COMPRESSED 0
#define LV_USE_FONT_PLACEHOLDER 1

/*====================
 * TEXT SETTINGS
 *====================*/

#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_"
#define LV_TXT_LINE_BREAK_LONG_LEN 0

#define LV_USE_BIDI 0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*====================
 * WIDGETS
 *====================*/

#define LV_USE_ARC       0
#define LV_USE_BAR       1
#define LV_USE_BTN       0
#define LV_USE_BTNMATRIX 0
#define LV_USE_CANVAS    0
#define LV_USE_CHECKBOX  0
#define LV_USE_DROPDOWN  0
#define LV_USE_IMG       0
#define LV_USE_LABEL     1
#define LV_USE_LINE      0
#define LV_USE_ROLLER    0
#define LV_USE_SLIDER    0
#define LV_USE_SWITCH    0
#define LV_USE_TEXTAREA  0
#define LV_USE_TABLE     0

#if LV_USE_LABEL
  #define LV_LABEL_TEXT_SELECTION 1
  #define LV_LABEL_LONG_TXT_HINT 1
#endif

#if LV_USE_TEXTAREA
  #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500
#endif

/*====================
 * EXTRA COMPONENTS
 *====================*/

#define LV_USE_ANIMIMG    0
#define LV_USE_CALENDAR   0
#define LV_USE_CHART      0
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN     0
#define LV_USE_KEYBOARD   0
#define LV_USE_LED        0
#define LV_USE_LIST       0
#define LV_USE_MENU       0
#define LV_USE_METER      0
#define LV_USE_MSGBOX     0
#define LV_USE_SPAN       0
#define LV_USE_SPINBOX    0
#define LV_USE_SPINNER    0
#define LV_USE_TABVIEW    0
#define LV_USE_TILEVIEW   0
#define LV_USE_WIN        0

#if LV_USE_SPAN
  #define LV_SPAN_SNIPPET_STACK_SIZE 64
#endif

/*====================
 * THEMES
 *====================*/

/* Disable built-in themes (you use runtime theming) */
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_BASIC   0
#define LV_USE_THEME_MONO    0

/*====================
 * LAYOUTS
 *====================*/

#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*====================
 * EXAMPLES / DEMOS
 *====================*/

#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0

#endif /* LV_CONF_H */
