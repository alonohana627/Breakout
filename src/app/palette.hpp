#pragma once

#include "levels.hpp"

#include <gamelib/core/color.hpp>

#include <array>

namespace breakout::app {

// Every color Breakout uses, authored as sRGB hex and converted to linear
// once. Neon on deep navy: the field is the darkest value, bricks and the
// ball the brightest, the HUD in between.
struct Palette {
    gamelib::core::Color field;
    gamelib::core::Color wall;
    std::array<gamelib::core::Color, 6> bricks; // Normal brick slots 0-5
    gamelib::core::Color tough2;
    gamelib::core::Color tough3;
    gamelib::core::Color crackLight;
    gamelib::core::Color unbreakable;
    gamelib::core::Color rivet;
    gamelib::core::Color powerBrick;
    std::array<gamelib::core::Color, game::kPowerTypeCount> powers; // by PowerType
    gamelib::core::Color paddle;
    gamelib::core::Color paddleGlow;
    gamelib::core::Color ball;
    gamelib::core::Color ballGlow;
    gamelib::core::Color text;
    gamelib::core::Color textDim;
    gamelib::core::Color accent;
    gamelib::core::Color danger;
};

[[nodiscard]] inline const Palette& palette()
{
    using gamelib::core::srgb;
    static const Palette kPalette{
        .field = srgb(0x10173A),
        .wall = srgb(0x4C6BE0),
        .bricks = {srgb(0xFF4F7B), srgb(0xFF8A3D), srgb(0xFFD166), srgb(0x6EE07A), srgb(0x2EC8F0), srgb(0x9A7BFF)},
        .tough2 = srgb(0x98A4C6),
        .tough3 = srgb(0x7886B0),
        .crackLight = srgb(0xFF9A3C),
        .unbreakable = srgb(0x343A57),
        .rivet = srgb(0xD9B36A),
        .powerBrick = srgb(0x262C4C),
        .powers = {srgb(0x2EC8F0), srgb(0xFF4F7B), srgb(0x6EE07A), srgb(0xFFD166)},
        .paddle = srgb(0xEDEBFF),
        .paddleGlow = srgb(0x5BC8FF),
        .ball = srgb(0xFFF8E8),
        .ballGlow = srgb(0xFFC27A),
        .text = srgb(0xF2F0E5),
        .textDim = srgb(0x8B93C0),
        .accent = srgb(0xFFD166),
        .danger = srgb(0xFF4F5E),
    };
    return kPalette;
}

[[nodiscard]] inline gamelib::core::Color powerColor(const game::PowerType type)
{
    return palette().powers[static_cast<std::size_t>(type)];
}

} // namespace breakout::app
