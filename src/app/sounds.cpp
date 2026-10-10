#include "sounds.hpp"

#include <gamelib/audio/synth.hpp>

namespace breakout::app {

namespace {

using gamelib::audio::Note;
using gamelib::audio::Wave;
using gamelib::audio::noise;
using gamelib::audio::tone;

// MIDI note numbers. Everything sits on C major pentatonic (C D E G A), so the
// sounds belong together.
constexpr float kC3 = 48, kG3 = 55, kA3 = 57, kC4 = 60, kE4 = 64, kG4 = 67, kA4 = 69;
constexpr float kC5 = 72, kD5 = 74, kE5 = 76, kG5 = 79, kA5 = 81, kC6 = 84, kE6 = 88, kG6 = 91, kC7 = 96;

struct Recipe {
    std::vector<std::vector<Note>> tracks;
    float peakDb = -14.0f; // loudest sample, dBFS
};

Recipe _recipe(const Sfx sfx)
{
    switch (sfx) {
    // A bright blip with a tick of noise; the game raises the pitch for higher rows.
    case Sfx::Brick: return {{{tone(Wave::Pulse25, kE5, 0.05f, kG5)}, {noise(16000.0f, 0.015f, 0.0f, 0.5f)}}, -13.0f};
    // Lower and buzzier, with a crunch: something gave way but held.
    case Sfx::Crack:
        return {{{tone(Wave::Pulse12, kA4, 0.06f, kE4)}, {noise(7000.0f, 0.05f, 2500.0f, 0.7f)}}, -13.0f};
    // A metallic clank: a high thin ping over a short low knock.
    case Sfx::Clank:
        return {{{tone(Wave::Pulse12, kC6 + 1.0f, 0.07f)}, {tone(Wave::Triangle, kC3, 0.05f)}}, -14.0f};
    case Sfx::Paddle: return {{{tone(Wave::Pulse50, kC5, 0.05f)}, {noise(12000.0f, 0.012f, 0.0f, 0.4f)}}, -14.0f};
    case Sfx::Wall: return {{{tone(Wave::Pulse25, kG4, 0.035f)}}, -19.0f};
    case Sfx::Launch: return {{{tone(Wave::Pulse12, kC5, 0.07f, kC6)}}, -17.0f};
    case Sfx::Drop: return {{{tone(Wave::Triangle, kG5, 0.05f), tone(Wave::Triangle, kC6, 0.07f)}}, -18.0f};
    // The power-up chime: a quick rising arpeggio over a soft root.
    case Sfx::PowerUp:
        return {{{tone(Wave::Pulse25, kC6, 0.045f), tone(Wave::Pulse25, kE6, 0.045f), tone(Wave::Pulse25, kG6, 0.045f),
                  tone(Wave::Pulse25, kC7, 0.16f)},
                 {tone(Wave::Triangle, kC4, 0.3f)}},
                -12.0f};
    case Sfx::PowerDown: return {{{tone(Wave::Pulse12, kG5, 0.06f), tone(Wave::Pulse12, kC5, 0.1f)}}, -18.0f};
    // A short upward run; the game pitches it up with the combo.
    case Sfx::Combo:
        return {{{tone(Wave::Pulse25, kC5, 0.035f), tone(Wave::Pulse25, kE5, 0.035f), tone(Wave::Pulse25, kG5, 0.035f),
                  tone(Wave::Pulse25, kC6, 0.08f)}},
                -15.0f};
    case Sfx::BallLost: return {{{tone(Wave::Pulse25, kE5, 0.1f, kA4)}}, -18.0f};
    // The life-lost sting: a falling slide with a low drop and a burst of noise.
    case Sfx::LifeLost:
        return {{{tone(Wave::Pulse25, kE5, 0.12f, kA3), tone(Wave::Pulse25, kA3, 0.2f, kA3 - 5.0f)},
                 {tone(Wave::Triangle, kA3, 0.32f, kA3 - 12.0f)},
                 {noise(5000.0f, 0.25f, 600.0f, 0.35f)}},
                -11.0f};
    // The level-clear jingle: a run up the scale to a held top note, bass underneath.
    case Sfx::LevelClear:
        return {{{tone(Wave::Pulse25, kC5, 0.08f), tone(Wave::Pulse25, kE5, 0.08f), tone(Wave::Pulse25, kG5, 0.08f),
                  tone(Wave::Pulse25, kC6, 0.08f), tone(Wave::Pulse25, kA5, 0.08f), tone(Wave::Pulse25, kC6, 0.08f),
                  tone(Wave::Pulse25, kE6, 0.45f)},
                 {tone(Wave::Triangle, kC3, 0.24f), tone(Wave::Triangle, kG3, 0.24f), tone(Wave::Triangle, kC4, 0.5f)}},
                -11.0f};
    // Game over: a slow fall with the bass doubling it.
    case Sfx::GameOver:
        return {{{tone(Wave::Pulse50, kG5, 0.14f), tone(Wave::Pulse50, kE5, 0.14f), tone(Wave::Pulse50, kD5, 0.14f),
                  tone(Wave::Pulse50, kC5, 0.14f), tone(Wave::Pulse50, kA4, 0.14f), tone(Wave::Pulse50, kC4, 0.5f)},
                 {tone(Wave::Triangle, kG3, 0.42f), tone(Wave::Triangle, kC3, 0.7f)}},
                -11.0f};
    case Sfx::MenuMove: return {{{tone(Wave::Pulse12, kC6, 0.03f)}}, -18.0f};
    case Sfx::MenuSelect: return {{{tone(Wave::Pulse25, kG5, 0.04f), tone(Wave::Pulse25, kD5 + 12.0f, 0.07f)}}, -15.0f};
    case Sfx::MenuBack: return {{{tone(Wave::Pulse25, kG5, 0.04f), tone(Wave::Pulse25, kC5, 0.07f)}}, -15.0f};
    }
    return {};
}

} // namespace

std::vector<float> renderSfx(const Sfx sfx, const std::uint32_t sampleRate)
{
    const Recipe recipe = _recipe(sfx);
    return gamelib::audio::renderMix(recipe.tracks, recipe.peakDb, sampleRate);
}

} // namespace breakout::app
