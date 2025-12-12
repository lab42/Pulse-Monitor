// ================= LVGL & Memory Macros =================
#define LV_MEM_CUSTOM 1                  // Use custom memory allocator
#define LV_MEMCPY_MEMSET_STD 1           // Use standard memcpy/memset
#define LV_ATTRIBUTE_FAST_MEM IRAM_ATTR  // Place critical LVGL functions in IRAM
#include <lvgl.h>
#include <Arduino.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <ArduinoJson.h>

LV_FONT_DECLARE(bootstrap_icons_80);
LV_FONT_DECLARE(comfortaa_40);
LV_FONT_DECLARE(comfortaa_42);

// ================= Configuration Variables =================

// Colors
lv_color_t bg_color;
lv_color_t text_color;
lv_color_t bar_bg_color;
lv_color_t bar_accent_color;
lv_color_t bar_warning_color;
lv_color_t bar_critical_color;

// Animation & Timing
uint32_t anim_time;

// Layout & Spacing
uint16_t outer_margin;        // Space around main container
uint16_t row_padding;          // Padding inside each row
uint16_t row_spacing;          // Space between rows
uint16_t icon_width;           // Width of CPU/Memory/GPU icons
uint16_t bar_height;           // Height of progress bars
uint16_t bar_radius;           // Corner radius of bars
uint16_t icon_bar_spacing;     // Space between icon and bar
uint16_t network_icon_spacing; // Space between network icon and text

// Thresholds
uint8_t warning_threshold;     // Percentage for yellow color
uint8_t critical_threshold;    // Percentage for red color

// Display Configuration
uint32_t pixel_clock;          // Pixel clock frequency
uint16_t buffer_lines;         // Number of lines in display buffer

// ================= UI Objects =================
lv_obj_t *cpu_bar;
lv_obj_t *memory_bar;
lv_obj_t *gpu_bar;
lv_obj_t *cpu_label;
lv_obj_t *memory_label;
lv_obj_t *gpu_label;
lv_obj_t *download_label;
lv_obj_t *upload_label;

// Global styles 
static lv_style_t cpu_bar_style_main;
static lv_style_t cpu_bar_style_indic;
static lv_style_t memory_bar_style_main;
static lv_style_t memory_bar_style_indic;
static lv_style_t gpu_bar_style_main;
static lv_style_t gpu_bar_style_indic;

// Display resolution
#define LCD_H_RES 800
#define LCD_V_RES 480

// RGB LCD GPIO pins
#define PIN_NUM_HSYNC       46
#define PIN_NUM_VSYNC       3
#define PIN_NUM_DE          5
#define PIN_NUM_PCLK        7
#define PIN_NUM_DATA0       14
#define PIN_NUM_DATA1       38
#define PIN_NUM_DATA2       18
#define PIN_NUM_DATA3       17
#define PIN_NUM_DATA4       10
#define PIN_NUM_DATA5       39
#define PIN_NUM_DATA6       0
#define PIN_NUM_DATA7       45
#define PIN_NUM_DATA8       48
#define PIN_NUM_DATA9       47
#define PIN_NUM_DATA10      21
#define PIN_NUM_DATA11      1
#define PIN_NUM_DATA12      2
#define PIN_NUM_DATA13      42
#define PIN_NUM_DATA14      41
#define PIN_NUM_DATA15      40
#define PIN_NUM_DISP_EN     -1
#define PIN_NUM_BK_LIGHT    2

// LVGL
static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static lv_color_t *lv_disp_buf;
static esp_lcd_panel_handle_t panel_handle = NULL;

static bool on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    return false;
}

void flush_display(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_disp_flush_ready(disp_drv);
}

void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    data->state = LV_INDEV_STATE_REL;
}

