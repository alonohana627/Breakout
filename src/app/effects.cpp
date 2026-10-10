#include "effects.hpp"

#include <algorithm>
#include <cmath>

namespace breakout::app {

using gamelib::core::Color;
using gamelib::core::Rect;
using gamelib::core::Vec2;

namespace {

constexpr float kGravity = 1400.0f;      // canvas units per second squared, shards only
constexpr float kFlashSeconds = 0.16f;
constexpr std::size_t kMaxShards = 600;  // a multi-ball frenzy cannot grow the pool without bound

float _easeOutCubic(const float t)
{
    const float u = 1.0f - std::clamp(t, 0.0f, 1.0f);
    return 1.0f - u * u * u;
}

std::size_t _cellIndex(const gamelib::core::Cell cell)
{
    return static_cast<std::size_t>(cell.y * game::kColumns + cell.x);
}

} // namespace

Effects::Effects()
    : m_rng{gamelib::core::randomSeed()}
{
}

void Effects::update(const float seconds)
{
    paddleFlash.update(seconds);
    shake.update(seconds);
    wallPulse.update(seconds);
    alarm.update(seconds);
    levelIntro.update(seconds);
    popups.update(seconds);
    for (gamelib::fx::Fade& fade : m_brickFlash) {
        fade.update(seconds);
    }
    for (Shard& shard : m_shards) {
        shard.age += seconds;
        shard.velocity.y += kGravity * seconds;
        shard.velocity *= std::pow(0.4f, seconds); // air drag
        shard.position += shard.velocity * seconds;
        shard.angle += shard.spin * seconds;
    }
    std::erase_if(m_shards, [](const Shard& s) { return s.age >= s.life; });
    for (Flash& flash : m_flashes) {
        flash.age += seconds;
    }
    std::erase_if(m_flashes, [](const Flash& f) { return f.age >= kFlashSeconds; });
}

void Effects::clear()
{
    paddleFlash.clear();
    shake.clear();
    wallPulse.clear();
    alarm.clear();
    levelIntro.clear();
    popups.clear();
    for (gamelib::fx::Fade& fade : m_brickFlash) {
        fade.clear();
    }
    m_shards.clear();
    m_flashes.clear();
    m_trails.clear();
}

void Effects::shatter(const Rect& brick, const Color color, const Vec2 ballVelocity)
{
    m_flashes.push_back({.rect = brick, .color = gamelib::core::mix(color, Color{1.0f, 1.0f, 1.0f, 1.0f}, 0.65f)});
    // Shards from a 5x2 grid over the brick, flung away from its center and a
    // little along the ball's direction.
    const Vec2 push = gamelib::core::normalized(ballVelocity) * 160.0f;
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 5; ++x) {
            if (m_shards.size() >= kMaxShards) {
                return;
            }
            const Vec2 at{brick.x + brick.width * (static_cast<float>(x) + 0.5f) / 5.0f,
                          brick.y + brick.height * (static_cast<float>(y) + 0.5f) / 2.0f};
            const Vec2 away = gamelib::core::normalized(at - brick.center() + Vec2{0.0f, 0.01f});
            m_shards.push_back({.position = at,
                                .velocity = away * m_rng.uniform(140.0f, 420.0f) + push
                                            + Vec2{m_rng.uniform(-60.0f, 60.0f), m_rng.uniform(-220.0f, -40.0f)},
                                .angle = m_rng.uniform(0.0f, 6.28f),
                                .spin = m_rng.uniform(-14.0f, 14.0f),
                                .size = m_rng.uniform(13.0f, 22.0f),
                                .life = m_rng.uniform(0.45f, 0.8f),
                                .color = color});
        }
    }
}

void Effects::flashBrick(const gamelib::core::Cell cell)
{
    const std::size_t i = _cellIndex(cell);
    if (i < m_brickFlash.size()) {
        m_brickFlash[i].trigger(0.8f);
    }
}

void Effects::sparks(const Vec2 at, const Color color, const int count, const float speed)
{
    for (int i = 0; i < count && m_shards.size() < kMaxShards; ++i) {
        const float angle = m_rng.uniform(3.4f, 6.0f); // mostly upwards
        m_shards.push_back({.position = at,
                            .velocity = gamelib::core::fromAngle(angle) * m_rng.uniform(0.4f, 1.0f) * speed,
                            .angle = angle,
                            .spin = 0.0f,
                            .size = m_rng.uniform(3.0f, 5.0f),
                            .life = m_rng.uniform(0.2f, 0.4f),
                            .color = color});
    }
}

void Effects::recordBall(const std::uint32_t id, const Vec2 position)
{
    m_trails[id].record(position);
}

void Effects::dropMissingTrails(const std::vector<std::uint32_t>& alive)
{
    std::erase_if(m_trails, [&](const auto& entry) { return std::ranges::find(alive, entry.first) == alive.end(); });
}

float Effects::brickFlash(const gamelib::core::Cell cell) const
{
    const std::size_t i = _cellIndex(cell);
    return i < m_brickFlash.size() ? m_brickFlash[i].value() : 0.0f;
}

void Effects::drawDebris(gamelib::draw::DrawList& list, const Vec2 offset) const
{
    for (const Flash& flash : m_flashes) {
        // The flash swells a little and fades out fast.
        const float t = flash.age / kFlashSeconds;
        const float fade = (1.0f - t) * (1.0f - t);
        const Rect r = flash.rect.expanded(12.0f * _easeOutCubic(t));
        list.box({r.x + offset.x, r.y + offset.y, r.width, r.height}, flash.color.withAlpha(fade),
                 {.cornerRadius = 8.0f, .glowWidth = 24.0f, .glowColor = flash.color.withAlpha(0.6f * fade)});
    }
    for (const Shard& shard : m_shards) {
        const float t = shard.age / shard.life;
        const float size = shard.size * (1.0f - 0.5f * t);
        const Vec2 p = shard.position + offset;
        list.box(Rect::fromCenter(p, {size, size * 0.7f}), shard.color.withAlpha(1.0f - t * t), {.cornerRadius = 1.5f},
                 shard.angle);
    }
}

void Effects::drawTrails(gamelib::draw::DrawList& list, const Vec2 offset, const Color color) const
{
    for (const auto& [id, trail] : m_trails) {
        for (std::size_t i = 1; i < trail.size(); ++i) {
            const float fade = 1.0f - Trail::age(i);
            list.circle(trail[i] + offset, game::kBallRadius * (0.35f + 0.6f * fade), color.withAlpha(0.5f * fade * fade));
        }
    }
}

} // namespace breakout::app
