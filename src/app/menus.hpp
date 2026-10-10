#pragma once

#include "settings.hpp"

#include <gamelib/ui/menu.hpp>

namespace breakout::app {

// What Breakout's menu items do (GameLib ui::Menu commands).
enum Command : int { Play, Quit, Resume, Restart, MainMenu, PlayAgain };

// Breakout's menu pages, built on GameLib's ui::Menu. The settings items edit
// `profile` in place, so it must outlive the menus.
struct Menus {
    gamelib::ui::Menu title;
    gamelib::ui::Menu paused;
    gamelib::ui::Menu gameOver;
};

[[nodiscard]] Menus buildMenus(Profile& profile);

// Breakout's colors for GameLib's drawMenu.
[[nodiscard]] gamelib::ui::MenuStyle menuStyle();

inline constexpr const char* kTitleFooter =
    "LEFT / RIGHT OR A / D: MOVE    SPACE: LAUNCH    ESC: PAUSE    F11: FULLSCREEN";

} // namespace breakout::app
