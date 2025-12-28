#ifndef CONFIG_H
#define CONFIG_H

#include <lvgl.h>
#include <Preferences.h>

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

// Theme and accent color enums
enum Theme {
  THEME_DARK = 1,
  THEME_LIGHT = 2
};

enum AccentColor {
  ACCENT_COLOR_SAPPHIRE = 1,
  ACCENT_COLOR_MAUVE = 2,
  ACCENT_COLOR_GREEN = 3,
  ACCENT_COLOR_PEACH = 4
};

// Configuration class with runtime theme switching
class Config {
private:
  Preferences prefs;
  Theme currentTheme;
  AccentColor currentAccent;
  
public:  
  Config() {
    prefs.begin("config", false);
    currentTheme = (Theme)prefs.getInt("theme", THEME_DARK);
    currentAccent = (AccentColor)prefs.getInt("accent", ACCENT_COLOR_SAPPHIRE);
  }
  
  void setTheme(Theme theme) {
    currentTheme = theme;
    prefs.putInt("theme", theme);
  }
  
  void setAccentColor(AccentColor accent) {
    currentAccent = accent;
    prefs.putInt("accent", accent);
  }
  
  Theme getTheme() { return currentTheme; }
  AccentColor getAccentColor() { return currentAccent; }
  
  lv_color_t getAccentColorValue() {
    switch(currentAccent) {
      case ACCENT_COLOR_SAPPHIRE: return lv_color_hex(0x209fb5);
      case ACCENT_COLOR_MAUVE:    return lv_color_hex(0x8839ef);
      case ACCENT_COLOR_GREEN:    return lv_color_hex(0x40a02b);
      case ACCENT_COLOR_PEACH:    return lv_color_hex(0xfe640b);
      default:                    return lv_color_hex(0x209fb5);
    }
  }
  
  lv_color_t getBgColor() {
    return (currentTheme == THEME_DARK) ? 
           lv_color_hex(0x000000) : lv_color_hex(0xeff1f5);
  }
  
  lv_color_t getTextColor() {
    return (currentTheme == THEME_DARK) ? 
           lv_color_hex(0xeff1f5) : lv_color_hex(0x4c4f69);
  }
  
  lv_color_t getBarBgColor() {
    return (currentTheme == THEME_DARK) ? 
           lv_color_hex(0x4c4f69) : lv_color_hex(0xdce0e8);
  }
  
  lv_color_t getWarningColor() {
    return lv_color_hex(0xdf8e1d);
  }
  
  lv_color_t getCriticalColor() {
    return lv_color_hex(0xd20f39);
  }
};

// Global config instance
extern Config config;

#endif // CONFIG_H
