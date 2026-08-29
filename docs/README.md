# drmenu & hypr-liquid-glass Documentation

Welcome to the documentation for **drmenu** and its companion Hyprland compositor plugin **hypr-liquid-glass**.

---

## 📚 Documentation Index

1. [**Configuration Reference**](configuration.md)
   - Complete guide to `config.json` options, layout controls, typography, and optical parameters.
2. [**Liquid Glass & Compositor Architecture**](liquid-glass.md)
   - Deep dive into native Hyprland GPU framebuffer blitting, hardware mipmap pyramids, 49-tap concentric Gaussian blur, isotropic SDF normal refraction, and dynamic hover optics.
3. [**Built-In Themes & Customization**](themes.md)
   - Catalog of built-in themes (`liquid-glass`, `apple-glass`, `visionos`, `apple-lens`, `blender`, etc.) and how to create custom styles.
4. [**Hyprland Setup & Integration Guide**](hyprland-integration.md)
   - Step-by-step instructions for building the plugin, loading it into Hyprland, configuring `hyprland.conf` keybinds, and troubleshooting.

---

## 🚀 Quick Feature Highlights

- **Native Hyprland Plugin (`hypr-liquid-glass`)**: Zero-overhead compositor-level liquid glass underlay with real-time screen refraction, dispersion, and Gaussian diffusion behind individual radial menu items.
- **Isotropic 2D Refraction**: Uses mathematical Signed Distance Field (SDF) surface gradients to bend background light around pill semicircles and corners accurately.
- **Hardware Mipmapped Blur**: 49-tap concentric Gaussian kernel sampling downsampled GPU mipmap levels for deep, velvety frosted glass at any radius without grain or TV static.
- **Live Hover Transitions**: Continuous cubic interpolation of scale, blur, refraction, chromatic aberration, and specular sheen during mouse hover animations.
- **Unified & Configurable Submenus**: Easily remove or customize submenu accent colors for a pure, seamless translucent aesthetic.
- **Origin Circle in Glass**: Center pivot disc rendered directly inside the compositor's liquid glass shader with customizable interactive dot indicators.
