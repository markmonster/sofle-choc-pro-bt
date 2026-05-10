/*
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_layer_hold_rgb

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>

#include <zmk/event_manager.h>
#include <zmk/behavior.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/rgb_underglow.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define BASE_LAYER 0
#define LOWER_LAYER 1
#define RAISE_LAYER 2
#define ADJUST_LAYER 3
#define CODE_LAYER 4
#define HRM_ON_LAYER 5

struct behavior_layer_hold_rgb_config {
    struct zmk_led_hsb hold_color;
    struct zmk_led_hsb base_color;
    uint8_t layer;
    bool locking;
};

static int on_layer_hold_rgb_binding_pressed(struct zmk_behavior_binding *binding,
                                             struct zmk_behavior_binding_event event) {
    const struct behavior_layer_hold_rgb_config *cfg =
        zmk_behavior_get_binding(binding->behavior_dev)->config;

    int err = zmk_rgb_underglow_set_hsb(cfg->hold_color);
    if (err) {
        LOG_DBG("Failed to set hold RGB color: %d", err);
    }

    return zmk_keymap_layer_activate(cfg->layer, cfg->locking);
}

static int on_layer_hold_rgb_binding_released(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event) {
    const struct behavior_layer_hold_rgb_config *cfg =
        zmk_behavior_get_binding(binding->behavior_dev)->config;

    int layer_err = zmk_keymap_layer_deactivate(cfg->layer, cfg->locking);
    int rgb_err = zmk_rgb_underglow_set_hsb(cfg->base_color);

    if (rgb_err) {
        LOG_DBG("Failed to set base RGB color: %d", rgb_err);
    }

    return layer_err ? layer_err : rgb_err;
}

static const struct behavior_driver_api behavior_layer_hold_rgb_driver_api = {
    .binding_pressed = on_layer_hold_rgb_binding_pressed,
    .binding_released = on_layer_hold_rgb_binding_released,
};

static int on_layer_state_changed(const zmk_event_t *eh) {
    if (!is_zmk_layer_state_changed(eh)) {
        return -EINVAL;
    }

    const struct zmk_layer_state_changed *event = cast_zmk_layer_state_changed(eh);
    zmk_keymap_layer_index_t layer = event->layer;
    struct zmk_led_hsb color = {
        .h = 35,
        .s = 20,
        .b = 0,
    };

    switch (layer) {
    case LOWER_LAYER:
        color = (struct zmk_led_hsb){.h = 210, .s = 90, .b = 35};
        break;
    case RAISE_LAYER:
        color = (struct zmk_led_hsb){.h = 285, .s = 90, .b = 35};
        break;
    case ADJUST_LAYER:
        color = (struct zmk_led_hsb){.h = 60, .s = 90, .b = 30};
        break;
    case CODE_LAYER:
        color = (struct zmk_led_hsb){.h = 35, .s = 90, .b = 35};
        break;
    case HRM_ON_LAYER:
        color = (struct zmk_led_hsb){.h = 140, .s = 90, .b = 35};
        break;
    case BASE_LAYER:
    default:
        color = (struct zmk_led_hsb){.h = 35, .s = 20, .b = 0};
        break;
    }

    if (!event->state) {
        if (layer == CODE_LAYER || zmk_keymap_highest_layer_active() == BASE_LAYER) {
            return zmk_rgb_underglow_off();
        }

        return 0;
    }

    int err = zmk_rgb_underglow_on();
    if (err) {
        return err;
    }

    if (layer == BASE_LAYER) {
        return zmk_rgb_underglow_off();
    }

    return zmk_rgb_underglow_set_hsb(color);
}

ZMK_LISTENER(layer_rgb_listener, on_layer_state_changed)
ZMK_SUBSCRIPTION(layer_rgb_listener, zmk_layer_state_changed);

#define LHR_INST(n)                                                                               \
    static const struct behavior_layer_hold_rgb_config behavior_layer_hold_rgb_config_##n = {    \
        .hold_color =                                                                             \
            {                                                                                     \
                .h = DT_INST_PROP(n, hold_hue),                                                   \
                .s = DT_INST_PROP(n, hold_sat),                                                   \
                .b = DT_INST_PROP(n, hold_bri),                                                   \
            },                                                                                    \
        .base_color =                                                                             \
            {                                                                                     \
                .h = DT_INST_PROP(n, base_hue),                                                   \
                .s = DT_INST_PROP(n, base_sat),                                                   \
                .b = DT_INST_PROP(n, base_bri),                                                   \
            },                                                                                    \
        .layer = DT_INST_PROP(n, layer),                                                          \
        .locking = DT_INST_PROP_OR(n, locking, false),                                            \
    };                                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_layer_hold_rgb_config_##n,            \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                     \
                            &behavior_layer_hold_rgb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LHR_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
