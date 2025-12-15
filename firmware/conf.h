#ifndef CONF_H
#define CONF_H

#define THEME_DARK   1
#define THEME_LIGHT  2

#define ACTIVE_THEME THEME_DARK

#if ACTIVE_THEME == THEME_DARK
  #define BG_COLOR lv_color_hex(0x000000)
  #define TEXT_COLOR lv_color_hex(0xeff1f5)
  #define BAR_BG_COLOR lv_color_hex(0x4c4f69)
  #define ACCENT_COLOR lv_color_hex(0x8839ef)
  #define WARNING_COLOR lv_color_hex(0xdf8e1d)
  #define CRITICAL_COLOR lv_color_hex(0xd20f39)
#elif ACTIVE_THEME == THEME_LIGHT
  #define BG_COLOR lv_color_hex(0xeff1f5)
  #define TEXT_COLOR lv_color_hex(0x4c4f69)
  #define BAR_BG_COLOR lv_color_hex(0xdce0e8)
  #define ACCENT_COLOR lv_color_hex(0x1e66f5)
  #define WARNING_COLOR lv_color_hex(0xdf8e1d)
  #define CRITICAL_COLOR lv_color_hex(0xd20f39)
#else
  #error "Invalid ACTIVE_THEME selected"
#endif

// General Configuration
#define ANIM_TIME 1000
#define OUTER_MARGIN 10
#define ROW_PADDING 10
#define ROW_SPACING 0
#define ICON_WIDTH 100
#define BAR_HEIGHT 90
#define BAR_RADIUS 10
#define ICON_BAR_SPACING 5
#define WARNING_THRESHOLD 80
#define CRITICAL_THRESHOLD 90
#define PIXEL_CLOCK (30 * 1000 * 1000)
#define BUFFER_LINES 60

#endif // CONF_H
