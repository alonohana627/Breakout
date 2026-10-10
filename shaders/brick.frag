#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/sdf.glsl"
#include "gamelib/noise.glsl"

// A Breakout brick: a glowing rounded box with a bevel and an inner light,
// cracking as it weakens.
//   bits.x    a per-brick seed (crack pattern, pulse phase)
//   bits.y    kind: 0 normal, 1 tough, 2 unbreakable, 3 power-up
//   params.x  damage 0..1 (tough bricks crack more with each hit)
//   params.y  hit flash 0..1 (whitens the brick)
//   params.z  glow width outside the brick (the quad is grown by it)
//   params.w  inner-light strength
//   color = body, color2 = glow, inner light and the light inside cracks

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

float rand(uint seed, uint i)
{
    return hash12(uvec2(seed, i * 7919u + 13u));
}

// Distance to a crack pattern: up to six jagged lines from an impact point,
// more and longer ones with more damage. The loop count is uniform per draw.
float crackDistance(vec2 p, vec2 halfSize, uint seed, float damage)
{
    vec2 center = (vec2(rand(seed, 0u), rand(seed, 1u)) - 0.5) * halfSize * vec2(1.0, 0.7);
    int count = int(ceil(damage * 6.0));
    float reach = halfSize.x * min(1.0, 0.35 + damage);
    float d = 1e5;
    for (int i = 0; i < 6; ++i) {
        if (i >= count) {
            break;
        }
        float angle = (float(i) + rand(seed, 10u + uint(i)) * 0.8) * 6.2831853 / 6.0;
        vec2 dir = vec2(cos(angle), sin(angle) * 0.6);
        float len = mix(0.45, 1.0, rand(seed, 20u + uint(i))) * reach;
        vec2 bend = vec2(-dir.y, dir.x) * (rand(seed, 30u + uint(i)) - 0.5) * halfSize.y * 0.7;
        vec2 mid = center + dir * len * 0.5 + bend;
        vec2 end = center + dir * len;
        d = min(d, sdSegment(p, center, mid, 0.0));
        d = min(d, sdSegment(p, mid, end, 0.0));
    }
    return d;
}

void main()
{
    vec2 p = spriteLocal(inUv);
    float glow = sprite.params.z;
    vec2 halfSize = sprite.size * 0.5 - glow;
    float d = sdRoundBox(p, halfSize, 6.0);
    uint seed = sprite.bits.x;
    uint kind = sprite.bits.y;
    float damage = sprite.params.x;

    vec3 base = sprite.color.rgb;
    vec3 light = sprite.color2.rgb;
    vec2 n = p / halfSize; // -1..1 across the brick

    // Bevel: a band along the edge lit from the top, shaded at the bottom.
    float inset = clamp(-d / 6.0, 0.0, 1.0); // 0 on the edge, 1 six units in
    float facing = -(n.x * 0.3 + n.y * 0.7);  // > 0 towards the top-left
    vec3 col = base * (0.86 + 0.18 * facing * inset);
    col = mix(col, base * 1.55 + 0.06, (1.0 - inset) * clamp(facing + 0.35, 0.0, 1.0) * 0.75);
    col = mix(col, base * 0.42, (1.0 - inset) * clamp(-facing + 0.1, 0.0, 1.0) * 0.8);

    // Inner light: a soft glow in the middle of the face.
    vec2 q = n * vec2(1.0, 1.8);
    col += light * exp(-dot(q, q) * 1.6) * 0.28 * sprite.params.w;

    if (kind == 1u) {
        // Tough: brushed metal, cracks that glow from inside.
        col *= 0.9 + 0.14 * valueNoise(p * vec2(0.06, 0.6) + float(seed % 97u));
        if (damage > 0.0) {
            float cd = crackDistance(p, halfSize, seed, damage);
            float line = coverage(cd - 1.2 - 1.3 * damage);
            col = mix(col, base * 0.18, line);
            col = mix(col, col * 0.7, damage * 0.5); // weaker bricks look darker
            col += light * exp(-max(cd - 1.0, 0.0) / 4.0) * (0.6 + 1.2 * damage);
        }
    } else if (kind == 2u) {
        // Unbreakable: dark plate with stripes and four rivets.
        float stripe = step(0.5, fract((p.x - p.y) / 22.0));
        col *= 0.8 + 0.22 * stripe * inset;
        vec2 rivet = abs(p) - (halfSize - vec2(9.0, 8.0));
        float r = length(rivet) - 3.2;
        col = mix(col, light * 0.9, coverage(r) * 0.85);
    } else if (kind == 3u) {
        // Power-up: a capsule-shaped core that pulses.
        float core = sdRoundBox(p, halfSize * vec2(0.62, 0.36), halfSize.y * 0.36);
        float pulse = 0.65 + 0.35 * sin(sprite.time * 5.0 + float(seed % 628u) * 0.01);
        col = mix(col, light * (1.1 + 0.3 * pulse), coverage(core) * 0.9);
        col += light * glowFalloff(core, 12.0) * 0.55 * pulse;
    }

    col = mix(col, vec3(1.0), sprite.params.y); // hit flash

    vec4 body = vec4(col, sprite.color.a * coverage(d));
    vec4 halo = vec4(light, sprite.color2.a * glowFalloff(d, glow));
    outColor = over(body, halo);
}
