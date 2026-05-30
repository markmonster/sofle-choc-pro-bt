/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/battery.h>
#include <zmk/display.h>
#include "status.h"
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/wpm_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/keymap.h>
#include <zmk/wpm.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

struct output_status_state {
    struct zmk_endpoint_instance selected_endpoint;
    int active_profile_index;
    bool active_profile_connected;
    bool active_profile_bonded;
};

struct layer_status_state {
    zmk_keymap_layer_index_t index;
    const char *label;
};

struct wpm_status_state {
    uint8_t wpm;
};

static void rotate_quote_canvas(lv_obj_t *canvas, lv_color_t cbuf[]) {
    static lv_color_t cbuf_tmp[QUOTE_SOURCE_CANVAS_WIDTH * QUOTE_SOURCE_CANVAS_HEIGHT];
    memcpy(cbuf_tmp, cbuf, sizeof(cbuf_tmp));

    lv_img_dsc_t img;
    img.data = (void *)cbuf_tmp;
    img.header.cf = LV_IMG_CF_TRUE_COLOR;
    img.header.w = QUOTE_SOURCE_CANVAS_WIDTH;
    img.header.h = QUOTE_SOURCE_CANVAS_HEIGHT;

    lv_canvas_fill_bg(canvas, LVGL_BACKGROUND, LV_OPA_COVER);
#ifdef CONFIG_NICE_VIEW_DISP_ROTATE_180
    lv_canvas_transform(canvas, &img, -900, LV_IMG_ZOOM_NONE, 15, -16,
                        QUOTE_SOURCE_CANVAS_WIDTH / 2, QUOTE_SOURCE_CANVAS_HEIGHT / 2, true);
#else
    lv_canvas_transform(canvas, &img, 900, LV_IMG_ZOOM_NONE, 13, -15,
                        QUOTE_SOURCE_CANVAS_WIDTH / 2, QUOTE_SOURCE_CANVAS_HEIGHT / 2, true);
#endif
}

static void draw_quote_band(lv_obj_t *widget, lv_color_t cbuf[]) {
    lv_obj_t *source_canvas = lv_obj_get_child(widget, 4);
    lv_obj_t *canvas = lv_obj_get_child(widget, 3);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_unscii_8, LV_TEXT_ALIGN_CENTER);

    lv_canvas_draw_rect(source_canvas, 0, 0, QUOTE_SOURCE_CANVAS_WIDTH, QUOTE_SOURCE_CANVAS_HEIGHT,
                        &rect_black_dsc);

#if 1
    // Diagnostic probe: enable to map the portrait quote area with border and L/C/R markers.
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);

    lv_point_t border_top[] = {{0, 0}, {QUOTE_SOURCE_CANVAS_WIDTH - 1, 0}};
    lv_point_t border_right[] = {{QUOTE_SOURCE_CANVAS_WIDTH - 1, 0},
                                 {QUOTE_SOURCE_CANVAS_WIDTH - 1, QUOTE_SOURCE_CANVAS_HEIGHT - 1}};
    lv_point_t border_bottom[] = {{0, QUOTE_SOURCE_CANVAS_HEIGHT - 1},
                                  {QUOTE_SOURCE_CANVAS_WIDTH - 1,
                                   QUOTE_SOURCE_CANVAS_HEIGHT - 1}};
    lv_point_t border_left[] = {{0, 0}, {0, QUOTE_SOURCE_CANVAS_HEIGHT - 1}};
    lv_point_t center_vertical[] = {{QUOTE_SOURCE_CANVAS_WIDTH / 2, 0},
                                    {QUOTE_SOURCE_CANVAS_WIDTH / 2,
                                     QUOTE_SOURCE_CANVAS_HEIGHT - 1}};
    lv_point_t center_horizontal[] = {{0, QUOTE_SOURCE_CANVAS_HEIGHT / 2},
                                      {QUOTE_SOURCE_CANVAS_WIDTH - 1,
                                       QUOTE_SOURCE_CANVAS_HEIGHT / 2}};

    lv_canvas_draw_line(source_canvas, border_top, 2, &line_dsc);
    lv_canvas_draw_line(source_canvas, border_right, 2, &line_dsc);
    lv_canvas_draw_line(source_canvas, border_bottom, 2, &line_dsc);
    lv_canvas_draw_line(source_canvas, border_left, 2, &line_dsc);
    lv_canvas_draw_line(source_canvas, center_vertical, 2, &line_dsc);
    lv_canvas_draw_line(source_canvas, center_horizontal, 2, &line_dsc);

    lv_canvas_draw_text(source_canvas, 1, 4, 20, &label_dsc, "L");
    lv_canvas_draw_text(source_canvas, 24, 4, 20, &label_dsc, "C");
    lv_canvas_draw_text(source_canvas, 47, 4, 20, &label_dsc, "R");
    lv_canvas_draw_text(source_canvas, 1, 84, 20, &label_dsc, "L");
    lv_canvas_draw_text(source_canvas, 24, 84, 20, &label_dsc, "C");
    lv_canvas_draw_text(source_canvas, 47, 84, 20, &label_dsc, "R");
