#pragma once

#include <string>

namespace Shaders {

inline const std::string LIQUID_GLASS_VERT = R"#(
#version 300 es
precision highp float;
layout (location = 0) in vec2 a_pos;

uniform mat3 u_proj;
out vec2 v_texcoord;

void main() {
    vec3 pos = u_proj * vec3(a_pos, 1.0);
    gl_Position = vec4(pos.xy, 0.0, 1.0);
    v_texcoord = a_pos;
}
)#";

inline const std::string LIQUID_GLASS_FRAG = R"#(
#version 300 es
precision highp float;

in vec2 v_texcoord;
out vec4 fragColor;

uniform sampler2D u_tex;
uniform vec2 u_resolution;
uniform vec4 u_pill_rect; // x, y, w, h in screen pixels
uniform float u_corner_radius;
uniform float u_blur_strength;
uniform float u_refraction_strength;
uniform float u_chromatic_aberration;
uniform float u_specular_strength;
uniform vec4 u_milky_tint;
uniform vec4 u_border_color;
uniform float u_border_width;

// Signed distance field for rounded rectangle
float sdRoundedBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

// True isotropic gradient normal of the signed distance field
vec2 getSDFNormal(vec2 p, vec2 b, float r) {
    const float eps = 0.5;
    float dx = sdRoundedBox(p + vec2(eps, 0.0), b, r) - sdRoundedBox(p - vec2(eps, 0.0), b, r);
    float dy = sdRoundedBox(p + vec2(0.0, eps), b, r) - sdRoundedBox(p - vec2(0.0, eps), b, r);
    float len = length(vec2(dx, dy));
    return len > 0.0001 ? vec2(dx, dy) / len : vec2(0.0);
}

