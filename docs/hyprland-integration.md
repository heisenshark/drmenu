# Hyprland Integration Guide

This guide explains how to build and load `hypr-liquid-glass` in Hyprland and configure keybinds for `drmenu`.

---

## 📦 1. Building the Plugin

The plugin is located in `plugins/hypr-liquid-glass`.

### Building with Nix (Recommended):
```bash
nix build path:./plugins/hypr-liquid-glass#default --out-link result-plugin
# Safely copy binary without in-place overwrite
rm -f plugins/hypr-liquid-glass/hypr-liquid-glass.so
cp result-plugin/lib/hypr-liquid-glass.so plugins/hypr-liquid-glass/hypr-liquid-glass.so
```

### Building with CMake:
```bash
cd plugins/hypr-liquid-glass
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## 🔌 2. Loading the Plugin in Hyprland

> **⚠️ Important Operational Note**:
> Never reload or overwrite in-use `.so` binaries in place while loaded in Hyprland to avoid compositor crashes. Always perform plugin loading manually.

To load the plugin manually:
```bash
hyprctl plugin load /absolute/path/to/plugins/hypr-liquid-glass/hypr-liquid-glass.so
```

To auto-load on Hyprland startup, add to `~/.config/hypr/hyprland.conf`:
```ini
exec-once = hyprctl plugin load /absolute/path/to/plugins/hypr-liquid-glass/hypr-liquid-glass.so
```

To verify the plugin is active and responsive:
```bash
hyprctl plugin list
```

---

## ⌨️ 3. Configuring `hyprland.conf` Keybinds

Add keybindings to summon `drmenu` at your mouse cursor:

```ini
# Open main application launcher menu at cursor
bind = SUPER, SPACE, exec, drmenu

# Open system power menu
bind = SUPER, ESCAPE, exec, drmenu --menu power

# Open media player menu
bind = SUPER, M, exec, drmenu --menu media
```

---

## 🔍 4. Verification & Diagnostics

`drmenu` includes a built-in diagnostic tool to test communication with Hyprland and the plugin:

```bash
drmenu --test-glass
```

Output should indicate:
```
[Glass] Socket connected to Hyprland OK.
[Glass] hypr-liquid-glass plugin responding OK.
```

---

## ❓ Is `hyprglass` used in this repository?

No. `plugins/hyprglass` is an external upstream submodule (`hyprnux/hyprglass`) and is **unused dead code** in `drmenu`. `drmenu`'s dedicated companion plugin is **`plugins/hypr-liquid-glass`**, which implements the custom `/repl` multi-pill IPC protocol, hardware mipmapped concentric blur, and isotropic SDF normal refraction.
