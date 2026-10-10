// Breakout - the brick-breaking arcade game on GameLib.
//
// Command line: GameLib's self-test switches (--frames N --hidden
// --require-validation --no-vsync --allow-software --resize-at-frame N
// --minimize-at-frame N).
#include "breakout_game.hpp"

#include <gamelib/app/app.hpp>

#include <memory>

int main(int argc, char** argv)
{
    const gamelib::app::AppConfig config{
        .title = "Breakout",
        .windowSize = {1280, 720},
        .canvas = {1920, 1080},
        .tick = std::chrono::nanoseconds{1'000'000'000 / 120},
    };
    return gamelib::app::runMain(argc, argv, config, [](gamelib::app::App& app) {
        return std::make_unique<breakout::app::BreakoutGame>(app);
    });
}
