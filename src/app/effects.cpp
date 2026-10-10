#include "effects.hpp"

#include <gamelib/core/easing.hpp>

#include <algorithm>

namespace breakout::app {

using gamelib::core::Color;
using gamelib::core::Rect;
using gamelib::core::Vec2;

namespace {

constexpr float kFlashSeconds = 0.16f;

std::size_t _cellIndex(const gamelib::core::Cell cell)
{
    return static_cast<std::size_t>(cell.y * game::kColumns + cell.x);
}

} // namespace

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
    m_debris.update(seconds);
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
    m_debris.clear();
    m_flashes.clear();
    m_trails.clear();
}

void Effects::shatter(const Rect& brick, const Color color, const Vec2 ballVelocity)
{
    m_flashes.push_back({.rect = brick, .color = gamelib::core::mix(color, Color{1.0f, 1.0f, 1.0f, 1.0f}, 0.65f)});
    // Shards from a 5x2 grid over the brick, flung away from its center and a
    // little along the ball's direction.
    const Vec2 push = gamelib::core::normalized(ballVelocity) * 160.0f;
    gamelib::core::Rng& rng = m_debris.rng();
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 5; ++x) {
            const Vec2 at{brick.x + brick.width * (static_cast<float>(x) + 0.5f) / 5.0f,
                          brick.y + brick.height * (static_cast<float>(y) + 0.5f) / 2.0f};
            const Vec2 away = gamelib::core::normalized(at - brick.center() + Vec2{0.0f, 0.01f});
            if (!m_debris.emit({.position = at,
                                .velocity = away * rng.uniform(140.0f, 420.0f) + push
                                            + Vec2{rng.uniform(-60.0f, 60.0f), rng.uniform(-220.0f, -40.0f)},
                                .angle = rng.uniform(0.0f, 6.28f),
                                .spin = rng.uniform(-14.0f, 14.0f),
                                .size = rng.uniform(13.0f, 22.0f),
                                .life = rng.uniform(0.45f, 0.8f),
                                .color = color})) {
                return; // the pool is full
            }
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
    m_debris.burst(at, count,
                   {.direction = {3.4f, 6.0f}, // mostly upwards
                    .speed = {0.4f * speed, speed},
                    .size = {3.0f, 5.0f},
                    .life = {0.2f, 0.4f},
                    .color = color});
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
        const Rect r = flash.rect.expanded(12.0f * gamelib::core::easeOutCubic(t));
        list.box({r.x + offset.x, r.y + offset.y, r.width, r.height}, flash.color.withAlpha(fade),
                 {.cornerRadius = 8.0f, .glowWidth = 24.0f, .glowColor = flash.color.withAlpha(0.6f * fade)});
    }
    m_debris.draw(list, offset); // GameLib's default style: boxes shrinking to half size and fading
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
