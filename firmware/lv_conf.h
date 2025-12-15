/**
 * @file lv_conf.h
 * Configuration file for LVGL v8.3.x
 * 
 * IMPORTANT: Copy this file to your Arduino libraries folder:
 * - Windows: Documents/Arduino/libraries/lvgl/
 * - Linux/Mac: ~/Arduino/libraries/lvgl/
 * 
 * Replace the existing lv_conf_template.h and rename it to lv_conf.h
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color. Useful if the display has an 8-bit interface (e.g. SPI) */
#define LV_COLOR_16_SWAP 0

/* Enable features to draw on transparent background */
#define LV_COLOR_SCREEN_TRANSP 0

/* Images pixels with this color will not be drawn if they are chroma keyed) */
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00) /*pure green*/

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* Use standard memcpy/memset for better performance */
#define LV_MEMCPY_MEMSET_STD 1

/* Use custom malloc (system malloc is faster) */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM == 1
  #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
  #define LV_MEM_CUSTOM_ALLOC   malloc
  #define LV_MEM_CUSTOM_FREE    free
  #define LV_MEM_CUSTOM_REALLOC realloc
#endif

/* Put performance-critical code in IRAM */
#define LV_ATTRIBUTE_FAST_MEM IRAM_ATTR

/* Number of the intermediate memory buffer used during rendering and other internal processing mechanisms. */
#define LV_MEM_BUF_MAX_NUM 16

/* Use the standard `memcpy` and `memset` instead of LVGL's own functions. */
#define LV_MEMCPY_MEMSET_STD 0

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period. LVG will redraw changed areas with this period time */
#define LV_DISP_DEF_REFR_PERIOD 10      /*[ms]*/

/* Input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30     /*[ms]*/

/* Use a custom tick source that tells the elapsed time in milliseconds. */
#define LV_TICK_CUSTOM 1 
#if LV_TICK_CUSTOM
  #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"         
  #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())    
#endif   /*LV_TICK_CUSTOM*/

/* Default Dot Per Inch. Used to initialize default sizes such as widgets sized, style paddings. */
#define LV_DPI_DEF 130     /*[px/inch]*/

#define LV_USE_PERF_MONITOR 0   

/*=================
 * OPERATING SYSTEM
 *=================*/

/* Select an operating system to use. */
// #define LV_USE_OS   LV_OS_NONE

/*=======================
 * RENDERING SETTINGS
 *=======================*/
#define LV_USE_SHADOW 0
#define LV_USE_GRADIENT 0
#define LV_USE_IMG_TRANSFORM 0

/* Use a direct mode where each pixel is written directly to the display */
#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW == 1
  /* Enable complex draw engine */
  #define LV_DRAW_SW_COMPLEX 0
  
  /* Allow buffering some shadow calculation. LV_DRAW_SW_SHADOW_CACHE_SIZE is the max. shadow size to buffer */
  #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
  
  /* Set number of maximally cached outline points. */
  #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
#endif

/* Enable GPU support */
#define LV_USE_DRAW_ARM2D 0
#define LV_USE_DRAW_STM32_DMA2D 0
#define LV_USE_DRAW_SWM341_DMA2D 0
#define LV_USE_DRAW_VG_LITE 0

/*=================
 * LOGGING
 *=================*/

/* Enable the log module */
#define LV_USE_LOG 0
#if LV_USE_LOG
  /* How important log should be added */
  #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

  /* 1: Print the log with 'printf'; 0: User need to register a callback with `lv_log_register_print_cb()` */
  #define LV_LOG_PRINTF 1

  /* Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs */
  #define LV_LOG_TRACE_MEM        1
  #define LV_LOG_TRACE_TIMER      1
  #define LV_LOG_TRACE_INDEV      1
  #define LV_LOG_TRACE_DISP_REFR  1
  #define LV_LOG_TRACE_EVENT      1
  #define LV_LOG_TRACE_OBJ_CREATE 1
  #define LV_LOG_TRACE_LAYOUT     1
  #define LV_LOG_TRACE_ANIM       1
#endif  /*LV_USE_LOG*/

/*=================
 * ASSERT
 *=================*/

