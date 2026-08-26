#pragma once

#include <string>

namespace Shaders {

inline const std::string LIQUID_GLASS_VERT = R"#(
#version 300 es
precision mediump float;
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_texcoord;

out vec2 v_texcoord;

void main() {
    v_texcoord = a_texcoord;
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)#";

inline const std::string LIQUID_GLASS_FRAG = R"#(
#version 300 es
precision mediump float;

in vec2 v_texcoord;
out vec4 fragColor;

uniform sampler2D u_tex;
uniform vec2 u_resolution;
uniform float u_blur_strength;
uniform float u_refraction_strength;
uniform float u_chromatic_aberration;
uniform float u_specular_strength;
uniform float u_corner_radius;

// Signed distance field for rounded rectangle
float sdRoundedBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

void main() {
    vec2 uv = v_texcoord;
    vec2 p = (uv - 0.5) * u_resolution;
    vec2 halfSize = u_resolution * 0.5;

    float dist = sdRoundedBox(p, halfSize, u_corner_radius);
    if (dist > 0.0) {
        // Discard or pass through outside
        discard;
    }

    // Normal gradient for convex refraction
    float edgeDist = -dist;
    float edgeFactor = clamp(edgeDist / 12.0, 0.0, 1.0);
    vec2 normal = normalize(p / max(vec2(1.0), halfSize - u_corner_radius));

    // Refraction vector
    float refr = (1.0 - edgeFactor) * u_refraction_strength;
    vec2 refrUV = uv - normal * refr;

    // Chromatic dispersion offsets
    vec2 chromOffset = normal * u_chromatic_aberration * (1.0 - edgeFactor * 0.5);

    vec3 col = vec3(0.0);

    // Multi-tap Poisson blur with optical dispersion
    if (u_blur_strength > 0.1) {
        float bStep = (u_blur_strength * 2.0) / u_resolution.x;

        // Red channel
        vec2 rUV = refrUV + chromOffset;
        col.r += texture(u_tex, rUV).r * 0.34;
        col.r += texture(u_tex, rUV + vec2( bStep,  bStep)).r * 0.22;
        col.r += texture(u_tex, rUV + vec2(-bStep,  bStep)).r * 0.22;
        col.r += texture(u_tex, rUV + vec2( bStep, -bStep)).r * 0.11;
        col.r += texture(u_tex, rUV + vec2(-bStep, -bStep)).r * 0.11;

        // Green channel (center)
        col.g += texture(u_tex, refrUV).g * 0.34;
        col.g += texture(u_tex, refrUV + vec2( bStep,  0.0)).g * 0.22;
        col.g += texture(u_tex, refrUV + vec2(-bStep,  0.0)).g * 0.22;
        col.g += texture(u_tex, refrUV + vec2( 0.0,  bStep)).g * 0.11;
        col.g += texture(u_tex, refrUV + vec2( 0.0, -bStep)).g * 0.11;

        // Blue channel
        vec2 bUV = refrUV - chromOffset;
        col.b += texture(u_tex, bUV).b * 0.34;
        col.b += texture(u_tex, bUV + vec2( bStep,  bStep)).b * 0.22;
        col.b += texture(u_tex, bUV + vec2(-bStep,  bStep)).b * 0.22;
        col.b += texture(u_tex, bUV + vec2( bStep, -bStep)).b * 0.11;
        col.b += texture(u_tex, bUV + vec2(-bStep, -bStep)).b * 0.11;
    } else {
        col.r = texture(u_tex, refrUV + chromOffset).r;
        col.g = texture(u_tex, refrUV).g;
        col.b = texture(u_tex, refrUV - chromOffset).b;
    }

    // Vibrancy lift
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(vec3(gray), col, 1.35);

    // Specular Fresnel sheen on top edge
    float specular = clamp((0.5 - p.y / (u_resolution.y * 0.5)), 0.0, 1.0) * (1.0 - edgeFactor) * u_specular_strength;
    col += vec3(specular);

    fragColor = vec4(col, 1.0);
}
)#";

} // namespace Shaders
