# Breakout

The brick-breaking arcade game, built on
[GameLib](https://github.com/alonohana627/GameLib). Everything on screen is
drawn by shaders; every sound is synthesized.

## Build and run

Requirements are GameLib's (Windows with MSYS2 CLANG64: C++26 Clang, CMake
3.28+, Ninja, Vulkan 1.3, glslc). CMake downloads GameLib from GitHub at the
commit pinned in `CMakeLists.txt`.

```powershell
cmake --preset debug
cmake --build --preset debug
build/debug/bin/breakout.exe
ctest --preset debug    # smoke runs with the Vulkan validation layer
```
