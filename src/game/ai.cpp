#include "ai.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace breakout::game {

namespace {

// The line the ball's center is on when it touches the paddle's top.
constexpr float kContactY = kPaddleY - kPaddleHeight / 2.0f - kBallRadius;
constexpr int kLaunchWaitTicks = 45;

} // namespace

DemoPlayer::DemoPlayer(const std::uint64_t seed, const float missChance)
    : m_skill{.reactionTicks = 4, .maxInput = 1.0f, .aimError = 0.0f, .deadZone = 3.0f, .easeDistance = 70.0f}
    , m_rng{seed}
    , m_reaction{m_skill.reactionTicks}
    , m_missChance{missChance}
{
}

void DemoPlayer::plan(const Session& session)
{
    const Paddle& paddle = session.paddle();
    const float halfWidth = paddle.width.value() / 2.0f;

    // The falling ball that arrives first.
    float soonest = std::numeric_limits<float>::max();
    const Ball* incoming = nullptr;
    for (const Ball& ball : session.balls()) {
        const gamelib::core::Vec2 p = ball.position.value();
        if (ball.attached || ball.velocity.y <= 0.0f || p.y > kContactY) {
            continue;
        }
        const float seconds = (kContactY - p.y) / (ball.velocity.y / gamelib::core::length(ball.velocity) * session.ballSpeed());
        if (seconds < soonest) {
            soonest = seconds;
            incoming = &ball;
        }
    }

    if (incoming != nullptr) {
        if (!m_tracking) {
            // A new return: aim somewhere on the paddle, sometimes badly wrong.
            m_tracking = true;
            m_aimOffset = m_rng.uniform(-0.6f, 0.6f) * halfWidth;
            if (m_rng.chance(m_missChance)) {
                m_aimOffset = (m_rng.chance(0.5f) ? -1.0f : 1.0f) * (halfWidth + 3.0f * kBallRadius);
            }
        }
        // predictBounce works along x between two horizontal walls; Breakout's
        // walls are vertical, so x and y swap.
        const gamelib::core::Vec2 p = incoming->position.value();
        const gamelib::core::Vec2 v = incoming->velocity;
        const float x = gamelib::ai::predictBounce({p.y, p.x}, {v.y, v.x}, kContactY, kFieldLeft + kBallRadius,
                                                   kFieldRight - kBallRadius);
        // A power-up arriving well before the ball is worth a detour.
        for (const PowerUp& powerUp : session.powerUps()) {
            const float seconds = (kPaddleY - powerUp.position.value().y) / session.rules().powerUpFallSpeed;
            if (seconds > 0.0f && seconds + 0.6f < soonest
                && std::abs(powerUp.position.value().x - x) < session.rules().paddleSpeed * (soonest - seconds) * 0.6f) {
                m_target = powerUp.position.value().x;
                return;
            }
        }
        m_target = x - m_aimOffset;
        return;
    }
    m_tracking = false;

    // Nothing falling: catch the lowest power-up, or wait under the balls.
    const PowerUp* lowest = nullptr;
    for (const PowerUp& powerUp : session.powerUps()) {
        if (powerUp.position.value().y < kPaddleY && (lowest == nullptr || powerUp.position.value().y > lowest->position.value().y)) {
            lowest = &powerUp;
        }
    }
    if (lowest != nullptr) {
        m_target = lowest->position.value().x;
        return;
    }
    if (!session.balls().empty() && !session.balls().front().attached) {
        m_target = session.balls().front().position.value().x;
    }
}

Input DemoPlayer::decide(const Session& session)
{
    Input input;
    switch (session.phase()) {
    case Phase::GameOver: return input;
    case Phase::LevelClear:
        m_tracking = false;
        if (session.canContinue() && ++m_waitTicks > kLaunchWaitTicks) {
            m_waitTicks = 0;
            input.launch = true;
        }
        return input;
    case Phase::Playing: break;
    }

    const bool waiting = std::ranges::any_of(session.balls(), [](const Ball& b) { return b.attached; });
    if (waiting) {
        // Launch after a short wait from wherever the paddle is.
        if (++m_waitTicks > kLaunchWaitTicks) {
            m_waitTicks = 0;
            input.launch = true;
        }
    } else {
        m_waitTicks = 0;
    }
    if (m_reaction.tick()) {
        plan(session);
    }
    input.move = gamelib::ai::steer(session.paddle().x.value(), m_target, m_skill);
    return input;
}

} // namespace breakout::game
