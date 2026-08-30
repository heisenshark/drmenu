# Configuration Reference

`drmenu` is configured using `~/.config/drmenu/config.json`. All settings can be defined globally or overridden per-submenu.

Both `camelCase` and `snake_case` keys are automatically recognized.

---

## 🎨 Theme & Base Settings

| Option | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `theme` | `string` | `"liquid-glass"` | Name of built-in theme (e.g. `liquid-glass`, `apple-glass`, `visionos`, `blender`, etc.) |
| `layout` | `string` | `"pills"` | Menu layout mode: `"pills"` (floating pills) or `"pie"` (segmented circle) |
| `radiusDistance` | `number` | `185` | Distance in pixels from origin to menu items |
| `fontFamily` | `string` | `"SF Pro Display, Inter, Sans"` | Font family for labels and key badges |
| `fontSize` / `font_size` | `number` | `13` | Base font pixel size for pill labels |
| `iconSize` / `icon_size` | `number` | `22` | Pixel width & height for application/system icons |
| `showTextShadow` / `text_shadow` | `bool` | `true` | Enable micro-shadow ambient contours behind text and icons for readability on white backgrounds |
| `textShadowColor` / `text_shadow_color` | `string` | `"#45000000"` | Shadow color for text micro-shadows |

---

## 💎 Liquid Glass & Optical Parameters

These parameters control the GPU shader when `hypr-liquid-glass` is loaded in Hyprland:

| Option | Type | Default | Recommended | Description |
| :--- | :--- | :--- | :--- | :--- |
| `glass` / `useGlass` | `bool` | `true` (for glass themes) | `true` | Enable Hyprland compositor liquid glass overlay |
| `blur` / `blurStrength` | `number` | `22.0` | `18.0 - 48.0` | Frosted diffusion blur radius in pixels |
| `refractionStrength` | `number` | `0.85` | `0.60 - 1.20` | Convex lens curvature and background warping |
| `chromaticAberration` | `number` | `1.4` | `0.8 - 2.5` | Prism dispersion / RGB wavelength split intensity |
| `specularStrength` | `number` | `0.75` | `0.50 - 1.20` | Front air-glass Fresnel reflection & top highlight |

---

## ⚡ Dynamic Hover Optics & Transitions (Interpolated)

When hovering over a pill, optics and geometry smoothly animate from base values to hover values (`Easing.OutCubic`):

| Option | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `hoverDuration` / `hover_duration` | `number` | `110` | Transition duration in milliseconds from idle state to hover state |
| `hoverScaleDuration` / `hover_scale_duration` | `number` | `90` | Scale spring animation duration in milliseconds (`Easing.OutBack`) |
| `blurHover` / `blur_hover` | `number` | `blur` | Blur radius on the hovered pill |
| `refractionHover` / `refraction_hover` | `number` | `refractionStrength` | Lens magnification on the hovered pill |
| `chromaticHover` / `chromatic_hover` | `number` | `chromaticAberration` | Prism dispersion on the hovered pill |
| `specularHover` / `specular_hover` | `number` | `specularStrength` | Fresnel gloss highlight on the hovered pill |

---

## 🔲 Pill Geometry & Borders

| Option | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `pillHeight` | `number` | `44` | Height of pill in pixels |
| `pillRadius` | `number` | `22` | Corner radius (`height / 2` for pill semicircle caps) |
| `pillColor` | `string` | `"#25121218"` | Translucent substrate tint (`#AARRGGBB` or `"transparent"`) |
| `pillHoverColor` | `string` | `"#4520202c"` | Substrate tint on hover |
| `pillBorderColor` | `string` | `"#45ffffff"` | Specular rim border color |
| `pillBorderHoverColor` | `string` | `"#b0ffffff"` | Specular rim border color on hover |
| `borderWidth` | `number` | `1.0` | Base rim border width in pixels |
| `borderHoverWidth` | `number` | `2.0` | Rim border width on hover |

---

## 🗂️ Submenu Accents & Indicators

| Option | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `showSubmenuAccent` | `bool` | `false` (in liquid-glass) | Set to `false` to remove all colored submenu tints |
| `submenuAccent` | `string` | `"transparent"` | Highlight color for submenu items (`"transparent"` to disable) |
| `showSubmenuIndicator` | `bool` | `true` | Show small `▶` chevron indicating a submenu item |
| `pillSubmenuColor` | `string` | `pillColor` | Custom substrate tint for submenu items |
| `pillSubmenuBorder` | `string` | `pillBorderColor` | Custom border color for submenu items |

---

## 🎯 Center Origin Disc

| Option | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `centerRadius` | `number` | `18` | Radius of center glass disc |
| `centerColor` | `string` | `"transparent"` | Background fill of center disc |
| `centerBorder` | `string` | `"#45ffffff"` | Rim border of center disc |
| `centerBorderHover` | `string` | `"transparent"` | Border color when hovered |
| `centerDotColor` | `string` | `"#60ffffff"` | Idle translucent center indicator dot |
| `centerDotHoverColor` | `string` | `"#a0ffffff"` | Hovered center indicator dot |

---

## 📄 Example `config.json`

```json
{
  "theme": "liquid-glass",
  "radiusDistance": 185,
  "fontSize": 13,
  "blur": 22.0,
  "blurHover": 36.0,
  "refractionStrength": 0.85,
  "refractionHover": 1.25,
  "chromaticAberration": 1.4,
  "chromaticHover": 2.2,
  "specularStrength": 0.75,
  "specularHover": 1.15,
  "showSubmenuAccent": false,
  "showNumberBadges": true
}
```
