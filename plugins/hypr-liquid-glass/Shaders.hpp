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

    // Multi-tap Poisson frosted blur driven directly by u_blur_strength
    float effectiveBlur = max(0.0, u_blur_strength);
    if (effectiveBlur > 0.01) {
        vec2 rad = (effectiveBlur * 0.9) / u_resolution;
        const vec2 taps[24] = vec2[24](
            vec2(-0.326212, -0.405805), vec2(-0.840144, -0.073580),
            vec2(-0.695914,  0.457137), vec2(-0.203345,  0.620716),
            vec2( 0.962340, -0.194983), vec2( 0.473434, -0.480026),
            vec2( 0.519456,  0.767022), vec2( 0.185461, -0.893124),
            vec2( 0.507431,  0.064425), vec2( 0.896420,  0.412458),
            vec2(-0.321940, -0.932615), vec2(-0.791559, -0.597705),
            vec2(-0.214444,  0.211431), vec2(-0.413941,  0.864928),
            vec2( 0.034502, -0.320490), vec2( 0.213567,  0.264288),
            vec2(-0.552123, -0.231241), vec2(-0.412532, -0.712314),
            vec2(-0.112451, -0.612452), vec2(-0.732145,  0.151241),
            vec2(-0.452141,  0.412451), vec2( 0.151241,  0.852141),
            vec2( 0.612451,  0.312451), vec2( 0.781245, -0.451241)
        );

        vec3 accum = vec3(0.0);
        for (int i = 0; i < 24; ++i) {
            vec2 offset = taps[i] * rad;
            vec2 rUV = clamp(refrUV + chromOffset + offset * 1.05, 0.001, 0.999);
            vec2 gUV = clamp(refrUV + offset, 0.001, 0.999);
            vec2 bUV = clamp(refrUV - chromOffset + offset * 0.95, 0.001, 0.999);
            accum.r += texture(u_tex, rUV).r;
            accum.g += texture(u_tex, gUV).g;
            accum.b += texture(u_tex, bUV).b;
        }
        col = accum * 0.04166667;
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

