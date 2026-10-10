#pragma once

#include "field.hpp"

#include <gamelib/core/color.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/core/rng.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/fx/juice.hpp>
#include <gamelib/fx/popups.hpp>

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace breakout::app {

// Breakout's visual feedback ("juice"), assembled from GameLib's fx parts plus
// a small shard pool: bricks flash and shatter, balls leave trails, the
// paddle flashes on contact, the screen shakes on a lost life, the walls pulse
// on a level clear, and popups announce combos and power-ups. Presentation
// only: it runs on real frame time and never touches the session.
class Effects {
public:
    static constexpr std::size_t kTrailLength = 12;
    using Trail = gamelib::fx::Trail<kTrailLength>;

    Effects();

    void update(float seconds);
    void clear();
    // 0..1 how far the level intro has run (1 = bricks in place).
    [[nodiscard]] float introProgress() const { return 1.0f - levelIntro.value(); }

    // A brick broke: a white flash over its rectangle and shards in its color.
    void shatter(const gamelib::core::Rect& brick, gamelib::core::Color color, gamelib::core::Vec2 ballVelocity);
    // A brick was hit but held: it flashes.
    void flashBrick(gamelib::core::Cell cell);
    // A burst of sparks at a point (paddle and wall contacts, caught capsules).
    void sparks(gamelib::core::Vec2 at, gamelib::core::Color color, int count, float speed);

    // Records the balls' interpolated positions this frame; trails of balls
    // that are gone are dropped.
    void recordBall(std::uint32_t id, gamelib::core::Vec2 position);
    void dropMissingTrails(const std::vector<std::uint32_t>& alive);

    [[nodiscard]] float brickFlash(gamelib::core::Cell cell) const;

    // Shards and flashes, drawn over the bricks.
    void drawDebris(gamelib::draw::DrawList& list, gamelib::core::Vec2 offset) const;
    // Ball trails, drawn under the balls.
    void drawTrails(gamelib::draw::DrawList& list, gamelib::core::Vec2 offset, gamelib::core::Color color) const;

    gamelib::fx::Fade paddleFlash{0.18f};
    gamelib::fx::Shake shake{0.5f, 20.0f};
    gamelib::fx::Fade wallPulse{1.4f};
    gamelib::fx::Fade alarm{0.9f};
    gamelib::fx::Fade levelIntro{1.2f}; // 1 when a level starts, falling to 0 as its bricks settle
    gamelib::fx::Popups popups{64.0f, 0.45f};

private:
    struct Shard {
        gamelib::core::Vec2 position;
        gamelib::core::Vec2 velocity;
        float angle = 0.0f;
        float spin = 0.0f;
        float size = 8.0f;
        float age = 0.0f;
        float life = 0.6f;
        gamelib::core::Color color;
    };
    struct Flash {
        gamelib::core::Rect rect;
        gamelib::core::Color color;
        float age = 0.0f;
    };

    gamelib::core::Rng m_rng;
    std::vector<Shard> m_shards;
    std::vector<Flash> m_flashes;
    std::array<gamelib::fx::Fade, static_cast<std::size_t>(game::kColumns * game::kMaxRows)> m_brickFlash;
    std::unordered_map<std::uint32_t, Trail> m_trails;
};

} // namespace breakout::app