void main() {
    vec2 pillSize = u_pill_rect.zw;
    vec2 halfSize = pillSize * 0.5;
    vec2 p = (v_texcoord - 0.5) * pillSize;

    float dist = sdRoundedBox(p, halfSize, u_corner_radius);
    
    // Smooth anti-aliased edge
    float edgeAlpha = 1.0 - smoothstep(0.0, 1.5, dist);
    if (edgeAlpha <= 0.0) {
        discard;
    }

    // Exact hardware screen pixel from rasterizer
    vec2 screenUV = gl_FragCoord.xy / u_resolution;

    // True geometric SDF normal vector (points outward from pill perimeter)
    vec2 normal = getSDFNormal(p, halfSize, u_corner_radius);

    // Convex lens curvature profile: maximum refraction at outer rim, smoothly zero at center
    float edgeDist = max(0.0, -dist);
    float edgeFactor = clamp(edgeDist / max(1.0, u_corner_radius), 0.0, 1.0);
    float lensSlope = sin((1.0 - edgeFactor) * 1.57079632679);

    // Refraction vector (bends texture coordinate in screen space along true normal)
    vec2 refr = normal * (lensSlope * u_refraction_strength * 14.0) / u_resolution;
    vec2 refrUV = screenUV - refr;

    // Chromatic dispersion offsets along true normal
    vec2 chromOffset = normal * (u_chromatic_aberration * 3.5 * lensSlope) / u_resolution;

    vec3 col = vec3(0.0);

    // Hardware-accelerated Mipmapped Gaussian Blur
    float effectiveBlur = max(0.0, u_blur_strength);
    if (effectiveBlur > 0.01) {
        float lod = clamp(log2(effectiveBlur * 0.18), 0.0, 3.5);
        vec2 rad = (effectiveBlur * 0.85) / u_resolution;

        // Center tap (weight 1.0)
        vec3 accum = vec3(
            textureLod(u_tex, clamp(refrUV + chromOffset, 0.001, 0.999), lod).r,
            textureLod(u_tex, clamp(refrUV, 0.001, 0.999), lod).g,
            textureLod(u_tex, clamp(refrUV - chromOffset, 0.001, 0.999), lod).b
        );
        float totalWeight = 1.0;

        // Ring 1: 6 samples at r = 0.22, weight = 0.88
        const int R1_COUNT = 6;
        for (int i = 0; i < R1_COUNT; ++i) {
            float a = float(i) * 1.047197551; // 2pi / 6
            vec2 offset = vec2(cos(a), sin(a)) * (0.22 * rad);
            float w = 0.88;
            accum.r += textureLod(u_tex, clamp(refrUV + chromOffset + offset * 1.02, 0.001, 0.999), lod).r * w;
            accum.g += textureLod(u_tex, clamp(refrUV + offset, 0.001, 0.999), lod).g * w;
            accum.b += textureLod(u_tex, clamp(refrUV - chromOffset + offset * 0.98, 0.001, 0.999), lod).b * w;
            totalWeight += w;
        }

        // Ring 2: 10 samples at r = 0.45, weight = 0.65
        const int R2_COUNT = 10;
        for (int i = 0; i < R2_COUNT; ++i) {
            float a = float(i) * 0.628318531 + 0.314159265; // 2pi / 10 + offset
            vec2 offset = vec2(cos(a), sin(a)) * (0.45 * rad);
            float w = 0.65;
            accum.r += textureLod(u_tex, clamp(refrUV + chromOffset + offset * 1.03, 0.001, 0.999), lod).r * w;
            accum.g += textureLod(u_tex, clamp(refrUV + offset, 0.001, 0.999), lod).g * w;
            accum.b += textureLod(u_tex, clamp(refrUV - chromOffset + offset * 0.97, 0.001, 0.999), lod).b * w;
            totalWeight += w;
        }

        // Ring 3: 14 samples at r = 0.72, weight = 0.40
        const int R3_COUNT = 14;
        for (int i = 0; i < R3_COUNT; ++i) {
            float a = float(i) * 0.448798951 + 0.157079633; // 2pi / 14 + offset
            vec2 offset = vec2(cos(a), sin(a)) * (0.72 * rad);
            float w = 0.40;
            accum.r += textureLod(u_tex, clamp(refrUV + chromOffset + offset * 1.04, 0.001, 0.999), lod).r * w;
            accum.g += textureLod(u_tex, clamp(refrUV + offset, 0.001, 0.999), lod).g * w;
            accum.b += textureLod(u_tex, clamp(refrUV - chromOffset + offset * 0.96, 0.001, 0.999), lod).b * w;
            totalWeight += w;
        }

        // Ring 4: 18 samples at r = 1.00, weight = 0.20
        const int R4_COUNT = 18;
        for (int i = 0; i < R4_COUNT; ++i) {
            float a = float(i) * 0.349065850 + 0.471238898; // 2pi / 18 + offset
            vec2 offset = vec2(cos(a), sin(a)) * (1.00 * rad);
            float w = 0.20;
            accum.r += textureLod(u_tex, clamp(refrUV + chromOffset + offset * 1.05, 0.001, 0.999), lod).r * w;
            accum.g += textureLod(u_tex, clamp(refrUV + offset, 0.001, 0.999), lod).g * w;
            accum.b += textureLod(u_tex, clamp(refrUV - chromOffset + offset * 0.95, 0.001, 0.999), lod).b * w;
            totalWeight += w;
        }

        col = accum / totalWeight;
    } else {
        col.r = texture(u_tex, clamp(refrUV + chromOffset, 0.001, 0.999)).r;
        col.g = texture(u_tex, clamp(refrUV, 0.001, 0.999)).g;
        col.b = texture(u_tex, clamp(refrUV - chromOffset, 0.001, 0.999)).b;
    }

    // ── Apple Liquid Glass Optical Filter Pipeline ─────────────────────────
    // 1. Vibrancy & Saturation Boost (Apple CAFilterColorSaturate)
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(gray), col, 1.22);

    // 2. Smart Adaptive Background Luminance Contrast (Readability over bright white windows)
    float bgLum = dot(col, vec3(0.2126, 0.7152, 0.0722));
    float adaptFactor = smoothstep(0.42, 0.88, bgLum);
    if (adaptFactor > 0.001) {
        // Intelligently attenuate over-exposure and blend in smoked contrast substrate
        vec3 darkSubstrate = col * 0.38 + vec3(0.04, 0.04, 0.08);
        col = mix(col, darkSubstrate, adaptFactor * 0.50);
        // Add subtle edge ambient darkening so glass pill boundary clearly contrasts with white background
        float edgeDarkening = smoothstep(0.0, 3.5, abs(dist));
        col *= mix(0.72, 1.0, edgeDarkening * (1.0 - adaptFactor * 0.45));
    }

    // 3. Inner Edge Ambient Occlusion (internal refractive depth)
    float innerEdgeShadow = smoothstep(0.0, 1.0, edgeFactor);
    col *= mix(0.92, 1.0, innerEdgeShadow);

    // 4. Substrate Material Tint (Frosted milk / acrylic pigment)
    if (u_milky_tint.a > 0.0) {
        col = mix(col, u_milky_tint.rgb, u_milky_tint.a);
    }

    // 5. Fresnel Surface Specular Sheen (Air-glass reflection + top rim highlight)
    if (u_specular_strength > 0.001) {
        vec2 lightDir = normalize(vec2(-0.35, -0.93));
        float nDotL = max(0.0, dot(normal, -lightDir));
        float topBias = pow(clamp(-p.y / halfSize.y, 0.0, 1.0), 1.5);
        float fresnel = pow(1.0 - edgeFactor, 2.0);
        float specHighlight = (topBias * 0.70 + nDotL * fresnel * 0.80) * u_specular_strength;
        col += vec3(1.0, 0.98, 0.95) * specHighlight;
    }

    // 5. Specular Rim / Bevel Border
    if (u_border_width > 0.0 && u_border_color.a > 0.0) {
        float borderDist = abs(dist + u_border_width * 0.5) - u_border_width * 0.5;
        float borderFactor = 1.0 - smoothstep(0.0, 1.2, borderDist);
        col = mix(col, u_border_color.rgb, borderFactor * u_border_color.a);
    }

    // 6. Anti-Aliased Outer Boundary Cutout
    fragColor = vec4(col, edgeAlpha);
}
)#";

} // namespace Shaders

