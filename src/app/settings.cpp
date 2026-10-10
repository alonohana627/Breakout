#include "settings.hpp"

#include <gamelib/storage/files.hpp>
#include <gamelib/storage/ini.hpp>

#include <algorithm>
#include <string>

namespace breakout::app {

namespace {

constexpr const char* kFileName = "breakout.ini";
constexpr int kFormatVersion = 1;

} // namespace

std::filesystem::path profilePath()
{
    return gamelib::storage::userDataDir("Breakout", "BREAKOUT_DATA_DIR") / kFileName;
}

ProfileFile openProfile(const std::filesystem::path& path)
{
    using gamelib::storage::IniFile;
    return ProfileFile{
        path,
        [](const IniFile& ini) {
            Profile profile; // defaults; the file only overrides
            if (const auto value = ini.get("settings", "sound")) {
                profile.volume = kVolumes.parse(*value).value_or(profile.volume);
            }
            if (const auto value = ini.get("settings", "fullscreen")) {
                profile.fullscreen = gamelib::storage::parseBool(*value).value_or(profile.fullscreen);
            }
            if (const auto value = ini.get("settings", "screen_shake")) {
                profile.screenShake = gamelib::storage::parseBool(*value).value_or(profile.screenShake);
            }
            if (const auto value = ini.get("scores", "best")) {
                profile.best = std::max<long long>(0, gamelib::storage::parseInt(*value).value_or(0));
            }
            return profile;
        },
        [](const Profile& profile, IniFile& ini) {
            ini.set("", "version", std::to_string(kFormatVersion));
            ini.set("settings", "sound", gamelib::storage::toLower(kVolumes.name(profile.volume)));
            ini.set("settings", "fullscreen", profile.fullscreen ? "true" : "false");
            ini.set("settings", "screen_shake", profile.screenShake ? "true" : "false");
            ini.set("scores", "best", std::to_string(profile.best));
        },
        "Breakout settings and best score. sound: off/low/medium/high, fullscreen and screen_shake: true/false."};
}

} // namespace breakout::app
