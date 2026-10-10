#pragma once

#include "session.hpp"

#include <gamelib/ai/opponent.hpp>
#include <gamelib/core/rng.hpp>

#include <cstdint>

namespace breakout::game {

// The computer player of the title screen's attract demo and of --demo.
// Composed from GameLib's ai helpers: it predicts where the next falling ball
// crosses the paddle's line (ai::predictBounce, sideways between the walls),
// picks a random spot on the paddle to hit it with for a varied return, now
// and then misjudges badly (so demos end), re-plans every few ticks
// (ai::ReactionTimer) and steers with a dead zone (ai::steer). It catches
// power-ups when no ball needs it and launches after a short wait.
// Deterministic for a seed.
class DemoPlayer {
public:
    explicit DemoPlayer(std::uint64_t seed, float missChance = 0.04f);

    [[nodiscard]] Input decide(const Session& session);

    // The x the paddle is heading for.
    [[nodiscard]] float target() const { return m_target; }

private:
    void plan(const Session& session);

    gamelib::ai::Skill m_skill;
    gamelib::core::Rng m_rng;
    gamelib::ai::ReactionTimer m_reaction;
    float m_missChance;
    float m_target = (kFieldLeft + kFieldRight) / 2.0f;
    float m_aimOffset = 0.0f;  // where on the paddle it wants the ball, for this return
    bool m_tracking = false;   // a falling ball is being tracked
    int m_waitTicks = 0;       // before launching or continuing
};

} // namespace breakout::game
