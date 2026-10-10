# breakout_app: everything around the rules - the GameLib main-loop glue and
# the view.
add_library(breakout_app STATIC
    breakout_game.cpp
    view.cpp)
target_include_directories(breakout_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(breakout_app PUBLIC breakout_game GameLib::app)
gamelib_target_warnings(breakout_app)
