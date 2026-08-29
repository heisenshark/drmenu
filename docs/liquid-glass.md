# Hyprland Liquid Glass (`hypr-liquid-glass`)

`hypr-liquid-glass` is a native C++ plugin for the Hyprland Wayland compositor that provides high-performance hardware-accelerated liquid glass, refraction, chromatic aberration, and frosted Gaussian blur underlays directly underneath `drmenu` items.

---

## 🏛️ Compositor Architecture

```
[ Hyprland Framebuffer Scene (Desktop / Apps) ]
                      │
                      ▼ (sampleCleanBackground)
 [ Dedicated g_pUnderlayFB Framebuffer Blit ]
                      │
                      ▼ (glGenerateMipmap)
 [ GPU Hardware Mipmap Pyramids (Mip 0 .. 4) ]
                      │
                      ▼ (hypr-liquid-glass Fragment Shader)
 [ 1. Refraction Lens Warping (SDF Normal Gradient) ]
                      │
                      ▼
 [ 2. Chromatic Dispersion (R/G/B Separation) ]
                      │
                      ▼
 [ 3. 49-Tap Concentric Gaussian Diffusion (textureLod) ]
                      │
                      ▼
 [ 4. Apple CAFilterColorSaturate Vibrancy Boost ]
                      │
                      ▼
 [ 5. Inner Edge Ambient Occlusion ]
                      │
                      ▼
 [ 6. Substrate Translucent Acrylic Tint ]
                      │
                      ▼
 [ 7. Fresnel Specular Surface Reflection ]
                      │
                      ▼
 [ 8. Anti-Aliased Outer Boundary Blit to Monitor ]
```

---

## 🔬 Core Shader Features

### 1. Isotropic SDF Gradient Normal (`getSDFNormal`)
Unlike approximations that squash coordinates on oblong pills, `hypr-liquid-glass` computes the exact mathematical surface normal from the gradient of the rounded box Signed Distance Field (SDF):
$$\mathbf{N}(p) = \frac{\nabla \text{sdRoundedBox}(p)}{\|\nabla \text{sdRoundedBox}(p)\|}$$
- **Semicircular caps**: Radial refraction bending light horizontally left and right.
- **Top / bottom edges**: Pure vertical refraction.
- **Corners**: Smooth 45° dispersion.

### 2. Hardware Mipmapped Frosted Blur
High-radius blurs (e.g. 24px - 48px) avoid spatial gap artifacts and noise by generating GPU mipmaps (`glGenerateMipmap`) on the background capture texture and sampling continuous levels of detail via `textureLod`:
$$\text{LOD} = \text{clamp}\left(\log_2(\text{effectiveBlur} \times 0.18), 0.0, 3.5\right)$$
A 49-tap concentric Gaussian kernel with staggered angles convolutes the downsampled texture with Gaussian weighting ($e^{-2.2 r^2}$), achieving ultra-deep, silky frosted blur without any TV static or grain.

### 3. Directional Fresnel Specular Sheen
A directional key light vector combined with grazing-angle Fresnel reflection replicates physical glass optics:
$$\text{specHighlight} = \left(\text{topBias} \times 0.70 + (\mathbf{N} \cdot \mathbf{L}) \times \text{fresnel} \times 0.80\right) \times u\_specular\_strength$$

### 4. Dynamic Hover Optics & Live Interpolation
Optics are passed individually per pill. During mouse hover transitions, `drmenu` interpolates blur, refraction, dispersion, and specular gloss across a smooth cubic curve (`Easing.OutCubic`, 110ms).

---

## 📡 IPC Protocol

`drmenu` communicates with `hypr-liquid-glass` over Hyprland's `/repl` UNIX socket:

```
/repl return (hl.plugin and hl.plugin.liquid_glass and hl.plugin.liquid_glass.set_pills) and hl.plugin.liquid_glass.set_pills([[<PILL_DATA>]])
```

Each pill record is delimited by semicolons:
```
<x> <y> <w> <h> <radius> <blur> <refr> <chrom> <spec> <milkyR> <milkyG> <milkyB> <milkyA> <borderR> <borderG> <borderB> <borderA> <borderW>;
```
All floating point values are dot-decimal formatted.
