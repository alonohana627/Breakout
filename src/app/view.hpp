#pragma once

#include "session.hpp"

#include <gamelib/draw/draw_list.hpp>

namespace breakout::app {

// Draws a session: field, bricks, paddle, balls, power-ups and the HUD.
// alpha interpolates moving things between the last two ticks.
void drawSession(gamelib::draw::DrawList& list, const game::Session& session, float alpha);

} // namespace breakout::app
