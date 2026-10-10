#include "breakout_game.hpp"

#include "backdrop_frag.spv.hpp"
#include "brick_frag.spv.hpp"
#include "capsule_frag.spv.hpp"

namespace breakout::app {

using gamelib::platform::Key;

BreakoutGame::BreakoutGame(gamelib::app::App& app, const bool demo)
    : m_demoMode{demo}
    , m_session{m_seeds.next64(), app.tickSeconds()}
    , m_demo{m_seeds.next64()}
    , m_shapes{.brick = app.renderer().addShape(shaders::brick_frag),
               .capsule = app.renderer().addShape(shaders::capsule_frag),
               .backdrop = app.renderer().addShape(shaders::backdrop_frag)}
{
}

void BreakoutGame::onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event)
{
    if (!event.pressed) {
        return; // the paddle reads held keys in tick()
    }
    if (event.key == Key::Escape) {
        app.requestQuit();
    } else if (m_session.phase() == game::Phase::GameOver && (event.key == Key::Space || event.key == Key::Enter)) {
        m_session = game::Session{m_seeds.next64(), app.tickSeconds()};
    }
}

game::Input BreakoutGame::playerInput(const gamelib::platform::InputState& keys) const
{
    return {.move = static_cast<float>(keys.axis({Key::Left, Key::A}, {Key::Right, Key::D})),
            .launch = keys.wasAnyPressed({Key::Space, Key::Enter})};
}

void BreakoutGame::tick(gamelib::app::App& app)
{
    if (m_demoMode && m_session.phase() == game::Phase::GameOver) {
        m_session = game::Session{m_seeds.next64(), app.tickSeconds()}; // the demo starts over
    }
    m_session.tick(m_demoMode ? m_demo.decide(m_session) : playerInput(app.input()));
    (void)m_session.takeEvents();
}

void BreakoutGame::draw(gamelib::app::App&, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    const ViewState view{.session = m_session, .shapes = m_shapes, .alpha = info.alpha};
    drawBackdrop(list, view);
    drawField(list, view);
    drawHud(list, view);
}

} // namespace breakout::app
