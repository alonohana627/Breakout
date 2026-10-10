// Breakout - generated from GameLib's new-game template.
//
// A glowing orb you steer with WASD or the arrow keys. It bounces off the
// edges of the canvas with a sound; Space changes its color with a chime.
// Everything on screen is drawn by shaders: GameLib's built-in vignette,
// circle and pixel-font text, and this game's own shaders/orb.frag.
//
// Command line (GameLib self-test switches): --frames N --hidden
// --require-validation --no-vsync --resize-at-frame N --minimize-at-frame N
#include "orb_frag.spv.hpp"

#include <gamelib/app/app.hpp>
#include <gamelib/audio/synth.hpp>
#include <gamelib/core/color.hpp>
#include <gamelib/core/math.hpp>

#include <algorithm>
#include <array>
#include <memory>
#include <string>

namespace {

using gamelib::core::Color;
using gamelib::core::Vec2;
using gamelib::platform::Key;

constexpr Vec2 kCanvas{1920.0f, 1080.0f};
constexpr float kOrbRadius = 70.0f;
constexpr float kOrbGlow = 40.0f;
constexpr float kMaxSpeed = 950.0f;     // canvas units per second
constexpr float kAcceleration = 4000.0f; // units per second squared

const std::array kColors{gamelib::core::srgb(0x31C7EF), gamelib::core::srgb(0xFF5E8A), gamelib::core::srgb(0xFFD166),
                         gamelib::core::srgb(0x7BE07B)};

class OrbGame final : public gamelib::app::Game {
public:
    explicit OrbGame(gamelib::app::App& app)
        : m_orbShape{app.renderer().addShape(shaders::orb_frag)}
    {
        // Sounds are synthesized once at startup; no audio files.
        using gamelib::audio::Wave;
        const std::vector<std::vector<gamelib::audio::Note>> bump{
            {gamelib::audio::tone(Wave::Triangle, 45.0f, 0.08f, 38.0f)},
            {gamelib::audio::noise(3000.0f, 0.03f, 0.0f, 0.4f)}};
        const std::vector<std::vector<gamelib::audio::Note>> chime{
            {gamelib::audio::tone(Wave::Pulse25, 72.0f, 0.06f), gamelib::audio::tone(Wave::Pulse25, 79.0f, 0.06f),
             gamelib::audio::tone(Wave::Pulse25, 84.0f, 0.16f)}};
        m_bumpSound = app.audio().add(gamelib::audio::renderMix(bump, -14.0f, app.audio().sampleRate()));
        m_chimeSound = app.audio().add(gamelib::audio::renderMix(chime, -15.0f, app.audio().sampleRate()));
        app.audio().setMasterVolume(0.5f);
    }

    // Presses that matter once (not held) arrive here, as they happen.
    void onKey(gamelib::app::App& app, const gamelib::platform::KeyEvent& event) override
    {
        if (!event.pressed) {
            return;
        }
        if (event.key == Key::Escape) {
            app.requestQuit();
        } else if (event.key == Key::Space) {
            m_colorIndex = (m_colorIndex + 1) % kColors.size();
            m_pulse = 1.0f;
            app.sounds().queue(m_chimeSound);
        }
    }

