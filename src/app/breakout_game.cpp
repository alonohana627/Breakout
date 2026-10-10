#include "breakout_game.hpp"

#include "view.hpp"

namespace breakout::app {

BreakoutGame::BreakoutGame(gamelib::app::App& app)
    : m_session{m_seeds.next64(), app.tickSeconds()}
    , m_demo{m_seeds.next64()}
{
}

void BreakoutGame::tick(gamelib::app::App& app)
{
    if (m_session.phase() == game::Phase::GameOver) {
        m_session = game::Session{m_seeds.next64(), app.tickSeconds()}; // the demo starts over
    }
    m_session.tick(m_demo.decide(m_session));
    (void)m_session.takeEvents();
}

void BreakoutGame::draw(gamelib::app::App&, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    drawSession(list, m_session, info.alpha);
}

} // namespace breakout::app
