#pragma once

#include "ai.hpp"
#include "session.hpp"

#include <gamelib/app/app.hpp>
#include <gamelib/core/rng.hpp>

namespace breakout::app {

// Breakout on GameLib's main loop: ticks the session with the demo player's
// input and draws it.
class BreakoutGame final : public gamelib::app::Game {
public:
    explicit BreakoutGame(gamelib::app::App& app);

    void tick(gamelib::app::App& app) override;
    void draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override;

private:
    gamelib::core::Rng m_seeds{gamelib::core::randomSeed()};
    game::Session m_session;
    game::DemoPlayer m_demo;
};

} // namespace breakout::app
