#include "breakout_game.hpp"

#include "backdrop_frag.spv.hpp"
#include "brick_frag.spv.hpp"
#include "capsule_frag.spv.hpp"
#include "palette.hpp"

#include <gamelib/audio/player.hpp>
#include <gamelib/ui/menu.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace breakout::app {

using gamelib::draw::Align;
using gamelib::platform::Key;

namespace {

// Menu keys are ignored this long after the game-over menu appears, so a key
// meant for the last ball does not instantly start the next game.
constexpr float kGameOverInputDelay = 0.6f;
// The last ball falls and the alarm fades before the game-over menu.
constexpr float kGameOverDelay = 1.2f;

// Brick sounds rise a semitone per row towards the top of the wall.
float _rowPitch(const int row)
{
    return std::exp2(static_cast<float>(std::clamp(8 - row, -2, 10)) / 12.0f);
}

// The brick cell whose rectangle holds `point` (event positions are brick centers).
gamelib::core::Cell _cellAt(const gamelib::core::Vec2 point)
{
    return {static_cast<int>((point.x - game::kFieldLeft) / game::kCellWidth),
            static_cast<int>((point.y - game::kBricksTop) / game::kCellHeight)};
}

std::string_view _powerPopup(const game::PowerType type)
{
    switch (type) {
    case game::PowerType::Wide: return "WIDE PADDLE!";
    case game::PowerType::Multi: return "MULTI BALL!";
    case game::PowerType::Slow: return "SLOW BALL!";
    case game::PowerType::Sticky: return "STICKY PADDLE!";
    }
    return "";
}

} // namespace

BreakoutGame::BreakoutGame(gamelib::app::App& app, ProfileFile profile, const bool demo)
    : m_profile{std::move(profile)}
    , m_menus{buildMenus(m_profile.value())}
    , m_shapes{.brick = app.renderer().addShape(shaders::brick_frag),
               .capsule = app.renderer().addShape(shaders::capsule_frag),
               .backdrop = app.renderer().addShape(shaders::backdrop_frag)}
    , m_sounds{app.audio(), renderSfx}
    , m_attract{[tick = app.tickSeconds()](const std::uint64_t seed) {
                    return Demo{.session = game::Session{seed, tick}, .player = game::DemoPlayer{seed * 31 + 7}};
                },
                [](Demo& d, float) { d.session.tick(d.player.decide(d.session)); },
                [](const Demo& d) { return d.session.phase() == game::Phase::GameOver; }, m_seeds.next64()}
    , m_demoMode{demo}
    , m_session{m_seeds.next64(), app.tickSeconds()}
    , m_demo{m_seeds.next64()}
{
    m_profile.save(); // a missing file is written with the defaults, so it can be found and edited
    m_scores.loadBest(m_profile.value().best);
    applySettings(app);
    if (demo) {
        startGame(app);
    }
}

void BreakoutGame::startGame(gamelib::app::App& app)
{
    m_session = game::Session{m_seeds.next64(), app.tickSeconds()};
    m_demo = game::DemoPlayer{m_seeds.next64()};
    m_scores.reset();
    m_effects.clear();
    m_gameEnded = false;
    showScreen(Screen::Playing);
    // The key that started the game must not launch the ball: releaseAll()
    // keeps its "pressed" transition, so clear that too.
    app.input().releaseAll();
    app.input().clearTransitions();
}

void BreakoutGame::showScreen(const Screen screen)
{
    m_screens.go(screen, m_time);
    activeMenu().select(0);
}

gamelib::ui::Menu& BreakoutGame::activeMenu()
{
    switch (m_screens.current()) {
    case Screen::Paused: return m_menus.paused;
    case Screen::GameOver: return m_menus.gameOver;
    case Screen::Title:
    case Screen::Playing: break;
    }
    return m_menus.title;
}

const game::Session& BreakoutGame::shownSession() const
{
    return m_screens.is(Screen::Title) ? m_attract.sim().session : m_session;
}

