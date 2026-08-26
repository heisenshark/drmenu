#pragma once

#include <string>

namespace Shaders {

inline const std::string LIQUID_GLASS_VERT = R"#(
#version 300 es
precision highp float;
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_texcoord;

out vec2 v_texcoord;
out vec2 v_screen_uv;

void main() {
    v_texcoord = a_texcoord;
    v_screen_uv = vec2(a_pos.x * 0.5 + 0.5, a_pos.y * 0.5 + 0.5);
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)#";

inline const std::string LIQUID_GLASS_FRAG = R"#(
#version 300 es
precision highp float;

in vec2 v_texcoord;
in vec2 v_screen_uv;
out vec4 fragColor;

uniform sampler2D u_tex;
uniform vec2 u_resolution;
uniform vec4 u_pill_rect; // x, y, w, h in screen pixels (y is top-down)
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

    // Normal gradient for convex refraction & chromatic dispersion
    float edgeDist = -dist;
    float edgeFactor = clamp(edgeDist / max(1.0, u_corner_radius), 0.0, 1.0);
    vec2 normal = normalize(p / max(vec2(1.0), halfSize - u_corner_radius));

    // Refraction vector (bends texture coordinate near glass boundaries)
    float refr = (1.0 - edgeFactor) * u_refraction_strength * (10.0 / u_resolution.x);
    vec2 refrUV = v_screen_uv - normal * refr;

    // Chromatic dispersion offsets (separates R and B channels)
    vec2 chromOffset = normal * (u_chromatic_aberration * 4.0) / u_resolution.x * (1.0 - edgeFactor * 0.5);

    vec3 col = vec3(0.0);

    // Multi-tap Poisson frosted blur with chromatic dispersion
    if (u_blur_strength > 0.1) {
        float bStep = (u_blur_strength * 0.40) / u_resolution.x;

        // Red channel (shifted)
        vec2 rUV = refrUV + chromOffset;
        col.r += texture(u_tex, rUV).r * 0.28;
        col.r += texture(u_tex, rUV + vec2( bStep * 1.4,  bStep * 0.8)).r * 0.18;
        col.r += texture(u_tex, rUV + vec2(-bStep * 1.4,  bStep * 0.8)).r * 0.18;
        col.r += texture(u_tex, rUV + vec2( bStep * 0.8, -bStep * 1.4)).r * 0.18;
        col.r += texture(u_tex, rUV + vec2(-bStep * 0.8, -bStep * 1.4)).r * 0.18;
        col.r += texture(u_tex, rUV + vec2(-bStep * 0.8, -bStep * 1.4)).r * 0.18;

        // Green channel (center)
        col.g += texture(u_tex, refrUV).g * 0.28;
        col.g += texture(u_tex, refrUV + vec2( bStep * 1.4,  0.0)).g * 0.18;
        col.g += texture(u_tex, refrUV + vec2(-bStep * 1.4,  0.0)).g * 0.18;
        col.g += texture(u_tex, refrUV + vec2( 0.0,  bStep * 1.4)).g * 0.18;
        col.g += texture(u_tex, refrUV + vec2( 0.0, -bStep * 1.4)).g * 0.18;

        // Blue channel (counter-shifted)
        vec2 bUV = refrUV - chromOffset;
        col.b += texture(u_tex, bUV).b * 0.28;
        col.b += texture(u_tex, bUV + vec2( bStep * 1.4,  bStep * 0.8)).b * 0.18;
        col.b += texture(u_tex, bUV + vec2(-bStep * 1.4,  bStep * 0.8)).b * 0.18;
        col.b += texture(u_tex, bUV + vec2( bStep * 0.8, -bStep * 1.4)).b * 0.18;
        col.b += texture(u_tex, bUV + vec2(-bStep * 0.8, -bStep * 1.4)).b * 0.18;
    } else {
        col.r = texture(u_tex, refrUV + chromOffset).r;
        col.g = texture(u_tex, refrUV).g;
        col.b = texture(u_tex, refrUV - chromOffset).b;
    }

    // Vibrancy & saturation boost
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(gray), col, 1.25);

    // Milky frosted glass tint
    if (u_milky_tint.a > 0.0) {
        col = mix(col, u_milky_tint.rgb, u_milky_tint.a);
    }

    // Specular Fresnel gloss sheen on top edge
    float specular = clamp((0.5 - p.y / halfSize.y), 0.0, 1.0) * (1.0 - edgeFactor) * u_specular_strength;
    col += vec3(specular * 0.35);

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

