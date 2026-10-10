add_subdirectory(game)
add_subdirectory(app)

# The executable is only main.cpp; everything else lives in the libraries.
add_executable(breakout main.cpp)
target_link_libraries(breakout PRIVATE breakout_app)
gamelib_target_warnings(breakout)
gamelib_copy_runtime(breakout)
