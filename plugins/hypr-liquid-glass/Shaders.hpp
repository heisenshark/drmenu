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

// Interleaved Gradient Noise for sub-pixel spatial dithering
float ign(vec2 p) {
    return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715))));
}

// Signed distance field for rounded rectangle
float sdRoundedBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
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

    // Normal gradient for convex refraction & chromatic dispersion
    float edgeDist = -dist;
    float edgeFactor = clamp(edgeDist / max(1.0, u_corner_radius), 0.0, 1.0);
    vec2 normP = p / max(vec2(1.0), halfSize - u_corner_radius);
    float normLen = length(normP);
    vec2 normal = normLen > 0.001 ? normP / normLen : vec2(0.0);

    // Refraction vector (bends texture coordinate near glass boundaries in screen space)
    vec2 refr = (1.0 - edgeFactor) * u_refraction_strength * (10.0 / u_resolution);
    vec2 refrUV = screenUV - normal * refr;

    // Chromatic dispersion offsets (separates R and B channels)
    vec2 chromOffset = normal * (u_chromatic_aberration * 3.0) / u_resolution * (1.0 - edgeFactor * 0.5);

    vec3 col = vec3(0.0);

    // Multi-tap Golden Ratio Vogel Spiral frosted blur with spatial dithering and Gaussian decay
    float effectiveBlur = max(0.0, u_blur_strength);
    if (effectiveBlur > 0.01) {
        vec2 rad = effectiveBlur / u_resolution;
        float dither = ign(gl_FragCoord.xy) * 6.283185307;
        const int SAMPLES = 48;
        const float GOLDEN_ANGLE = 2.399963229728653; // pi * (3.0 - sqrt(5.0))

        vec3 accum = vec3(0.0);
        float totalWeight = 0.0;

        for (int i = 0; i < SAMPLES; ++i) {
            float fi = float(i);
            float r = sqrt((fi + 0.5) / float(SAMPLES));
            float theta = fi * GOLDEN_ANGLE + dither;
            vec2 offset = vec2(cos(theta), sin(theta)) * (r * rad);

            float weight = exp(-2.2 * r * r);

            vec2 rUV = clamp(refrUV + chromOffset + offset * 1.04, 0.001, 0.999);
            vec2 gUV = clamp(refrUV + offset, 0.001, 0.999);
            vec2 bUV = clamp(refrUV - chromOffset + offset * 0.96, 0.001, 0.999);

            accum.r += texture(u_tex, rUV).r * weight;
            accum.g += texture(u_tex, gUV).g * weight;
            accum.b += texture(u_tex, bUV).b * weight;
            totalWeight += weight;
        }
        col = accum / totalWeight;
    } else {
        col.r = texture(u_tex, clamp(refrUV + chromOffset, 0.001, 0.999)).r;
        col.g = texture(u_tex, clamp(refrUV, 0.001, 0.999)).g;
        col.b = texture(u_tex, clamp(refrUV - chromOffset, 0.001, 0.999)).b;
    }

    // Vibrancy boost
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(gray), col, 1.12);

    // Milky frosted glass tint
    if (u_milky_tint.a > 0.0) {
        col = mix(col, u_milky_tint.rgb, u_milky_tint.a);
    }

    // Specular Fresnel gloss sheen
    if (u_specular_strength > 0.001) {
        vec2 lightDir = normalize(vec2(-0.35, -0.93));
        float nDotL = max(0.0, dot(normal, -lightDir));
        float topBias = pow(clamp(-p.y / halfSize.y, 0.0, 1.0), 1.5);
        float fresnel = pow(1.0 - edgeFactor, 2.0);
        float specHighlight = (topBias * 0.7 + nDotL * fresnel * 0.8) * u_specular_strength;
        col += vec3(1.0, 0.98, 0.95) * specHighlight;
    }

    // Liquid glass rim border
    if (u_border_width > 0.0 && u_border_color.a > 0.0) {
        float borderDist = abs(dist + u_border_width * 0.5) - u_border_width * 0.5;
        float borderFactor = 1.0 - smoothstep(0.0, 1.2, borderDist);
        col = mix(col, u_border_color.rgb, borderFactor * u_border_color.a);
    }

    fragColor = vec4(col, edgeAlpha);
}
)#";

} // namespace Shaders