void BreakoutGame::onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event)
{
    if (!event.pressed) {
        return; // the paddle reads held keys in tick(); menus react to presses only
    }
    if (event.key == Key::F11) {
        m_profile.value().fullscreen = !m_profile.value().fullscreen;
        applySettings(app);
        m_profile.save();
        return;
    }
    if (m_screens.is(Screen::Playing)) {
        if (event.key == Key::Escape || event.key == Key::P) {
            showScreen(Screen::Paused);
            app.input().releaseAll(); // releases now go to the menu
            m_sounds.queue(app.sounds(), Sfx::MenuBack);
        }
        return;
    }
    handleMenu(app, event.key);
}

void BreakoutGame::handleMenu(gamelib::app::App& app, const Key key)
{
    const std::optional<gamelib::ui::MenuInput> input = gamelib::ui::menuInput(key);
    if (!input || (m_screens.is(Screen::GameOver) && !m_screens.inputReady(m_time, kGameOverInputDelay))) {
        return;
    }
    const gamelib::ui::MenuResult result = activeMenu().handle(*input);
    if (result.changed) {
        applySettings(app);
        m_profile.save();
    }
    switch (result.command) {
    case Play:
    case PlayAgain:
    case Restart: startGame(app); break;
    case Resume:
        m_screens.go(Screen::Playing, m_time);
        app.input().releaseAll();
        app.input().clearTransitions(); // Enter on RESUME must not launch a waiting ball
        break;
    case MainMenu:
        saveBest();
        m_effects.clear();
        showScreen(Screen::Title);
        break;
    case Quit:
        saveBest();
        app.requestQuit();
        break;
    default: break;
    }
    // Feedback after acting, so a new volume is heard right away.
    switch (result.feedback) {
    case gamelib::ui::MenuFeedback::None: break;
    case gamelib::ui::MenuFeedback::Moved:
    case gamelib::ui::MenuFeedback::Changed: m_sounds.queue(app.sounds(), Sfx::MenuMove); break;
    case gamelib::ui::MenuFeedback::Selected: m_sounds.queue(app.sounds(), Sfx::MenuSelect); break;
    case gamelib::ui::MenuFeedback::Back: m_sounds.queue(app.sounds(), Sfx::MenuBack); break;
    }
}

game::Input BreakoutGame::playerInput(const gamelib::platform::InputState& keys) const
{
    return {.move = static_cast<float>(keys.axis({Key::Left, Key::A}, {Key::Right, Key::D})),
            .launch = keys.wasAnyPressed({Key::Space, Key::Enter})};
}

void BreakoutGame::tick(gamelib::app::App& app)
{
    if (m_screens.is(Screen::Title)) {
        m_attract.tick(app.tickSeconds());
        return;
    }
    if (!m_screens.is(Screen::Playing)) {
        return; // paused or game over: the session is frozen
    }
    m_session.tick(m_demoMode ? m_demo.decide(m_session) : playerInput(app.input()));
}

void BreakoutGame::frame(gamelib::app::App& app, const gamelib::app::FrameInfo& info)
{
    m_time = info.time;
    m_gameOverDelay.update(info.seconds);
    if (!m_screens.is(Screen::Paused)) {
        m_effects.update(info.seconds); // effects freeze with the game while paused
    }
    handleEvents(app);
    if (m_screens.is(Screen::Playing)) {
        std::vector<std::uint32_t> alive;
        for (const game::Ball& ball : m_session.balls()) {
            m_effects.recordBall(ball.id, ball.position.at(info.alpha));
            alive.push_back(ball.id);
        }
        m_effects.dropMissingTrails(alive);
    }
    m_scores.set(m_session.score());

    if (m_gameEnded && m_gameOverDelay.ready() && m_screens.is(Screen::Playing)) {
        m_gameEnded = false;
        if (m_demoMode) {
            startGame(app); // endless
        } else {
            saveBest();
            showScreen(Screen::GameOver);
            app.input().releaseAll();
        }
    }
}

