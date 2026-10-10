#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"

// A falling power-up capsule: a horizontal pill shaded like a cylinder, a
// light band rolling across it and an outer glow. The game draws the letter
// on top.
//   params.x  glow width outside the capsule (the quad is grown by it)
//   color = body, color2 = glow and rolling light

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main()
{
    vec2 p = spriteLocal(inUv);
    float glow = sprite.params.x;
    vec2 halfSize = sprite.size * 0.5 - glow;
    float r = halfSize.y;
    float d = sdSegment(p, vec2(-halfSize.x + r, 0.0), vec2(halfSize.x - r, 0.0), r);

    // Cylinder shading across y, a highlight near the top.
    float y = clamp(p.y / r, -1.0, 1.0);
    float roundness = sqrt(1.0 - y * y);
    vec3 col = sprite.color.rgb * (0.45 + 0.65 * roundness);
    col += vec3(1.0) * smoothstep(0.75, 1.0, 1.0 - abs(y + 0.45) * 2.2) * 0.35;
    // A band of light rolling around the capsule.
    float band = smoothstep(0.82, 1.0, sin(p.x * 0.12 - sprite.time * 7.0));
    col = mix(col, sprite.color2.rgb * 1.3, band * 0.35 * roundness);
    // A white rim.
    col = mix(col, vec3(1.0), coverage(abs(d + 1.5) - 1.5) * 0.55);

    vec4 body = vec4(col, sprite.color.a * coverage(d));
    vec4 halo = vec4(sprite.color2.rgb, sprite.color2.a * glowFalloff(d, glow));
    outColor = over(body, halo);
}
