# nice!view

The nice!view is a low-power, high refresh rate display meant to replace I2C OLEDs traditionally used.

This shield requires that an `&nice_view_spi` labeled SPI bus is provided with _at least_ MOSI, SCK, and CS pins defined.

## Disable custom widget

The nice!view shield includes a custom vertical widget. To use the built-in ZMK one, add the following item to your `.conf` file:

```
CONFIG_ZMK_DISPLAY_STATUS_SCREEN_BUILT_IN=y
CONFIG_ZMK_LV_FONT_DEFAULT_SMALL_MONTSERRAT_26=y
CONFIG_LV_FONT_DEFAULT_MONTSERRAT_26=y
```

## Rotate screen

You can rotate the custom widget by 180 degrees. To do so, add the following item to your `.conf` file:

```
CONFIG_NICE_VIEW_DISP_ROTATE_180=y
```

## Widget coordinates

The custom status widgets are laid out in their local canvas coordinates first, then rotated into the final portrait view.

That means:

- `x` and `y` in `status.c` and `peripheral_status.c` describe the unrotated canvas space
- the visible on-screen position is affected again by `rotate_canvas()`
- if a label or pill seems to move the wrong way, the fix is often on the opposite axis from what the screen view suggests

Keep this in mind when adjusting the BT pill, layer label, or footer artwork so we do not keep chasing the portrait display as if it were the source layout.
