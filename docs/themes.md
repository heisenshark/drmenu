# Built-In Themes & Styles

`drmenu` comes with a collection of tuned presets designed for both physical liquid glass rendering and classic solid/acrylic styling.

---

## 💎 Liquid Glass Presets

| Theme Name | Blur | Refraction | Chromatic | Specular | Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `liquid-glass` / `liquid` | `22.0` | `0.85` | `1.4` | `0.75` | Default dark liquid glass with translucent dark substrate and white specular rims. Submenu accents disabled for pure minimalism. |
| `liquid-glass-light` / `liquid-light` | `20.0` | `0.80` | `1.3` | `0.90` | Light-mode liquid glass with high specular gloss and crisp white/accent borders. |
| `apple-glass` / `macos-glass` | `24.0` | `0.90` | `1.4` | `0.85` | macOS Tahoe / Sonoma style frosted glass with SF Pro typography and deep blur. |
| `apple-glass-light` / `macos-light` | `22.0` | `0.80` | `1.2` | `0.95` | iOS / macOS light frosted acrylic with rich vibrancy. |
| `visionos` / `vision-glass` | `28.0` | `0.95` | `1.6` | `0.90` | visionOS spatial glass with large blur radius, high dispersion, and electric cyan accents. |
| `apple-lens` / `lens-dark` | `0.0` | `1.15` | `1.8` | `0.90` | Pure optical clear crystal lens without blur; focuses on intense background refraction and rainbow edge dispersion. |
| `apple-lens-light` / `lens-light` | `0.0` | `1.10` | `1.6` | `1.00` | Light optical crystal lens with maximum specular sheen. |
| `visionos-lens` | `0.0` | `1.20` | `2.0` | `0.95` | High-curvature optical glass lens with strong prismatic dispersion. |
| `apple-glass-pie` | `24.0` | `0.85` | `1.4` | `0.80` | Segmented circular pie wheel with liquid glass backdrop. |

---

## 🎨 Solid & Acrylic Themes

For environments without the Hyprland plugin or for minimal flat aesthetics:

| Theme Name | Layout | Description |
| :--- | :--- | :--- |
| `blender` | `pie` | Blender-style pie menu with dark background, torus origin ring, and 90° mouse tracking indicator arc. |
| `pie` / `pie-dark` | `pie` | Classic segmented dark radial pie wheel. |
| `shotgun` | `pills` | Minimalist dark pill layout with orange/amber accents. |
| `nord` | `pills` | Arctic dark blue palette based on the Nord theme. |
| `catppuccin-mocha` | `pills` | Pastel dark palette with mauve and lavender accents. |
| `tokyo-night` | `pills` | Vibrant neon blue and purple nocturnal palette. |
| `dracula` | `pills` | High-contrast purple and pink theme. |
| `gruvbox-dark` | `pills` | Retro groove warm dark brown/amber palette. |
| `monochrome` | `pills` | High-contrast black and white styling. |

---

## 🛠️ Creating Custom Themes

To create a custom style, specify properties in `~/.config/drmenu/config.json`:

```json
{
  "theme": "liquid-glass",
  "blur": 30.0,
  "blurHover": 45.0,
  "refractionStrength": 1.0,
  "refractionHover": 1.35,
  "chromaticAberration": 1.8,
  "chromaticHover": 2.5,
  "specularStrength": 0.85,
  "specularHover": 1.20,
  "pillRadius": 22,
  "pillHeight": 44,
  "pillColor": "#20101018",
  "pillHoverColor": "#40202030",
  "pillBorderColor": "#40ffffff",
  "pillBorderHoverColor": "#b0ffffff",
  "borderWidth": 1.0,
  "borderHoverWidth": 2.0,
  "showSubmenuAccent": false,
  "fontFamily": "Inter, Sans",
  "fontSize": 13
}
```
