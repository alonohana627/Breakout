# breakout_game: the rules, the levels and the demo player. Depends only on
# GameLib::core and GameLib::ai (math, collision, grid, RNG, timers,
# computer-player helpers) - no window, GPU, audio or clock.
add_library(breakout_game STATIC
    ai.cpp
    levels.cpp
    session.cpp)
target_include_directories(breakout_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(breakout_game PUBLIC GameLib::core GameLib::ai)
gamelib_target_warnings(breakout_game)
