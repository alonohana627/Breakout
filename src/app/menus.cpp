#include "menus.hpp"

#include "palette.hpp"

namespace breakout::app {

Menus buildMenus(Profile& profile)
{
    Menus menus{
        .title = gamelib::ui::Menu{"BREAKOUT"},
        .paused = gamelib::ui::Menu{"PAUSED", Resume},        // Escape resumes
        .gameOver = gamelib::ui::Menu{"GAME OVER", MainMenu}, // Escape leaves
    };
    menus.title.action("PLAY", Play)
        .option("SOUND", profile.standard.volume, gamelib::audio::kVolumeLevels)
        .toggle("SCREEN SHAKE", profile.screenShake)
        .toggle("FULLSCREEN", profile.standard.fullscreen)
        .action("QUIT", Quit);
    menus.paused.action("RESUME", Resume)
        .option("SOUND", profile.standard.volume, gamelib::audio::kVolumeLevels)
        .toggle("SCREEN SHAKE", profile.screenShake)
        .toggle("FULLSCREEN", profile.standard.fullscreen)
        .action("RESTART", Restart)
        .action("MAIN MENU", MainMenu)
        .action("QUIT", Quit);
    menus.gameOver.action("PLAY AGAIN", PlayAgain).action("MAIN MENU", MainMenu);
    return menus;
}

gamelib::ui::MenuStyle menuStyle()
{
    using gamelib::core::srgb;
    const Palette& colors = palette();
    return {
        .veil = {0.0f, 0.0f, 0.02f, 0.6f},
        .panel = srgb(0x0E1430, 0.95f),
        .border = colors.wall,
        .title = colors.accent,
        .text = colors.text,
        .highlight = colors.accent,
        .selection = srgb(0x22305E),
        .dim = colors.textDim,
    };
}

} // namespace breakout::app
