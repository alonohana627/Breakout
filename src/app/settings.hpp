#pragma once

#include <gamelib/app/settings.hpp>
#include <gamelib/storage/persisted.hpp>

#include <cstdint>
#include <filesystem>

namespace breakout::app {

// What Breakout keeps between runs in breakout.ini: the player's settings and
// the best score. Sound volume and fullscreen are GameLib's standard settings.
struct Profile {
    gamelib::app::StandardSettings standard;
    bool screenShake = true;
    std::int64_t best = 0;

    friend bool operator==(const Profile&, const Profile&) = default;
};

// The file, loaded with defaults for anything missing or broken and written
// only when something changed (GameLib's storage::Persisted).
using ProfileFile = gamelib::storage::Persisted<Profile>;

// breakout.ini in the user data folder (%APPDATA%\Breakout on Windows). The
// environment variable BREAKOUT_DATA_DIR overrides the folder (smoke runs use it).
[[nodiscard]] std::filesystem::path profilePath();

[[nodiscard]] ProfileFile openProfile(const std::filesystem::path& path);

} // namespace breakout::app
