#pragma once

#include "session.hpp"

#include <gamelib/core/math.hpp>
#include <gamelib/draw/draw_list.hpp>
#include <gamelib/draw/sprite.hpp>

#include <cstdint>

namespace breakout::app {

// Breakout's own procedural shapes (registered once with the renderer).
struct Shapes {
    gamelib::draw::ShapeId brick;    // shaders/brick.frag
    gamelib::draw::ShapeId capsule;  // shaders/capsule.frag
    gamelib::draw::ShapeId backdrop; // shaders/backdrop.frag
};

// What the view needs besides the session: interpolation, effects and HUD
// values that live in the app.
struct ViewState {
    const game::Session& session;
    const Shapes& shapes;
    float alpha = 1.0f;        // interpolation between the last two ticks
    gamelib::core::Vec2 shake; // added to everything in the field
    std::int64_t best = 0;
    float wallPulse = 0.0f;    // 0..1, level clear
    float alarm = 0.0f;        // 0..1, life lost
};

void drawBackdrop(gamelib::draw::DrawList& list, const ViewState& view);
// Bricks, power-ups, paddle and balls.
void drawField(gamelib::draw::DrawList& list, const ViewState& view);
// Score, best, level, lives, active power-ups, combo.
void drawHud(gamelib::draw::DrawList& list, const ViewState& view);

} // namespace breakout::app
