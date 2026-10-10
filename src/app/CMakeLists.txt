# breakout_app: everything around the rules - the GameLib main-loop glue,
# menus, settings, sounds, effects and the view. Menus, the settings file,
# sound banks and effects come from GameLib (ui, storage, audio, fx); what
# stays here is Breakout's content and look.
add_library(breakout_app STATIC
    breakout_game.cpp
    menus.cpp
    settings.cpp
    sounds.cpp
    view.cpp)
target_include_directories(breakout_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(breakout_app PUBLIC breakout_game GameLib::app GameLib::storage GameLib::ui GameLib::fx GameLib::ai)
gamelib_target_warnings(breakout_app)
# Breakout's own procedural shapes, embedded as SPIR-V (see view.hpp, Shapes).
gamelib_add_shaders(breakout_app SHADERS
    ${PROJECT_SOURCE_DIR}/shaders/backdrop.frag
    ${PROJECT_SOURCE_DIR}/shaders/brick.frag
    ${PROJECT_SOURCE_DIR}/shaders/capsule.frag)
