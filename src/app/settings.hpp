#pragma once

#include <gamelib/audio/player.hpp>
#include <gamelib/core/options.hpp>
#include <gamelib/storage/persisted.hpp>

#include <cstdint>
#include <filesystem>

namespace breakout::app {

// What Breakout keeps between runs in breakout.ini: the player's settings and
// the best score.
struct Profile {
    gamelib::audio::VolumeLevel volume = gamelib::audio::VolumeLevel::Medium;
    bool fullscreen = false;
    bool screenShake = true;
    std::int64_t best = 0;

    friend bool operator==(const Profile&, const Profile&) = default;
};

inline constexpr gamelib::core::OptionSet<gamelib::audio::VolumeLevel, 4> kVolumes{{"OFF", "LOW", "MEDIUM", "HIGH"}};

// The file, loaded with defaults for anything missing or broken and written
// only when something changed (GameLib's storage::Persisted).
using ProfileFile = gamelib::storage::Persisted<Profile>;

// breakout.ini in the user data folder (%APPDATA%\Breakout on Windows). The
// environment variable BREAKOUT_DATA_DIR overrides the folder (smoke runs use it).
[[nodiscard]] std::filesystem::path profilePath();

[[nodiscard]] ProfileFile openProfile(const std::filesystem::path& path);

} // namespace breakout::app
