#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// The whole canvas behind the game: a dark surround with a vignette, the
// playfield well with a faint grid and drifting haze, and glowing walls on
// its left, top and right.
//   params    the field rectangle: left, top, right, bottom (canvas units)
//   color     field color; color2 wall color
//   bits.x    (float bits) a pulse 0..1 that brightens the walls (level clear)
//   bits.y    (float bits) a red alarm tint 0..1 (life lost)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

void main()
{
    vec2 p = inUv * sprite.size + sprite.position; // canvas units
    vec2 lo = sprite.params.xy;
    vec2 hi = sprite.params.zw;
    float pulse = uintBitsToFloat(sprite.bits.x);
    float alarm = uintBitsToFloat(sprite.bits.y);
    // The field box reaches below the canvas: the bottom has no wall.
    vec2 halfSize = (hi - lo) * 0.5 + vec2(0.0, 200.0);
    vec2 center = (lo + hi) * 0.5 + vec2(0.0, 200.0);
    float d = sdBox(p - center, halfSize);

    // Surround: darker than the field, with a vignette.
    vec2 v = (inUv - 0.5) * vec2(1.7, 1.0);
    vec3 surround = sprite.color.rgb * 0.55 * (1.0 - 0.45 * dot(v, v));

    // Field: lighter at the top, a faint grid, slow haze.
    float t = clamp((p.y - lo.y) / (hi.y - lo.y), 0.0, 1.0);
    vec3 field = sprite.color.rgb * mix(1.3, 0.8, t);
    vec2 cell = abs(fract((p - lo) / 48.0 + 0.5) - 0.5) * 48.0; // distance to the nearest grid line
    float grid = 1.0 - smoothstep(0.0, 1.2, min(cell.x, cell.y));
    field += sprite.color2.rgb * grid * 0.045;
    float haze = valueNoise(p / 260.0 + vec2(sprite.time * 0.03, -sprite.time * 0.02));
    field += sprite.color2.rgb * haze * haze * 0.06;

    vec3 col = mix(surround, field, coverage(d));
    // Walls: a bright line on the field's edge and a glow on both sides.
    float wall = abs(d) - 2.5;
    float glow = exp(-max(wall, 0.0) / 22.0);
    vec3 wallColor = sprite.color2.rgb * (1.0 + 1.2 * pulse);
    col += wallColor * glow * (0.2 + 0.25 * pulse);
    col = mix(col, wallColor * 1.4 + 0.1, coverage(wall));
    col = mix(col, col * vec3(1.6, 0.45, 0.5) + vec3(0.05, 0.0, 0.0), alarm * 0.6);

    outColor = vec4(dither(col, gl_FragCoord.xy, 0.05), 1.0);
}
