// Breakout - the brick-breaking arcade game on GameLib.
//
// Command line: --demo (the computer plays, endlessly) plus GameLib's
// self-test switches (--frames N --hidden --require-validation --no-vsync
// --allow-software --resize-at-frame N --minimize-at-frame N).
#include "breakout_game.hpp"
#include "settings.hpp"

#include <gamelib/app/app.hpp>

#include <memory>
#include <optional>

int main(int argc, char** argv)
{
    // Loaded before the window exists, so a saved fullscreen setting is
    // applied while the window is still hidden.
    std::optional<breakout::app::ProfileFile> profile{breakout::app::openProfile(breakout::app::profilePath())};

    const gamelib::app::AppConfig config{
        .title = "Breakout",
        .windowSize = {1280, 720},
        .canvas = {1920, 1080},
        .tick = std::chrono::nanoseconds{1'000'000'000 / 120},
        .fullscreen = profile->value().fullscreen,
        .f11TogglesFullscreen = false, // Breakout handles F11 itself, to save the setting
        .gameFlags = {"--demo"},
    };
    return gamelib::app::runMain(argc, argv, config, [&](gamelib::app::App& app) {
        return std::make_unique<breakout::app::BreakoutGame>(app, std::move(*profile), app.hasFlag("--demo"));
    });
}
