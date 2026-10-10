#pragma once

#include <gamelib/core/grid.hpp>
#include <gamelib/core/math.hpp>

// The playfield's geometry in canvas units (the 1920x1080 canvas, y grows
// downwards). The field is a tall well in the middle of the canvas; the
// HUD uses the space on either side.

namespace breakout::game {

inline constexpr float kFieldLeft = 384.0f;
inline constexpr float kFieldRight = 1536.0f;
inline constexpr float kFieldTop = 64.0f;
inline constexpr float kFieldBottom = 1080.0f; // the open side: a ball below it is lost
inline constexpr float kFieldWidth = kFieldRight - kFieldLeft;

// Bricks sit on a grid of cells, kColumns wide, starting a little below the top
// wall so the ball can get behind the wall of bricks.
inline constexpr int kColumns = 12;
inline constexpr int kMaxRows = 12;
inline constexpr float kCellWidth = kFieldWidth / kColumns; // 96
inline constexpr float kCellHeight = 40.0f;
inline constexpr float kBricksTop = kFieldTop + 88.0f;
inline constexpr float kBrickGap = 3.0f; // on each side of a brick inside its cell

inline constexpr float kPaddleY = 990.0f; // center
inline constexpr float kPaddleHeight = 22.0f;
inline constexpr float kBallRadius = 11.0f;
inline constexpr gamelib::core::Vec2 kPowerUpSize{70.0f, 28.0f};

// The rectangle of the brick in `cell`.
[[nodiscard]] constexpr gamelib::core::Rect brickRect(const gamelib::core::Cell cell)
{
    return {kFieldLeft + static_cast<float>(cell.x) * kCellWidth + kBrickGap,
            kBricksTop + static_cast<float>(cell.y) * kCellHeight + kBrickGap, kCellWidth - 2.0f * kBrickGap,
            kCellHeight - 2.0f * kBrickGap};
}

} // namespace breakout::game