void BreakoutGame::handleEvents(gamelib::app::App& app)
{
    // The attract demo is silent and has no effects; its events are dropped.
    (void)m_attract.sim().session.takeEvents();
    gamelib::audio::SoundQueue& queue = app.sounds();
    const Palette& colors = palette();
    for (const game::Event& event : m_session.takeEvents()) {
        switch (event.kind) {
        case game::EventKind::Launched: m_sounds.queue(queue, Sfx::Launch); break;
        case game::EventKind::PaddleHit:
            m_sounds.queue(queue, Sfx::Paddle);
            m_effects.paddleFlash.trigger();
            m_effects.sparks(event.position + gamelib::core::Vec2{0.0f, game::kBallRadius}, colors.paddleGlow, 6, 320.0f);
            break;
        case game::EventKind::WallHit:
            m_sounds.queue(queue, Sfx::Wall);
            m_effects.sparks(event.position, colors.wall, 3, 220.0f);
            break;
        case game::EventKind::BrickHit:
            m_sounds.queue(queue, Sfx::Crack, 1.0f, _rowPitch(event.row));
            m_effects.flashBrick(_cellAt(event.position));
            break;
        case game::EventKind::BrickBroken:
            m_effects.shatter(game::brickRect(_cellAt(event.position)), brickColor(event.brick, event.color, event.power), {});
            m_sounds.queue(queue, event.brick == game::BrickKind::Tough ? Sfx::Crack : Sfx::Brick, 1.0f,
                           _rowPitch(event.row));
            if (event.brick == game::BrickKind::Tough) {
                m_sounds.queue(queue, Sfx::Brick, 0.8f, _rowPitch(event.row));
            }
            break;
        case game::EventKind::UnbreakableHit:
            m_sounds.queue(queue, Sfx::Clank);
            m_effects.sparks(event.position, colors.rivet, 6, 300.0f);
            break;
        case game::EventKind::PowerUpDropped: m_sounds.queue(queue, Sfx::Drop); break;
        case game::EventKind::PowerUpCaught:
            m_sounds.queue(queue, Sfx::PowerUp);
            m_effects.paddleFlash.trigger();
            m_effects.sparks(event.position, powerColor(event.power), 14, 420.0f);
            m_effects.popups.add(std::string{_powerPopup(event.power)}, powerColor(event.power), 1.1f);
            break;
        case game::EventKind::PowerUpEnded: m_sounds.queue(queue, Sfx::PowerDown); break;
        case game::EventKind::Combo:
            // Up a whole tone per milestone, at most an octave.
            m_sounds.queue(queue, Sfx::Combo, 1.0f,
                           std::exp2(std::min(static_cast<float>(event.combo) / 5.0f, 6.0f) * 2.0f / 12.0f));
            m_effects.popups.add("COMBO X" + std::to_string(event.combo), colors.accent, 0.9f);
            break;
        case game::EventKind::BallLost: m_sounds.queue(queue, Sfx::BallLost); break;
        case game::EventKind::LifeLost:
            if (m_session.phase() != game::Phase::GameOver) {
                m_sounds.queue(queue, Sfx::LifeLost);
            }
            m_effects.shake.trigger();
            m_effects.alarm.trigger();
            break;
        case game::EventKind::LevelCleared:
            m_sounds.queue(queue, Sfx::LevelClear);
            m_effects.wallPulse.trigger();
            break;
        case game::EventKind::GameOver:
            m_sounds.queue(queue, Sfx::GameOver);
            m_gameEnded = true;
            m_gameOverDelay.start(kGameOverDelay);
            break;
        case game::EventKind::LevelStarted:
            m_effects.levelIntro.trigger();
            m_effects.popups.add("LEVEL " + std::to_string(m_session.level()) + "  " + m_session.levelName(), colors.paddleGlow,
                                 1.6f);
            break;
        case game::EventKind::PowerUpMissed: break;
        }
    }
}

void BreakoutGame::saveBest()
{
    m_profile.value().best = m_scores.best();
    m_profile.save();
}