#else
    lv_canvas_draw_text(source_canvas, 0, 6, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "If you");
    lv_canvas_draw_text(source_canvas, 0, 14, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "can't");
    lv_canvas_draw_text(source_canvas, 0, 22, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "change");
    lv_canvas_draw_text(source_canvas, 0, 30, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "the cards");
    lv_canvas_draw_text(source_canvas, 0, 38, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "you are");
    lv_canvas_draw_text(source_canvas, 0, 46, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "dealt,");
    lv_canvas_draw_text(source_canvas, 0, 54, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "change");
    lv_canvas_draw_text(source_canvas, 0, 62, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "how you");
    lv_canvas_draw_text(source_canvas, 0, 70, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "play");
    lv_canvas_draw_text(source_canvas, 0, 78, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "your");
    lv_canvas_draw_text(source_canvas, 0, 86, QUOTE_SOURCE_CANVAS_WIDTH, &label_dsc, "hand.");
#endif

    rotate_quote_canvas(canvas, cbuf);
}

static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);

    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // Draw battery
    draw_battery(canvas, state);

    // Draw output status
    char output_text[10] = {};

    switch (state->selected_endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        strcat(output_text, LV_SYMBOL_USB);
        break;
    case ZMK_TRANSPORT_BLE:
        if (state->active_profile_bonded) {
            if (state->active_profile_connected) {
                strcat(output_text, LV_SYMBOL_WIFI);
            } else {
                strcat(output_text, LV_SYMBOL_CLOSE);
            }
        } else {
            strcat(output_text, LV_SYMBOL_SETTINGS);
        }
        break;
    }

    lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc, output_text);

    lv_draw_rect_dsc_t rect_white_dsc;
    init_rect_dsc(&rect_white_dsc, LVGL_FOREGROUND);
    lv_draw_label_dsc_t label_dsc_bt;
    init_label_dsc(&label_dsc_bt, LVGL_BACKGROUND, &lv_font_montserrat_10, LV_TEXT_ALIGN_CENTER);

    // Draw the active Bluetooth slot in the top section.
    int active_slot = state->active_profile_index + 1;
    int indicator_x = 34;
    int indicator_y = 23;
    int pill_left = indicator_x - 17;
    int pill_top = indicator_y - 7;
    rect_white_dsc.radius = 7;

    lv_canvas_draw_rect(canvas, pill_left, pill_top, 34, 14, &rect_white_dsc);

    char label[8];
    snprintf(label, sizeof(label), "BT %d", active_slot);
    lv_canvas_draw_text(canvas, 0, pill_top + 1, CANVAS_SIZE, &label_dsc_bt, label);

