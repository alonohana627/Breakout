#include "levels.hpp"

#include "field.hpp"

#include <algorithm>
#include <array>

namespace breakout::game {

namespace {

struct Design {
    std::string_view name;
    std::array<std::string_view, kMaxRows> rows; // unused rows stay empty
};

// The hand-designed levels, from gentle to mean. Every breakable brick must be
// reachable: unbreakable bricks never close a pocket completely.
// clang-format off
constexpr std::array kDesigns{
    Design{"FIRST LIGHT", {
        "............",
        "666666666666",
        "555555555555",
        "44444*444444",
        "333333333333",
        "2222222*2222",
        "111111111111",
    }},
    Design{"PYRAMID", {
        ".....TT.....",
        "....5665....",
        "...5*66*5...",
        "..44444444..",
        ".3333MM3333.",
        "222222222222",
        "111111111111",
    }},
    Design{"FORTRESS", {
        "#..........#",
        "#.TTTTTTTT.#",
        "#.T666666T.#",
        "#.T6W55C6T.#",
        "#.T666666T.#",
        "#.TTTTTTTT.#",
        "#....##....#",
        "............",
        "2.2.2..2.2.2",
    }},
    Design{"CHECKERS", {
        "1.2.3.4.5.6.",
        ".1.2.3.4.5.6",
        "X.C.X.X.S.X.",
        ".6.5.4.3.2.1",
        "6.5.4.3.2.1.",
        ".X.X.W.X.X.X",
        "T.T.T.T.T.T.",
    }},
    Design{"INVADER", {
        "..4......4..",
        "...4....4...",
        "..44444444..",
        ".44#4MM4#44.",
        "444444444444",
        "4.4XXXXXX4.4",
        "4.4......4.4",
        "...44..44...",
        "............",
        "S..........W",
    }},
    Design{"ZIGZAG", {
        "XX..........",
        "TTXX........",
        "666TTXX.....",
        ".*666TTXX...",
        "...*666TTXX.",
        ".....*666TTX",
        ".......*666T",
        "#########...",
        "............",
        "...3333333..",
    }},
};
// clang-format on

Brick _brickFor(const char c, gamelib::core::Rng& rng)
{
    const auto power = [](const PowerType type) {
        return Brick{.kind = BrickKind::PowerUp, .hits = 1, .maxHits = 1, .color = 0, .power = type};
    };
    switch (c) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
        return Brick{.kind = BrickKind::Normal, .hits = 1, .maxHits = 1, .color = static_cast<std::uint8_t>(c - '1')};
    case 'T': return Brick{.kind = BrickKind::Tough, .hits = 2, .maxHits = 2};
    case 'X': return Brick{.kind = BrickKind::Tough, .hits = 3, .maxHits = 3};
    case '#': return Brick{.kind = BrickKind::Unbreakable};
    case '*': return power(static_cast<PowerType>(rng.below(kPowerTypeCount)));
    case 'W': return power(PowerType::Wide);
    case 'M': return power(PowerType::Multi);
    case 'S': return power(PowerType::Slow);
    case 'C': return power(PowerType::Sticky);
    default: return Brick{};
    }
}

// A generated level: a left half of random bricks mirrored onto the right,
// getting denser and tougher with the level number.
Level _generate(const int number, gamelib::core::Rng& rng)
{
    const int depth = number - designedLevelCount(); // 1 for the first generated level
    const int rows = std::clamp(6 + depth / 2, 6, 10);
    const float toughChance = std::min(0.12f + 0.04f * static_cast<float>(depth), 0.45f);
    const float threeHitShare = std::min(0.2f + 0.05f * static_cast<float>(depth), 0.6f);
    const int colorPattern = static_cast<int>(rng.below(3)); // stripes, columns or chevrons

    for (;;) {
        std::array<std::string, kMaxRows> text;
        for (int row = 0; row < rows; ++row) {
            std::string line(kColumns, '.');
            bool wallUsed = false; // at most one unbreakable brick per half row: never a closed pocket
            for (int col = 0; col < kColumns / 2; ++col) {
                char c = '.';
                const float roll = rng.unit();
                if (roll < 0.14f) {
                    c = '.';
                } else if (roll < 0.19f && !wallUsed && row < rows - 1) {
                    c = '#';
                    wallUsed = true;
                } else if (roll < 0.26f) {
                    c = '*';
                } else if (roll < 0.26f + toughChance) {
                    c = rng.chance(threeHitShare) ? 'X' : 'T';
                } else {
                    int slot = row;
                    if (colorPattern == 1) {
                        slot = col;
                    } else if (colorPattern == 2) {
                        slot = row + (kColumns / 2 - 1 - col); // chevrons once mirrored
                    }
                    c = static_cast<char>('1' + (rows - 1 - slot % 6 + 6) % 6);
                }
                line[static_cast<std::size_t>(col)] = c;
                line[static_cast<std::size_t>(kColumns - 1 - col)] = c;
            }
            text[static_cast<std::size_t>(row)] = std::move(line);
        }
        std::array<std::string_view, kMaxRows> views;
        std::ranges::copy(text, views.begin());
        Level level = parseLevel("SECTOR " + std::to_string(number), std::span{views}.first(static_cast<std::size_t>(rows)),
                                 rng);
        if (breakableCount(level.bricks) >= 30) {
            return level;
        }
    }
}

} // namespace

int designedLevelCount()
{
    return static_cast<int>(kDesigns.size());
}

Level makeLevel(const int number, const std::uint64_t seed)
{
    gamelib::core::Rng rng{seed ^ (static_cast<std::uint64_t>(number) * 0x9E3779B97F4A7C15ULL)};
    if (number >= 1 && number <= designedLevelCount()) {
        const Design& design = kDesigns[static_cast<std::size_t>(number - 1)];
        const auto used = std::ranges::find_if(design.rows, [](const std::string_view row) { return row.empty(); });
        const auto count = static_cast<std::size_t>(used - design.rows.begin());
        return parseLevel(std::string{design.name}, std::span{design.rows}.first(count), rng);
    }
    return _generate(number, rng);
}

Level parseLevel(std::string name, const std::span<const std::string_view> rows, gamelib::core::Rng& rng)
{
    const std::size_t height = std::min(rows.size(), static_cast<std::size_t>(kMaxRows));
    return {.name = std::move(name),
            .bricks = gamelib::core::gridFromText(rows.first(height), [&rng](const char c) { return _brickFor(c, rng); })};
}

int breakableCount(const Bricks& bricks)
{
    return static_cast<int>(std::ranges::count_if(bricks.cells(), [](const Brick& b) { return b.breakable(); }));
}

} // namespace breakout::game