/* Enable asserts if an operation is failed or an invalid data is found. */
#define LV_USE_ASSERT_NULL          1   /*Check if the parameter is NULL. (Very fast, recommended)*/
#define LV_USE_ASSERT_MALLOC        1   /*Checks is the memory is successfully allocated or no. (Very fast, recommended)*/
#define LV_USE_ASSERT_STYLE         0   /*Check if the styles are properly initialized. (Very fast, recommended)*/
#define LV_USE_ASSERT_MEM_INTEGRITY 0   /*Check the integrity of `lv_mem` after critical operations. (Slow)*/
#define LV_USE_ASSERT_OBJ           0   /*Check the object's type and existence (e.g. not deleted). (Slow)*/

/* Add a custom handler when assert happens */
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);   /*Halt by default*/

/*==================
 * FONT USAGE
 *==================*/

/* Montserrat fonts with various styles and weights */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 0
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

#define LV_FONT_DEFAULT &lv_font_montserrat_40

/* Demonstrate special features */
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0  /*bpp = 3*/
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0  /*Hebrew, Arabic, Persian letters and all their forms*/
#define LV_FONT_SIMSUN_16_CJK            0  /*1000 most common CJK radicals*/

/* Pixel perfect monospace fonts */
#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0

/* Optionally declare custom fonts here */
#define LV_FONT_CUSTOM_DECLARE

/* Enable handling large font and/or fonts with a lot of characters. */
#define LV_FONT_FMT_TXT_LARGE 0

/* Enables/disables support for compressed fonts. */
#define LV_USE_FONT_COMPRESSED 0

/* Enable drawing placeholders when glyph dsc is not found */
#define LV_USE_FONT_PLACEHOLDER 1

/*=================
 * TEXT SETTINGS
 *=================*/

/* Select a character encoding for strings. */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/* Can break (wrap) texts on these chars */
#define LV_TXT_BREAK_CHARS " ,.;:-_"

/* If a word is at least this long, will break wherever "prettiest" */
#define LV_TXT_LINE_BREAK_LONG_LEN 0

/* Minimum number of characters in a long word to put on a line before a break. */
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3

/* Minimum number of characters in a long word to put on a line after a break. */
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/* Support bidirectional texts. Allows mixing Left-to-Right and Right-to-Left texts. */
#define LV_USE_BIDI 0
#if LV_USE_BIDI
  /* Set the default direction. Supported values: `LV_BASE_DIR_LTR`, `LV_BASE_DIR_RTL`, `LV_BASE_DIR_AUTO` */
  #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif

/* Enable Arabic/Persian processing */
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*=================
 * WIDGET USAGE
 *=================*/

/* Documentation of the widgets: https://docs.lvgl.io/latest/en/html/widgets/index.html */

#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_CANVAS       1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1   /* Requires: lv_label */
#define LV_USE_IMG          1   /* Requires: lv_label */
#define LV_USE_LABEL        1
#if LV_USE_LABEL
  #define LV_LABEL_TEXT_SELECTION 1   /* Enable selecting text of the label */
  #define LV_LABEL_LONG_TXT_HINT 1    /* Store some extra info in labels to speed up drawing of very long texts */
#endif
#define LV_USE_LINE         1
#define LV_USE_ROLLER       1   /* Requires: lv_label */
#define LV_USE_SLIDER       1   /* Requires: lv_bar */
#define LV_USE_SWITCH       1
#define LV_USE_TEXTAREA     1   /* Requires: lv_label */
#if LV_USE_TEXTAREA != 0
  #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500    /*ms*/
#endif

#define LV_USE_TABLE        1

/*==================
 * EXTRA COMPONENTS
 *==================*/

/* 1: Enable the lv_animimg component (animated image) */
#define LV_USE_ANIMIMG      1

/* 1: Enable the lv_calendar component */
#define LV_USE_CALENDAR     1
#if LV_USE_CALENDAR
  #define LV_CALENDAR_WEEK_STARTS_MONDAY 0
  #if LV_CALENDAR_WEEK_STARTS_MONDAY
    #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"}
  #else
    #define LV_CALENDAR_DEFAULT_DAY_NAMES {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"}
  #endif

  #define LV_CALENDAR_DEFAULT_MONTH_NAMES {"January", "February", "March",  "April", "May",  "June", "July", "August", "September", "October", "November", "December"}
  #define LV_USE_CALENDAR_HEADER_ARROW 1
  #define LV_USE_CALENDAR_HEADER_DROPDOWN 1