#if 0
    // Draw WPM
    lv_draw_label_dsc_t label_dsc_wpm;
    init_label_dsc(&label_dsc_wpm, LVGL_FOREGROUND, &lv_font_unscii_8, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_white_dsc;
    init_rect_dsc(&rect_white_dsc, LVGL_FOREGROUND);
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);

    lv_canvas_draw_rect(canvas, 0, 21, 68, 42, &rect_white_dsc);
    lv_canvas_draw_rect(canvas, 1, 22, 66, 40, &rect_black_dsc);

    char wpm_text[6] = {};
    snprintf(wpm_text, sizeof(wpm_text), "%d", state->wpm[9]);
    lv_canvas_draw_text(canvas, 42, 52, 24, &label_dsc_wpm, wpm_text);

    int max = 0;
    int min = 256;

    for (int i = 0; i < 10; i++) {
        if (state->wpm[i] > max) {
            max = state->wpm[i];
        }
        if (state->wpm[i] < min) {
            min = state->wpm[i];
        }
    }

    int range = max - min;
    if (range == 0) {
        range = 1;
    }

    lv_point_t points[10];
    for (int i = 0; i < 10; i++) {
        points[i].x = 2 + i * 7;
        points[i].y = 60 - (state->wpm[i] - min) * 36 / range;
    }
    lv_canvas_draw_line(canvas, points, 10, &line_dsc);
#endif

    // Rotate canvas
    rotate_canvas(canvas, cbuf);
}

static void draw_middle(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 1);
    (void)state;

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // Rotate canvas
    rotate_canvas(canvas, cbuf);
}

static void draw_bottom(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 2);

    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_10, LV_TEXT_ALIGN_CENTER);

    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_SIZE, CANVAS_SIZE, &rect_black_dsc);

    // Draw layer
    if (state->layer_label == NULL || strlen(state->layer_label) == 0) {
        char text[10] = {};

        sprintf(text, "LAYER %i", state->layer_index);

        lv_canvas_draw_text(canvas, 0, 5, 68, &label_dsc, text);
    } else {
        lv_canvas_draw_text(canvas, 0, 5, 68, &label_dsc, state->layer_label);
    }

    // Rotate canvas
    rotate_canvas(canvas, cbuf);
}

static void set_battery_status(struct zmk_widget_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

    widget->state.battery = state.level;

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}

static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    const struct zmk_battery_state_changed *ev = as_zmk_battery_state_changed(eh);

    return (struct battery_status_state){
        .level = (ev != NULL) ? ev->state_of_charge : zmk_battery_state_of_charge(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)

ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */

static void set_output_status(struct zmk_widget_status *widget,
                              const struct output_status_state *state) {
    widget->state.selected_endpoint = state->selected_endpoint;
    widget->state.active_profile_index = state->active_profile_index;
    widget->state.active_profile_connected = state->active_profile_connected;
    widget->state.active_profile_bonded = state->active_profile_bonded;

    draw_top(widget->obj, widget->cbuf, &widget->state);
    draw_middle(widget->obj, widget->cbuf2, &widget->state);
}

static void output_status_update_cb(struct output_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_output_status(widget, &state); }
}

static struct output_status_state output_status_get_state(const zmk_event_t *_eh) {
    return (struct output_status_state){
        .selected_endpoint = zmk_endpoints_selected(),
        .active_profile_index = zmk_ble_active_profile_index(),
        .active_profile_connected = zmk_ble_active_profile_is_connected(),
        .active_profile_bonded = !zmk_ble_active_profile_is_open(),
    };
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_output_status, struct output_status_state,
                            output_status_update_cb, output_status_get_state)
ZMK_SUBSCRIPTION(widget_output_status, zmk_endpoint_changed);

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_output_status, zmk_usb_conn_state_changed);
#endif
#if defined(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(widget_output_status, zmk_ble_active_profile_changed);
#endif

static void set_layer_status(struct zmk_widget_status *widget, struct layer_status_state state) {
    widget->state.layer_index = state.index;
    widget->state.layer_label = state.label;

    draw_bottom(widget->obj, widget->cbuf3, &widget->state);
}

