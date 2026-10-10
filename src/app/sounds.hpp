#pragma once

#include <gamelib/audio/sound_bank.hpp>

#include <cstdint>
#include <vector>

namespace breakout::app {

// Every sound effect in Breakout, synthesized at startup with GameLib's
// chiptune synth.
enum class Sfx : std::uint8_t {
    Brick,       // a brick breaks (played higher for higher rows)
    Crack,       // a tough brick cracks
    Clank,       // the ball hits an unbreakable brick
    Paddle,
    Wall,
    Launch,
    Drop,        // a power-up capsule falls out of its brick
    PowerUp,     // a capsule is caught: the chime
    PowerDown,   // a timed power-up ran out
    Combo,       // a combo milestone (pitched up with the count)
    BallLost,    // one of several balls fell
    LifeLost,    // the sting
    LevelClear,  // the jingle
    GameOver,
    MenuMove,
    MenuSelect,
    MenuBack,
};
inline constexpr std::size_t kSfxCount = 17;

// Renders one effect to mono float samples, exactly as the game plays it.
[[nodiscard]] std::vector<float> renderSfx(Sfx sfx, std::uint32_t sampleRate);

// The effects, rendered once and registered with the player by GameLib's
// SoundBank, then queued by Sfx (each plays once per frame).
using Sounds = gamelib::audio::SoundBank<Sfx, kSfxCount>;

} // namespace breakout::app