    // The simulation: a fixed step, reading the held keys.
    void tick(gamelib::app::App& app) override
    {
        const float dt = app.tickSeconds();
        const gamelib::platform::InputState& keys = app.input();
        const Vec2 wanted = gamelib::core::normalized(
                                {static_cast<float>(keys.axis({Key::A, Key::Left}, {Key::D, Key::Right})),
                                 static_cast<float>(keys.axis({Key::W, Key::Up}, {Key::S, Key::Down}))})
                            * kMaxSpeed;
        const float step = kAcceleration * dt;
        m_velocity = {gamelib::core::approach(m_velocity.x, wanted.x, step),
                      gamelib::core::approach(m_velocity.y, wanted.y, step)};
        m_previous = m_position;
        m_position += m_velocity * dt;

        // Bounce off the canvas edges, with a sound for each new contact.
        bool bumped = false;
        if (m_position.x < kOrbRadius || m_position.x > kCanvas.x - kOrbRadius) {
            m_position.x = std::clamp(m_position.x, kOrbRadius, kCanvas.x - kOrbRadius);
            m_velocity.x = -m_velocity.x * 0.8f;
            bumped = true;
        }
        if (m_position.y < kOrbRadius || m_position.y > kCanvas.y - kOrbRadius) {
            m_position.y = std::clamp(m_position.y, kOrbRadius, kCanvas.y - kOrbRadius);
            m_velocity.y = -m_velocity.y * 0.8f;
            bumped = true;
        }
        if (bumped && gamelib::core::length(m_velocity) > 120.0f) {
            ++m_bumps;
            app.sounds().queue(m_bumpSound, 1.0f, 0.9f + 0.2f * (m_position.y / kCanvas.y));
        }
    }

    // Presentation on real time: effects that fade regardless of tick rate.
    void frame(gamelib::app::App&, const gamelib::app::FrameInfo& info) override
    {
        m_pulse = std::max(0.0f, m_pulse - info.seconds * 2.5f);
    }

    void draw(gamelib::app::App&, gamelib::draw::DrawList& list, const gamelib::app::FrameInfo& info) override
    {
        list.vignette({0.0f, 0.0f, kCanvas.x, kCanvas.y}, gamelib::core::srgb(0x141A30));

        // Interpolate between the last two ticks for smooth motion at any refresh rate.
        const Vec2 orb = gamelib::core::lerp(m_previous, m_position, info.alpha);
        const Color color = kColors[m_colorIndex];
        list.circle(orb + Vec2{0.0f, kOrbRadius * 0.9f}, kOrbRadius * 0.8f, {0.0f, 0.0f, 0.0f, 0.25f},
                    {.glowWidth = 30.0f, .glowColor = {0.0f, 0.0f, 0.0f, 0.2f}}); // a soft shadow
        const float size = 2.0f * (kOrbRadius + kOrbGlow) * (1.0f + 0.15f * m_pulse);
        list.add({
            .rect = gamelib::core::Rect::fromCenter(orb, {size, size}),
            .color = color,
            .color2 = gamelib::core::mix(color, Color{1.0f, 1.0f, 1.0f, 1.0f}, 0.4f),
            .shape = m_orbShape,
            .params = {m_pulse, kOrbGlow, 0.0f, 0.0f},
        });

        const gamelib::core::Color text = gamelib::core::srgb(0xF2F0E5);
        list.text("BREAKOUT", kCanvas.x / 2.0f, 60.0f,
                  {.pixelSize = 10.0f, .color = text, .align = gamelib::draw::Align::Center, .shadow = true});
        list.text("MOVE: WASD / ARROWS    COLOR: SPACE    QUIT: ESC", kCanvas.x / 2.0f, kCanvas.y - 80.0f,
                  {.pixelSize = 4.0f, .color = text.withAlpha(0.7f), .align = gamelib::draw::Align::Center});
        list.text("BUMPS: " + std::to_string(m_bumps), 60.0f, 60.0f, {.pixelSize = 5.0f, .color = text});
    }

private:
    gamelib::draw::ShapeId m_orbShape;
    gamelib::audio::SoundId m_bumpSound{};
    gamelib::audio::SoundId m_chimeSound{};
    Vec2 m_position{kCanvas.x / 2.0f, kCanvas.y / 2.0f};
    Vec2 m_previous = m_position;
    Vec2 m_velocity;
    std::size_t m_colorIndex = 0;
    float m_pulse = 0.0f;
    int m_bumps = 0;
};

} // namespace

int main(int argc, char** argv)
{
    const gamelib::app::AppConfig config{
        .title = "Breakout",
        .windowSize = {1280, 720},
        .canvas = {1920, 1080},
        .tick = std::chrono::nanoseconds{1'000'000'000 / 120},
    };
    return gamelib::app::runMain(argc, argv, config,
                                 [](gamelib::app::App& app) { return std::make_unique<OrbGame>(app); });
}