static void layer_status_update_cb(struct layer_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_layer_status(widget, state); }
}

static struct layer_status_state layer_status_get_state(const zmk_event_t *eh) {
    zmk_keymap_layer_index_t index = zmk_keymap_highest_layer_active();
    return (struct layer_status_state){
        .index = index, .label = zmk_keymap_layer_name(zmk_keymap_layer_index_to_id(index))};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_layer_status, struct layer_status_state, layer_status_update_cb,
                            layer_status_get_state)

ZMK_SUBSCRIPTION(widget_layer_status, zmk_layer_state_changed);

static void set_wpm_status(struct zmk_widget_status *widget, struct wpm_status_state state) {
    for (int i = 0; i < 9; i++) {
        widget->state.wpm[i] = widget->state.wpm[i + 1];
    }
    widget->state.wpm[9] = state.wpm;

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_status(widget, state); }
}

struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    return (struct wpm_status_state){.wpm = zmk_wpm_get_state()};
};

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state, wpm_status_update_cb,
                            wpm_status_get_state)
ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

#ifdef CONFIG_NICE_VIEW_DISP_ROTATE_180 // sets positions for default and flipped canvases
int top_pos = 0;
int middle_pos = 68;
int bottom_pos = 136;
#else
int top_pos = 92;
int middle_pos = 0;
int bottom_pos = -44;
#endif

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 160, 68);
#ifdef LV_OBJ_FLAG_OVERFLOW_VISIBLE
    lv_obj_add_flag(widget->obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
#endif
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_LEFT, top_pos, 0);
    lv_canvas_set_buffer(top, widget->cbuf, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);
    lv_obj_t *middle = lv_canvas_create(widget->obj);
    lv_obj_align(middle, LV_ALIGN_TOP_LEFT, middle_pos, 0);
    lv_canvas_set_buffer(middle, widget->cbuf2, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);
    lv_obj_t *bottom = lv_canvas_create(widget->obj);
    lv_obj_align(bottom, LV_ALIGN_TOP_LEFT, bottom_pos, 0);
    lv_canvas_set_buffer(bottom, widget->cbuf3, CANVAS_SIZE, CANVAS_SIZE, LV_IMG_CF_TRUE_COLOR);

#if 0
    // Calibration probe: enable this canvas to show the quote area as a white rectangle.
    lv_obj_t *quote = lv_canvas_create(widget->obj);
    lv_obj_align(quote, LV_ALIGN_TOP_LEFT, 34, 0);
    lv_obj_set_size(quote, QUOTE_CANVAS_WIDTH, QUOTE_CANVAS_HEIGHT);
    lv_canvas_set_buffer(quote, widget->quote_cbuf, QUOTE_CANVAS_WIDTH, QUOTE_CANVAS_HEIGHT,
                         LV_IMG_CF_TRUE_COLOR);
#endif

    lv_obj_t *quote = lv_canvas_create(widget->obj);
    lv_obj_align(quote, LV_ALIGN_TOP_LEFT, 34, 0);
    lv_obj_set_size(quote, QUOTE_CANVAS_WIDTH, QUOTE_CANVAS_HEIGHT);
    lv_canvas_set_buffer(quote, widget->quote_cbuf, QUOTE_CANVAS_WIDTH, QUOTE_CANVAS_HEIGHT,
                         LV_IMG_CF_TRUE_COLOR);

    lv_obj_t *quote_source = lv_canvas_create(widget->obj);
    lv_obj_add_flag(quote_source, LV_OBJ_FLAG_HIDDEN);
    lv_canvas_set_buffer(quote_source, widget->quote_src_cbuf, QUOTE_SOURCE_CANVAS_WIDTH,
                         QUOTE_SOURCE_CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR);
    draw_quote_band(widget->obj, widget->quote_src_cbuf);

    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_output_status_init();
    widget_layer_status_init();
    widget_wpm_status_init();

    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
