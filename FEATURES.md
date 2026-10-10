# Features

What Breakout does today. Only features that are implemented and verified by
running the game are listed; updated in the same commit as each feature (see
`AGENTS.md`).

## Build

- Standalone CMake project on GameLib, pinned to commit `53e8c13` from GitHub.
- Smoke run with the Vulkan validation layer required (`ctest --preset debug`).

## Rules (`src/game`, GameLib::core and GameLib::ai only)

- A 12-column field with walls left, right and top; a paddle at the bottom
  with a short acceleration run-up; 3 lives, a ball lost off the bottom costs
  one when no other ball is in play.
- Bounce angle off the paddle by where the ball hits it (up to 62 degrees);
  ball speed rises with every brick hit up to a cap and starts a little
  faster each level; paths flatter than 18 degrees are steepened.
- Swept collisions against walls, paddle and every brick
  (`core::moveAndCollide`): the ball never tunnels.
- Bricks: normal (1 hit), tough (2 or 3 hits), unbreakable, and power-up
  bricks that drop a falling capsule to catch: wider paddle, multi-ball
  (every ball splits in three), slow ball, sticky paddle (catches the ball,
  the launch input releases it, auto-release after 3 s).
- Six hand-designed levels as text grids in code, then seeded, mirrored,
  procedurally generated levels that get denser and tougher.
- Score per brick with a combo bonus for consecutive bricks without touching
  the paddle; a level-clear bonus.
- Deterministic fixed-tick simulation with interpolated positions for
  smooth drawing.
- Demo player built on GameLib's ai helpers (bounce prediction, reaction
  timer, steering) that aims varied returns, catches power-ups and misses now
  and then.
