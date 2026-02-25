zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZMK_BEHAVIOR_LAYER_HOLD_RGB
  src/behaviors/behavior_layer_hold_rgb.c
)
