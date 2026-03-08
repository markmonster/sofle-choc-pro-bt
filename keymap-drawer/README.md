# keymap-drawer container (Podman Desktop)

Build the reusable image:

```powershell
podman build -t keymap-drawer-cli -f keymap-drawer/Dockerfile .
```

Parse a ZMK keymap:

```powershell
podman run --rm -v "${PWD}:/work" -w /work keymap-drawer-cli -c keymap-drawer/config.yaml parse -z config/sofle_choc_pro.keymap -o keymap-drawer/sofle.yaml
```

Draw SVG:

```powershell
podman run --rm -v "${PWD}:/work" -w /work keymap-drawer-cli -c keymap-drawer/config.yaml draw keymap-drawer/sofle.yaml --dts-layout boards/arm/sofle_choc_pro/sofle_choc_pro-layouts.dtsi -o keymap-drawer/sofle.svg
```

Draw only selected layers (optional), e.g. `lower` + `raise`:

```powershell
podman run --rm -v "${PWD}:/work" -w /work keymap-drawer-cli -c keymap-drawer/config.yaml draw keymap-drawer/sofle.yaml --dts-layout boards/arm/sofle_choc_pro/sofle_choc_pro-layouts.dtsi -s lower raise -o keymap-drawer/sofle-lower-raise.svg
```

Why the extra `--dts-layout` flag?

- `sofle_choc_pro` is a custom ZMK board, so `keymap-drawer` cannot find it in its built-in keyboard list.
- Passing your local layout DTSI file tells `keymap-drawer` exactly how to place keys.

If your layout file defines multiple layouts, add `-l <layout_name>` to the draw command.

Optionally pin version at build time:

```powershell
podman build --build-arg KEYMAP_DRAWER_VERSION=keymap-drawer==<version> -t keymap-drawer-cli -f keymap-drawer/Dockerfile .
```

If your Podman machine cannot resolve `${PWD}` correctly on Windows, use an absolute path instead:

```powershell
podman run --rm -v "D:/Sources/sofle-choc-pro-bt:/work" -w /work keymap-drawer-cli --help
```
