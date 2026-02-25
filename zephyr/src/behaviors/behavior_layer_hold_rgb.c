/*
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_layer_hold_rgb

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/keymap.h>
#include <zmk/rgb_underglow.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_layer_hold_rgb_config {
    struct zmk_led_hsb hold_color;
    struct zmk_led_hsb base_color;
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

    return zmk_keymap_layer_activate(binding->param1, cfg->locking);
}

static int on_layer_hold_rgb_binding_released(struct zmk_behavior_binding *binding,
                                              struct zmk_behavior_binding_event event) {
    const struct behavior_layer_hold_rgb_config *cfg =
        zmk_behavior_get_binding(binding->behavior_dev)->config;

    int layer_err = zmk_keymap_layer_deactivate(binding->param1, cfg->locking);
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
        .locking = DT_INST_PROP_OR(n, locking, false),                                            \
    };                                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &behavior_layer_hold_rgb_config_##n,            \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                     \
                            &behavior_layer_hold_rgb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LHR_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
