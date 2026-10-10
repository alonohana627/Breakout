#pragma once

#include "ai.hpp"
#include "session.hpp"
#include "view.hpp"

#include <gamelib/app/app.hpp>
#include <gamelib/core/rng.hpp>

namespace breakout::app {

// Breakout on GameLib's main loop: maps the keyboard (or the demo player)
// to the session's input, ticks it and draws it.
class BreakoutGame final : public gamelib::app::Game {
public:
    // demo: the computer plays, endlessly (--demo).
    BreakoutGame(gamelib::app::App& app, bool demo);

    void onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event) override;
    void tick(gamelib::app::App& app) override;
    void draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override;

private:
    [[nodiscard]] game::Input playerInput(const gamelib::platform::InputState& keys) const;

    gamelib::core::Rng m_seeds{gamelib::core::randomSeed()};
    bool m_demoMode;
    game::Session m_session;
    game::DemoPlayer m_demo;
    Shapes m_shapes;
};

} // namespace breakout::app
