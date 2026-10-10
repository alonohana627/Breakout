#pragma once

#include <gamelib/core/grid.hpp>
#include <gamelib/core/options.hpp>
#include <gamelib/core/rng.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace breakout::game {

enum class BrickKind : std::uint8_t {
    None,
    Normal,      // breaks on the first hit
    Tough,       // 2 or 3 hits, cracking a little more each time
    Unbreakable, // only bounces the ball; not needed to clear the level
    PowerUp,     // breaks on the first hit and drops a power-up capsule
};

enum class PowerType : std::uint8_t {
    Wide,   // a wider paddle for a while
    Multi,  // every ball in play splits into three
    Slow,   // slower balls for a while
    Sticky, // the paddle catches balls; Space releases them
};
inline constexpr int kPowerTypeCount = 4;
inline constexpr gamelib::core::OptionSet<PowerType, kPowerTypeCount> kPowerNames{{"WIDE", "MULTI", "SLOW", "STICKY"}};

struct Brick {
    BrickKind kind = BrickKind::None;
    int hits = 0;          // hits left before it breaks (0 for None and Unbreakable)
    int maxHits = 0;       // hits it started with
    std::uint8_t color = 0; // palette slot 0..5 for Normal bricks (the view picks the color)
    PowerType power = PowerType::Wide; // what a PowerUp brick drops

    [[nodiscard]] constexpr bool present() const { return kind != BrickKind::None; }
    [[nodiscard]] constexpr bool breakable() const { return kind != BrickKind::None && kind != BrickKind::Unbreakable; }
};

using Bricks = gamelib::core::Grid<Brick>;

struct Level {
    std::string name;
    Bricks bricks{0, 0};
};

// The number of hand-designed levels; later levels are generated.
[[nodiscard]] int designedLevelCount();

// Level `number` (1-based): a designed one while they last, then a
// procedurally generated one. Deterministic for (number, seed): power-up
// types of '*' bricks and generated layouts come from the seed.
[[nodiscard]] Level makeLevel(int number, std::uint64_t seed);

// Builds a level from a text grid, one string per brick row, kColumns
// characters each:
//   .  empty                 1-6  normal brick, color slot 0-5
//   T  tough, 2 hits         X    tough, 3 hits
//   #  unbreakable           *    power-up brick of a random type
//   W M S C  power-up brick: wide, multi, slow, sticky (catch)
// Unknown characters are empty cells.
[[nodiscard]] Level parseLevel(std::string name, std::span<const std::string_view> rows, gamelib::core::Rng& rng);

// Bricks that still have to break for the level to be cleared.
[[nodiscard]] int breakableCount(const Bricks& bricks);

} // namespace breakout::game
