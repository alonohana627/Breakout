# Features

What Breakout does today. Only features that are implemented and verified by
running the game are listed; updated in the same commit as each feature (see
`AGENTS.md`).

## Build

- Standalone CMake project on GameLib, pinned to commit `53e8c13` from GitHub.
- Smoke runs with the Vulkan validation layer required (`ctest --preset debug`):
  the start screen, an unthrottled demo game, and the demo through a resize
  and a minimize/restore.

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

## Controls and screens

- Left/Right or A/D move the paddle; Space or Enter launches the ball (and
  releases a caught one); Escape or P pauses; F11 toggles fullscreen.
- Title menu over a live attract demo (the demo player in a real game,
  restarted when it ends); pause menu; a "SPACE TO LAUNCH" hint on the serve;
  a level-clear screen with the bonus, continued with Space; a game-over
  menu with score, level and "NEW BEST!" after a short delay; menu keys are
  ignored briefly after it appears.
- Settings in the title and pause menus: sound volume (off/low/medium/high),
  screen shake on/off, fullscreen. Saved with the best score to
  `breakout.ini` in the user data folder (`BREAKOUT_DATA_DIR` overrides it).
- `--demo`: the computer plays on the playing screen, starting over after
  each game over.

## Look (all procedural shaders)

- Backdrop (`shaders/backdrop.frag`): a dark surround with a vignette, the
  playfield well with a faint grid and drifting noise haze, glowing walls on
  three sides; supports a wall pulse and a red alarm tint.
- Bricks (`shaders/brick.frag`): glowing rounded boxes with a bevel lit from
  the top and an inner light; tough bricks are brushed metal that cracks with
  each hit, the cracks glowing from inside; unbreakable bricks are dark
  striped plates with rivets; power-up bricks show a pulsing core in the
  power-up's color.
- Power-up capsules (`shaders/capsule.frag`): cylinder-shaded pills with a
  rolling light band and a letter (W wide, M multi, S slow, C sticky).
- Glowing paddle (its rim turns yellow while sticky) and glowing ball;
  smooth interpolated motion at any frame rate.
- HUD on both sides of the field: seven-segment score, best and level
  digits, the level name, lives as balls, active power-ups with time bars,
  the combo count and a ball-speed meter.
