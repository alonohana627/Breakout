#pragma once

#include "ai.hpp"
#include "menus.hpp"
#include "session.hpp"
#include "settings.hpp"
#include "view.hpp"

#include <gamelib/ai/attract.hpp>
#include <gamelib/app/app.hpp>
#include <gamelib/core/rng.hpp>
#include <gamelib/core/score.hpp>
#include <gamelib/core/timers.hpp>
#include <gamelib/ui/screens.hpp>

namespace breakout::app {

// Which screen is showing. Only Playing advances the player's session; the
// title screen runs an attract demo behind the menu.
enum class Screen { Title, Playing, Paused, GameOver };

// The attract demo behind the title menu: a session and its computer player.
struct Demo {
    game::Session session;
    game::DemoPlayer player;
};

// Breakout on GameLib's main loop: maps the keyboard (or the demo player)
// to the session's input, runs the menus and screens, keeps the best score
// and settings, and draws everything.
class BreakoutGame final : public gamelib::app::Game {
public:
    // demo: the computer plays on the playing screen, endlessly (--demo).
    BreakoutGame(gamelib::app::App& app, ProfileFile profile, bool demo);

    void onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event) override;
    void tick(gamelib::app::App& app) override;
    void frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info) override;
    void draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override;

private:
    void startGame(gamelib::app::App& app);
    void showScreen(Screen screen);
    void handleMenu(gamelib::app::App& app, gamelib::platform::Key key);
    void handleEvents(gamelib::app::App& app);
    void applySettings(gamelib::app::App& app);
    void saveBest();
    [[nodiscard]] game::Input playerInput(const gamelib::platform::InputState& keys) const;
    [[nodiscard]] gamelib::ui::Menu& activeMenu();
    [[nodiscard]] const game::Session& shownSession() const;
    void drawOverlay(gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info);

    ProfileFile m_profile; // menus edit m_profile.value() in place
    Menus m_menus;
    Shapes m_shapes;
    gamelib::core::Rng m_seeds{gamelib::core::randomSeed()};
    gamelib::ui::Screens<Screen> m_screens{Screen::Title};
    gamelib::ai::AttractMode<Demo> m_attract;
    gamelib::core::ScoreKeeper m_scores;
    gamelib::core::Cooldown m_gameOverDelay; // the last ball falls before the game-over menu
    bool m_gameEnded = false;

    bool m_demoMode;
    game::Session m_session; // the player's game (or the --demo game)
    game::DemoPlayer m_demo;
    float m_time = 0.0f;
};

} // namespace breakout::app