void BreakoutGame::draw(gamelib::app::App& app, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    const bool moving = m_screens.is(Screen::Playing) || m_screens.is(Screen::Title);
    const bool title = m_screens.is(Screen::Title);
    const ViewState view{.session = shownSession(),
                         .shapes = m_shapes,
                         .alpha = moving ? info.alpha : 1.0f,
                         .shake = m_profile.value().screenShake ? m_effects.shake.offset(info.time) : gamelib::core::Vec2{},
                         .best = m_scores.best(),
                         .wallPulse = title ? 0.0f : m_effects.wallPulse.value(),
                         .alarm = title ? 0.0f : m_effects.alarm.value() * 0.5f,
                         .effects = title ? nullptr : &m_effects,
                         .intro = title ? 1.0f : m_effects.introProgress(),
                         .launchGuide = m_screens.is(Screen::Playing) && !m_demoMode};
    drawBackdrop(list, view);
    if (!title) {
        // Under the bricks and balls: the popups never hide the play.
        m_effects.popups.draw(list, {(game::kFieldLeft + game::kFieldRight) / 2.0f, 640.0f},
                              {.pixelSize = 7.0f, .align = Align::Center, .shadow = true});
    }
    drawField(list, view);
    drawHud(list, view);

    const gamelib::core::Size canvas = app.canvas();
    const gamelib::ui::MenuStyle style = menuStyle();
    switch (m_screens.current()) {
    case Screen::Playing: drawOverlay(list, info); break;
    case Screen::Title: gamelib::ui::drawMenu(list, m_menus.title, canvas, style, {}, kTitleFooter); break;
    case Screen::Paused: gamelib::ui::drawMenu(list, m_menus.paused, canvas, style); break;
    case Screen::GameOver: {
        std::string subtitle = "SCORE " + std::to_string(m_session.score()) + "   LEVEL " + std::to_string(m_session.level());
        if (m_scores.isNewBest()) {
            subtitle = "NEW BEST! " + subtitle;
        }
        gamelib::ui::drawMenu(list, m_menus.gameOver, canvas, style, subtitle);
        break;
    }
    }
}

void BreakoutGame::drawOverlay(gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info)
{
    const Palette& colors = palette();
    const float centerX = (game::kFieldLeft + game::kFieldRight) / 2.0f;
    if (m_session.phase() == game::Phase::LevelClear) {
        list.box({game::kFieldLeft, 380.0f, game::kFieldWidth, 300.0f}, colors.field.withAlpha(0.85f),
                 {.glowWidth = 40.0f, .glowColor = colors.field.withAlpha(0.6f)});
        list.text("LEVEL " + std::to_string(m_session.level()) + " CLEAR", centerX, 420.0f,
                  {.pixelSize = 12.0f, .color = colors.accent, .align = Align::Center, .shadow = true});
        list.text("BONUS +" + std::to_string(game::kLevelClearPoints * m_session.level()), centerX, 540.0f,
                  {.pixelSize = 6.0f, .color = colors.text, .align = Align::Center});
        if (m_session.canContinue() && std::fmod(info.time, 1.0f) < 0.7f) {
            list.text("PRESS SPACE", centerX, 610.0f,
                      {.pixelSize = 5.0f, .color = colors.textDim, .align = Align::Center});
        }
        return;
    }
    // The serve: a ball waits on the paddle.
    const auto balls = m_session.balls();
    if (!m_demoMode && balls.size() == 1 && balls.front().attached && !balls.front().caught) {
        list.text("SPACE TO LAUNCH", centerX, 1028.0f,
                  {.pixelSize = 5.0f, .color = colors.textDim, .align = Align::Center});
    }
}

void BreakoutGame::applySettings(gamelib::app::App& app)
{
    app.setFullscreen(m_profile.value().fullscreen);
    app.audio().setMasterVolume(gamelib::audio::gain(m_profile.value().volume));
}

} // namespace breakout::app