#endif  /*LV_USE_CALENDAR*/

/* 1: Enable the lv_chart component */
#define LV_USE_CHART        1

/* 1: Enable the lv_colorwheel component */
#define LV_USE_COLORWHEEL   1

/* 1: Enable the lv_imgbtn component */
#define LV_USE_IMGBTN       1

/* 1: Enable the lv_keyboard component */
#define LV_USE_KEYBOARD     1

/* 1: Enable the lv_led component */
#define LV_USE_LED          1

/* 1: Enable the lv_list component */
#define LV_USE_LIST         1

/* 1: Enable the lv_menu component */
#define LV_USE_MENU         1

/* 1: Enable the lv_meter component */
#define LV_USE_METER        1

/* 1: Enable the lv_msgbox component */
#define LV_USE_MSGBOX       1

/* 1: Enable the lv_span component (rich text like HTML <span>) */
#define LV_USE_SPAN         1
#if LV_USE_SPAN
  /* A line text can contain maximum num of span descriptor */
  #define LV_SPAN_SNIPPET_STACK_SIZE 64
#endif

/* 1: Enable the lv_spinbox component */
#define LV_USE_SPINBOX      1

/* 1: Enable the lv_spinner component */
#define LV_USE_SPINNER      1

/* 1: Enable the lv_tabview component */
#define LV_USE_TABVIEW      1

/* 1: Enable the lv_tileview component */
#define LV_USE_TILEVIEW     1

/* 1: Enable the lv_win component */
#define LV_USE_WIN          1

/*==================
 * THEMES
 *==================*/

/* A simple, impressive and very complete theme */
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
  /* 0: Light mode; 1: Dark mode */
  #define LV_THEME_DEFAULT_DARK 1

  /* 1: Enable grow on press */
  #define LV_THEME_DEFAULT_GROW 1

  /* Default transition time in [ms] */
  #define LV_THEME_DEFAULT_TRANSITION_TIME 0
#endif /*LV_USE_THEME_DEFAULT*/

/* A very simple theme that is a good starting point for a custom theme */
#define LV_USE_THEME_BASIC 1

/* A theme designed for monochrome displays */
#define LV_USE_THEME_MONO 1

/*==================
 * LAYOUTS
 *==================*/

/* A layout similar to Flexbox in CSS */
#define LV_USE_FLEX 1

/* A layout similar to Grid in CSS */
#define LV_USE_GRID 1

/*==================
 * EXAMPLES
 *==================*/

/* Enable the examples to be built with the library */
#define LV_BUILD_EXAMPLES 0

/*==================
 * DEMOS
 *==================*/

/* Show some widget. It might be required to increase `LV_MEM_SIZE` */
#define LV_USE_DEMO_WIDGETS 0
#if LV_USE_DEMO_WIDGETS
  #define LV_DEMO_WIDGETS_SLIDESHOW 0
#endif

/* Demonstrate the usage of encoder and keyboard */
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0

/* Benchmark your system */
#define LV_USE_DEMO_BENCHMARK 0
#if LV_USE_DEMO_BENCHMARK
  /* Use RGB565A8 images with 16 bit color depth instead of ARGB8565 */
  #define LV_DEMO_BENCHMARK_RGB565A8 0
#endif

/* Stress test for LVGL */
#define LV_USE_DEMO_STRESS 0

/* Music player demo */
#define LV_USE_DEMO_MUSIC 0
#if LV_USE_DEMO_MUSIC
  #define LV_DEMO_MUSIC_SQUARE    0
  #define LV_DEMO_MUSIC_LANDSCAPE 0
  #define LV_DEMO_MUSIC_ROUND     0
  #define LV_DEMO_MUSIC_LARGE     0
  #define LV_DEMO_MUSIC_AUTO_PLAY 0
#endif

/*--END OF LV_CONF_H--*/

#endif /*LV_CONF_H*/