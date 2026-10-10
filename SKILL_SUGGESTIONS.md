# Skill suggestions

Skills that would have made work on this project go better. See `AGENTS.md` for
how this file is maintained.

## Already installed

In `~/.claude/skills/` (checked 2026-10-10):

- arcade-action-physics (installed during this game)
- chiptune-sfx-synthesis
- ci-cross-platform
- cmake-build-developer
- cmake-library-consumption
- computer-graphics-developer
- cozy-sim-design (installed during this game)
- cpp-debugging-profiling
- cpp-testing
- cpp26-developer
- game-audio-developer
- game-engine-developer
- game-feel-designer
- game-library-extraction
- game-persistence
- game-visual-design
- glfw-platform-developer
- glsl-shader-build-embedding
- grid-algorithms-for-games (installed during this game)
- learning-mentor
- procedural-dungeon-generation (installed during this game)
- puzzle-game-design-undo (installed during this game)
- sdf-2d-shape-shaders
- shader-developer
- software-architecture-developer
- technical-docs-writer
- tilemap-and-camera-2d (installed during this game)
- vulkan-app-smoke-testing
- vulkan-developer
- vulkan-text-and-ui
- win32-game-window-automation
- windows-release-packaging

## Suggested

Ordered by value for the next steps.

### brick-breaker-rules

Breakout and Arkanoid rules as concrete numbers and algorithms: field and
brick proportions (columns, brick aspect, gap, space above the wall), paddle
width and acceleration, the bounce angle as a function of the contact point
and its maximum, minimum vertical angle against endless side-to-side loops,
serve and launch rules, speed-up per brick and per level with a cap, swept
collisions against many bricks (and what to do when a brick breaks in the
middle of a move), brick types (multi-hit, unbreakable, power-up carriers)
and keeping levels winnable, the classic power-up set with durations
(enlarge, catch, slow, disruption/multi-ball, laser, extra life) and drop
rates, scoring with combos, level formats as text grids, procedural level
generation with symmetry, and a demo AI that predicts the ball off the side
walls and aims its returns.

Why: `game-feel-designer` names Breakout but has no Breakout section, so the
paddle angle, speed curve, power-up timings, sticky-paddle release and the
demo player's aim all had to be worked out from scratch here.

### window-screenshot-bursts

Reviewing short-lived effects in a running game from screenshots: bursts of
captures every 100-250 ms with the `win32-game-window-automation` driver,
combining them into one contact-sheet PNG (System.Drawing, fixed columns,
scaled tiles) to look at many frames at once, sampling a pixel per frame to
find the frame where a screen changed, temporary debug hooks to start at a
given level or state without committing them, and what each effect should
look like in a still (a flash, a shake offset, particles, a fading popup).

Why: brick shatter flashes, shards, the life-lost alarm and the level-clear
screen last a fraction of a second; catching them took bursts of shots, a
hand-written montage script and pixel sampling to find the right frame.