void create_ui()
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, bg_color, 0);

    // Initialize styles with animation
    lv_style_init(&cpu_bar_style_main);
    lv_style_set_bg_color(&cpu_bar_style_main, bar_bg_color);
    lv_style_set_radius(&cpu_bar_style_main, bar_radius);
    lv_style_set_anim_time(&cpu_bar_style_main, anim_time);

    lv_style_init(&cpu_bar_style_indic);
    lv_style_set_bg_color(&cpu_bar_style_indic, bar_accent_color);
    lv_style_set_radius(&cpu_bar_style_indic, 0);
    lv_style_set_anim_time(&cpu_bar_style_indic, anim_time);

    lv_style_init(&memory_bar_style_main);
    lv_style_set_bg_color(&memory_bar_style_main, bar_bg_color);
    lv_style_set_radius(&memory_bar_style_main, bar_radius);
    lv_style_set_anim_time(&memory_bar_style_main, anim_time);

    lv_style_init(&memory_bar_style_indic);
    lv_style_set_bg_color(&memory_bar_style_indic, bar_accent_color);
    lv_style_set_radius(&memory_bar_style_indic, bar_radius);
    lv_style_set_anim_time(&memory_bar_style_indic, anim_time);

    lv_style_init(&gpu_bar_style_main);
    lv_style_set_bg_color(&gpu_bar_style_main, bar_bg_color);
    lv_style_set_radius(&gpu_bar_style_main, bar_radius);
    lv_style_set_anim_time(&gpu_bar_style_main, anim_time);

    lv_style_init(&gpu_bar_style_indic);
    lv_style_set_bg_color(&gpu_bar_style_indic, bar_accent_color);
    lv_style_set_radius(&gpu_bar_style_indic, bar_radius);
    lv_style_set_anim_time(&gpu_bar_style_indic, anim_time);

    // Main container
    lv_obj_t *main_container = lv_obj_create(scr);
    lv_obj_set_size(main_container, LCD_H_RES - (outer_margin * 2), LCD_V_RES - (outer_margin * 2));
    lv_obj_center(main_container);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(main_container, row_padding, 0);
    lv_obj_set_style_pad_row(main_container, row_spacing, 0);
    lv_obj_set_style_pad_column(main_container, 0, 0);
    lv_obj_set_flex_align(main_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_bg_color(main_container, bg_color, 0);
    lv_obj_set_style_border_width(main_container, 0, 0);

    const lv_coord_t row_height = (LCD_V_RES - (outer_margin * 2) - (row_padding * 2) - (row_spacing * 3)) / 4;

    // CPU Row
    lv_obj_t *cpu_row = lv_obj_create(main_container);
    lv_obj_set_size(cpu_row, lv_pct(100), row_height);
    lv_obj_set_flex_flow(cpu_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(cpu_row, row_padding * 2, 0);
    lv_obj_set_style_pad_column(cpu_row, icon_bar_spacing, 0);
    lv_obj_set_style_border_width(cpu_row, 0, 0);
    lv_obj_set_flex_grow(cpu_row, 0);
    lv_obj_set_style_bg_color(cpu_row, bg_color, 0);
    lv_obj_set_flex_align(cpu_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cpu_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cpu_icon = lv_label_create(cpu_row);
    lv_obj_set_style_text_font(cpu_icon, &bootstrap_icons_80, 0);
    lv_obj_set_style_text_color(cpu_icon, text_color, 0);
    lv_label_set_text(cpu_icon, "\U0000F2D6");
    lv_obj_set_width(cpu_icon, icon_width);

    cpu_bar = lv_bar_create(cpu_row);
    lv_bar_set_range(cpu_bar, 0, 100);
    lv_bar_set_value(cpu_bar, 0, LV_ANIM_OFF);
    lv_obj_set_flex_grow(cpu_bar, 1);
    lv_obj_set_height(cpu_bar, bar_height);
    lv_obj_add_style(cpu_bar, &cpu_bar_style_main, LV_PART_MAIN);
    lv_obj_add_style(cpu_bar, &cpu_bar_style_indic, LV_PART_INDICATOR);

    cpu_label = lv_label_create(cpu_bar);
    lv_label_set_text(cpu_label, "0%");
    lv_obj_set_style_text_color(cpu_label, text_color, 0);
    lv_obj_set_style_text_font(cpu_label, &comfortaa_40, 0);
    lv_obj_center(cpu_label);

    // Memory Row
    lv_obj_t *memory_row = lv_obj_create(main_container);
    lv_obj_set_size(memory_row, lv_pct(100), row_height);
    lv_obj_set_flex_flow(memory_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(memory_row, row_padding * 2, 0);
    lv_obj_set_style_pad_column(memory_row, icon_bar_spacing, 0);
    lv_obj_set_style_border_width(memory_row, 0, 0);
    lv_obj_set_flex_grow(memory_row, 0);
    lv_obj_set_style_bg_color(memory_row, bg_color, 0);
    lv_obj_set_flex_align(memory_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(memory_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *memory_icon = lv_label_create(memory_row);
    lv_obj_set_style_text_font(memory_icon, &bootstrap_icons_80, 0);
    lv_obj_set_style_text_color(memory_icon, text_color, 0);
    lv_label_set_text(memory_icon, "\U0000F6E3");
    lv_obj_set_width(memory_icon, icon_width);

    memory_bar = lv_bar_create(memory_row);
    lv_bar_set_range(memory_bar, 0, 100);
    lv_bar_set_value(memory_bar, 0, LV_ANIM_OFF);
    lv_obj_set_flex_grow(memory_bar, 1);
    lv_obj_set_height(memory_bar, bar_height);
    lv_obj_add_style(memory_bar, &memory_bar_style_main, LV_PART_MAIN);
    lv_obj_add_style(memory_bar, &memory_bar_style_indic, LV_PART_INDICATOR);

    memory_label = lv_label_create(memory_bar);
    lv_label_set_text(memory_label, "0%");
    lv_obj_set_style_text_color(memory_label, text_color, 0);
    lv_obj_set_style_text_font(memory_label, &comfortaa_40, 0);
    lv_obj_center(memory_label);

    // GPU Row
    lv_obj_t *gpu_row = lv_obj_create(main_container);
    lv_obj_set_size(gpu_row, lv_pct(100), row_height);
    lv_obj_set_flex_flow(gpu_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(gpu_row, row_padding * 2, 0);
    lv_obj_set_style_pad_column(gpu_row, icon_bar_spacing, 0);
    lv_obj_set_style_border_width(gpu_row, 0, 0);
    lv_obj_set_flex_grow(gpu_row, 0);
    lv_obj_set_style_bg_color(gpu_row, bg_color, 0);
    lv_obj_set_flex_align(gpu_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(gpu_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *gpu_icon = lv_label_create(gpu_row);
    lv_obj_set_style_text_font(gpu_icon, &bootstrap_icons_80, 0);
    lv_obj_set_style_text_color(gpu_icon, text_color, 0);
    lv_label_set_text(gpu_icon, "\U0000F6E2");
    lv_obj_set_width(gpu_icon, icon_width);

    gpu_bar = lv_bar_create(gpu_row);
    lv_bar_set_range(gpu_bar, 0, 100);
    lv_bar_set_value(gpu_bar, 0, LV_ANIM_OFF);
    lv_obj_set_flex_grow(gpu_bar, 1);
    lv_obj_set_height(gpu_bar, bar_height);
    lv_obj_add_style(gpu_bar, &gpu_bar_style_main, LV_PART_MAIN);
    lv_obj_add_style(gpu_bar, &gpu_bar_style_indic, LV_PART_INDICATOR);

    gpu_label = lv_label_create(gpu_bar);
    lv_label_set_text(gpu_label, "0%");
    lv_obj_set_style_text_color(gpu_label, text_color, 0);
    lv_obj_set_style_text_font(gpu_label, &comfortaa_40, 0);
    lv_obj_center(gpu_label);

    // Network Row
    lv_obj_t *network_row = lv_obj_create(main_container);
    lv_obj_set_size(network_row, lv_pct(100), row_height);
    lv_obj_set_flex_flow(network_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(network_row, row_padding * 2, 0);
    lv_obj_set_style_pad_column(network_row, row_spacing * 4, 0);
    lv_obj_set_style_border_width(network_row, 0, 0);
    lv_obj_set_flex_grow(network_row, 0);
    lv_obj_set_style_bg_color(network_row, bg_color, 0);
    lv_obj_set_flex_align(network_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(network_row, LV_OBJ_FLAG_SCROLLABLE);

    // Download container
    lv_obj_t *download_container = lv_obj_create(network_row);
    lv_obj_set_flex_flow(download_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_size(download_container, lv_pct(48), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(download_container, 0, 0);
    lv_obj_set_style_pad_column(download_container, network_icon_spacing, 0);
    lv_obj_set_style_border_width(download_container, 0, 0);
    lv_obj_set_style_bg_opa(download_container, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_align(download_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *download_icon = lv_label_create(download_container);
    lv_obj_set_style_text_font(download_icon, &bootstrap_icons_80, 0);
    lv_obj_set_style_text_color(download_icon, text_color, 0);
    lv_label_set_text(download_icon, "\U0000F30A");

    download_label = lv_label_create(download_container);
    lv_obj_set_style_text_color(download_label, text_color, 0);
    lv_obj_set_style_text_font(download_label, &comfortaa_40, 0);
    lv_label_set_text(download_label, "0 Mbps");

    // Upload container
    lv_obj_t *upload_container = lv_obj_create(network_row);
    lv_obj_set_flex_flow(upload_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_size(upload_container, lv_pct(48), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(upload_container, 0, 0);
    lv_obj_set_style_pad_column(upload_container, network_icon_spacing, 0);
    lv_obj_set_style_border_width(upload_container, 0, 0);
    lv_obj_set_style_bg_opa(upload_container, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_align(upload_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *upload_icon = lv_label_create(upload_container);
    lv_obj_set_style_text_font(upload_icon, &bootstrap_icons_80, 0);
    lv_obj_set_style_text_color(upload_icon, text_color, 0);
    lv_label_set_text(upload_icon, "\U0000F603");

    upload_label = lv_label_create(upload_container);
    lv_obj_set_style_text_color(upload_label, text_color, 0);
    lv_obj_set_style_text_font(upload_label, &comfortaa_40, 0);
    lv_label_set_text(upload_label, "0 Mbps");
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // Initialize Configuration
    bg_color = lv_color_hex(0x000000);
    text_color = lv_color_hex(0xeff1f5);
    bar_bg_color = lv_color_hex(0x4c4f69);
    bar_accent_color = lv_color_hex(0x8839ef);
    bar_warning_color = lv_color_hex(0xdf8e1d);
    bar_critical_color = lv_color_hex(0xd20f39);
    
    anim_time = 1000;
    outer_margin = 10;
    row_padding = 5;
    row_spacing = 5;
    icon_width = 100;
    bar_height = 80;
    bar_radius = 5;
    icon_bar_spacing = 15;
    network_icon_spacing = 35;
    warning_threshold = 80;
    critical_threshold = 90;
    pixel_clock = 24 * 1000 * 1000;
    buffer_lines = 60;

    pinMode(PIN_NUM_BK_LIGHT, OUTPUT);
    digitalWrite(PIN_NUM_BK_LIGHT, HIGH);

    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = pixel_clock,
            .h_res = LCD_H_RES,
            .v_res = LCD_V_RES,
            .hsync_pulse_width = 48,
            .hsync_back_porch = 88,
            .hsync_front_porch = 40,
            .vsync_pulse_width = 3,
            .vsync_back_porch = 32,
            .vsync_front_porch = 13,
            .flags = { .pclk_active_neg = 1 },
        },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = 1,
        .bounce_buffer_size_px = LCD_H_RES * 10,
        .sram_trans_align = 4,
        .psram_trans_align = 64,
        .hsync_gpio_num = PIN_NUM_HSYNC,
        .vsync_gpio_num = PIN_NUM_VSYNC,
        .de_gpio_num = PIN_NUM_DE,
        .pclk_gpio_num = PIN_NUM_PCLK,
        .disp_gpio_num = PIN_NUM_DISP_EN,
        .data_gpio_nums = {
            PIN_NUM_DATA0, PIN_NUM_DATA1, PIN_NUM_DATA2, PIN_NUM_DATA3,
            PIN_NUM_DATA4, PIN_NUM_DATA5, PIN_NUM_DATA6, PIN_NUM_DATA7,
            PIN_NUM_DATA8, PIN_NUM_DATA9, PIN_NUM_DATA10, PIN_NUM_DATA11,
            PIN_NUM_DATA12, PIN_NUM_DATA13, PIN_NUM_DATA14, PIN_NUM_DATA15,
        },
        .flags = { .fb_in_psram = 1 },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
    
    esp_lcd_rgb_panel_event_callbacks_t cbs = { .on_vsync = on_vsync_event };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, NULL));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    lv_init();

    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(LCD_H_RES * buffer_lines * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(LCD_H_RES * buffer_lines * sizeof(lv_color_t), MALLOC_CAP_DMA);

    if (!buf1 || !buf2) {
        Serial.println("ERROR: Failed to allocate DMA buffers!");
    } else {
        lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * buffer_lines);
    }

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = flush_display;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.full_refresh = 0;
    disp_drv.sw_rotate = 0;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    create_ui();
}

String msgBuffer = "";

void loop() {
    while (Serial.available() > 0) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (msgBuffer.length() > 0) {
                if (msgBuffer == "ID:ed1d2a7c8af14a27b77b1c127d806aed") {
                    Serial.println("ID:91d8141364e544e181fca2382cd6751a");
                    continue;
                }

                DynamicJsonDocument doc(512);
                DeserializationError error = deserializeJson(doc, msgBuffer);
                
                if (!error) {
                    float cpu = doc["cpu"] | 0.0;
                    float mem = doc["memory"] | 0.0;
                    float gpu = doc["gpu"] | 0.0;
                    float up = doc["upload"] | 0.0;
                    float down = doc["download"] | 0.0;
                    
                    int icpu = (int)(cpu + 0.5f);
                    lv_bar_set_value(cpu_bar, icpu, LV_ANIM_ON);
                    
                    if (icpu > critical_threshold) {
                        lv_style_set_bg_color(&cpu_bar_style_indic, bar_critical_color);
                    } else if (icpu > warning_threshold) {
                        lv_style_set_bg_color(&cpu_bar_style_indic, bar_warning_color);
                    } else {
                        lv_style_set_bg_color(&cpu_bar_style_indic, bar_accent_color);
                    }
                    lv_obj_report_style_change(&cpu_bar_style_indic);
                    
                    char cpu_str[20];
                    snprintf(cpu_str, 20, "%.2f%%", cpu);
                    lv_label_set_text(cpu_label, cpu_str);

                    int imem = (int)(mem + 0.5f);
                    lv_bar_set_value(memory_bar, imem, LV_ANIM_ON);
                    
                    if (imem > critical_threshold) {
                        lv_style_set_bg_color(&memory_bar_style_indic, bar_critical_color);
                    } else if (imem > warning_threshold) {
                        lv_style_set_bg_color(&memory_bar_style_indic, bar_warning_color);
                    } else {
                        lv_style_set_bg_color(&memory_bar_style_indic, bar_accent_color);
                    }
                    lv_obj_report_style_change(&memory_bar_style_indic);
                    
                    char memory_str[20];
                    snprintf(memory_str, 20, "%.2f%%", mem);
                    lv_label_set_text(memory_label, memory_str);
                    
                    int igpu = (int)(gpu + 0.5f);
                    lv_bar_set_value(gpu_bar, igpu, LV_ANIM_ON);
                    
                    if (igpu > critical_threshold) {
                        lv_style_set_bg_color(&gpu_bar_style_indic, bar_critical_color);
                    } else if (igpu > warning_threshold) {
                        lv_style_set_bg_color(&gpu_bar_style_indic, bar_warning_color);
                    } else {
                        lv_style_set_bg_color(&gpu_bar_style_indic, bar_accent_color);
                    }
                    lv_obj_report_style_change(&gpu_bar_style_indic);
                    
                    char gpu_str[20];
                    snprintf(gpu_str, 20, "%.2f%%", gpu);
                    lv_label_set_text(gpu_label, gpu_str);
                    
                    char upload_str[20];
                    char download_str[20];
                    snprintf(upload_str, 20, "%.2f Mbps", up);
                    snprintf(download_str, 20, "%.2f Mbps", down);
                    
                    lv_label_set_text(upload_label, upload_str);
                    lv_label_set_text(download_label, download_str);
                }
                msgBuffer = "";
            }
        } else {
            msgBuffer += c;
        }
    }
    
    lv_timer_handler();
    delay(1);
}