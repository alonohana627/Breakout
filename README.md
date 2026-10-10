# Breakout

The brick-breaking arcade game, built on
[GameLib](https://github.com/alonohana627/GameLib). Everything on screen is
drawn by shaders (glowing bevelled bricks that crack, rolling power-up
capsules, a gridded neon field); every sound is synthesized. See
`FEATURES.md` for the full list.

## Controls

| Key | Action |
|---|---|
| Left / Right, A / D | move the paddle (it accelerates a little) |
| Space or Enter | launch the ball; release a ball the sticky paddle caught; continue after a level |
| Escape or P | pause menu |
| F11 | fullscreen on/off (saved) |
| Up / Down, Enter, Escape | menus; Left / Right change settings |

Bricks: colored ones break on one hit, silver ones take two or three (and
crack), dark plates never break, and bricks with a glowing core drop a
power-up: **W** wider paddle, **M** multi-ball, **S** slow ball, **C** sticky
paddle. Hitting several bricks without touching the paddle builds a combo
bonus. Six designed levels, then generated ones forever. 3 lives.

## Build and run

Requirements are GameLib's (Windows with MSYS2 CLANG64: C++26 Clang, CMake
3.28+, Ninja, Vulkan 1.3, glslc). CMake downloads GameLib from GitHub at the
commit pinned in `CMakeLists.txt` (see `docs/GAMELIB_REVISION.md`).

```powershell
cmake --preset debug
cmake --build --preset debug
build/debug/bin/breakout.exe          # play
build/debug/bin/breakout.exe --demo   # the computer plays, endlessly
ctest --preset debug                  # smoke runs with the Vulkan validation layer
```

Settings and the best score live in `%APPDATA%\Breakout\breakout.ini`
(`BREAKOUT_DATA_DIR` overrides the folder). GameLib's self-test switches
work too: `--frames N --hidden --require-validation --no-vsync
--resize-at-frame N --minimize-at-frame N`.

## Layout

- `src/game`: the rules (`Session`), levels as text grids (`levels.cpp`) and
  the demo player (`ai.cpp`); links only `GameLib::core` and `GameLib::ai`.
- `src/app`: GameLib main-loop glue (`BreakoutGame`), view, effects, menus,
  settings, sounds, palette.
- `shaders`: `brick.frag`, `capsule.frag`, `backdrop.frag`.
- `smoke`: CTest smoke runs (no unit tests in games; see `AGENTS.md`).
