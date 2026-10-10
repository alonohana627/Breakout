#pragma once

#include "field.hpp"
#include "levels.hpp"

#include <gamelib/core/event_queue.hpp>
#include <gamelib/core/interpolated.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/core/rng.hpp>
#include <gamelib/core/timers.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace breakout::game {

// The rules of Breakout: a paddle, balls, a wall of bricks, power-ups, lives
// and levels. Pure simulation: no window, keys, pixels, sounds or clock - the
// caller advances it in fixed ticks with the player's input, so it is
// deterministic (same seed + same inputs = same game).

struct Rules {
    int lives = 3;
    float paddleWidth = 170.0f;
    float wideWidth = 270.0f;           // with the Wide power-up
    float widthChangeSpeed = 700.0f;    // units per second while growing or shrinking
    float paddleSpeed = 1450.0f;        // units per second at full input
    float paddleAcceleration = 9000.0f; // units per second squared: a little run-up
    float paddleDeceleration = 16000.0f; // stops quicker than it starts
    float startSpeed = 560.0f;          // ball speed at the start of level 1
    float levelSpeedUp = 20.0f;         // added to the start speed per level
    float speedUpPerBrick = 4.0f;       // every brick hit makes the balls a little faster
    float maxSpeed = 1000.0f;
    float slowFactor = 0.62f;           // ball speed while Slow is active
    float maxBounceDegrees = 62.0f;     // off the vertical, at the paddle's very end
    float minVerticalDegrees = 18.0f;   // flatter paths are steepened (no endless side-to-side)
    float powerUpFallSpeed = 260.0f;
    float wideSeconds = 15.0f;
    float slowSeconds = 10.0f;
    float stickySeconds = 15.0f;
    float stickyReleaseSeconds = 3.0f;  // a caught ball launches by itself after this
    float levelClearSeconds = 1.2f;     // before Space continues to the next level
    float multiSpreadDegrees = 24.0f;   // angle between split balls
    int maxBalls = 12;
};

// Points.
inline constexpr int kNormalPoints = 50;
inline constexpr int kToughHitPoints = 30;   // per hit that does not break it
inline constexpr int kToughBreakPoints = 120;
inline constexpr int kPowerBrickPoints = 80;
inline constexpr int kComboStep = 10;        // extra points per brick for each earlier brick in the combo
inline constexpr int kMaxComboBonus = 300;
inline constexpr int kLevelClearPoints = 1000; // times the level number

struct Paddle {
    gamelib::core::Interpolated<float> x{(kFieldLeft + kFieldRight) / 2.0f};
    gamelib::core::Interpolated<float> width{170.0f};
    float velocity = 0.0f; // units per second

    [[nodiscard]] gamelib::core::Rect rect() const
    {
        return gamelib::core::Rect::fromCenter({x.value(), kPaddleY}, {width.value(), kPaddleHeight});
    }
};

struct Ball {
    std::uint32_t id = 0;           // unique within a session, for presentation (trails)
    gamelib::core::Interpolated<gamelib::core::Vec2> position;
    gamelib::core::Vec2 velocity;   // direction matters; the speed is the session's
    bool attached = false;          // riding on the paddle (serve or Sticky)
    float attachOffset = 0.0f;      // x offset from the paddle center while attached
    int attachedTicks = 0;
    bool caught = false;            // attached by Sticky (launches by itself after a while)
};

struct PowerUp {
    PowerType type = PowerType::Wide;
    gamelib::core::Interpolated<gamelib::core::Vec2> position; // center

    [[nodiscard]] gamelib::core::Rect rect() const
    {
        return gamelib::core::Rect::fromCenter(position.value(), kPowerUpSize);
    }
};

enum class Phase : std::uint8_t {
    Playing,    // balls in play or waiting on the paddle
    LevelClear, // every breakable brick is gone; the launch input continues
    GameOver,   // no lives left
};

enum class EventKind : std::uint8_t {
    Launched,       // a ball left the paddle
    PaddleHit,
    WallHit,
    BrickHit,       // a tough brick cracked (did not break)
    BrickBroken,
    UnbreakableHit,
    PowerUpDropped,
    PowerUpCaught,
    PowerUpMissed,  // fell off the bottom
    PowerUpEnded,   // a timed power-up ran out
    Combo,          // the combo reached a new milestone (combo = count)
    BallLost,       // one ball fell off; others are still in play
    LifeLost,
    LevelCleared,   // points = the clear bonus
    LevelStarted,
    GameOver,
};

struct Event {
    EventKind kind = EventKind::Launched;
    gamelib::core::Vec2 position;    // where it happened
    BrickKind brick = BrickKind::None;
    std::uint8_t color = 0;          // Normal bricks: the palette slot
    PowerType power = PowerType::Wide;
    int row = 0;                     // brick row (0 = top)
    int hitsLeft = 0;                // tough bricks
    int points = 0;
    int combo = 0;
};

// What the player does this tick.
struct Input {
    float move = 0.0f;   // -1 full speed left .. +1 full speed right
    bool launch = false; // pressed this tick: launch attached balls, continue after a level
};

class Session {
public:
    // tickSeconds: the fixed tick length; timers count whole ticks of it.
    Session(std::uint64_t seed, float tickSeconds, const Rules& rules = {}, int startLevel = 1);

    // Advances the game by one fixed tick.
    void tick(const Input& input);

    // The events since the last call, oldest first.
    [[nodiscard]] std::vector<Event> takeEvents() { return m_events.take(); }

    [[nodiscard]] const Rules& rules() const { return m_rules; }
    [[nodiscard]] Phase phase() const { return m_phase; }
    [[nodiscard]] int level() const { return m_level; }
    [[nodiscard]] const std::string& levelName() const { return m_levelName; }
    [[nodiscard]] int lives() const { return m_lives; }
    [[nodiscard]] std::int64_t score() const { return m_score; }
    [[nodiscard]] int combo() const { return m_combo; }
    [[nodiscard]] const Bricks& bricks() const { return m_bricks; }
    [[nodiscard]] int bricksLeft() const { return m_bricksLeft; }
    [[nodiscard]] const Paddle& paddle() const { return m_paddle; }
    [[nodiscard]] std::span<const Ball> balls() const { return m_balls; }
    [[nodiscard]] std::span<const PowerUp> powerUps() const { return m_powerUps; }
    // The current ball speed in units per second (Slow included).
    [[nodiscard]] float ballSpeed() const { return m_speed * m_speedFactor; }
    // 0..1 how far the speed has risen from this level's start to the cap.
    [[nodiscard]] float speedProgress() const;
    // Seconds left of a timed power-up (0 when inactive; Multi is never timed).
    [[nodiscard]] float powerSecondsLeft(PowerType type) const;
    // The level-clear wait has run out: the launch input continues.
    [[nodiscard]] bool canContinue() const { return m_phase == Phase::LevelClear && !m_clearDelay.running(); }
    // The unit direction an attached ball would leave in if launched now.
    [[nodiscard]] gamelib::core::Vec2 launchDirection(const Ball& ball) const { return returnOffPaddle(ball.position.value()); }

private:
    void startLevel(int number);
    [[nodiscard]] int ticksFor(float seconds) const { return gamelib::core::Countdown::ticksFor(seconds, m_dt); }
    void serve();
    void movePaddle(const Input& input);
    void pushBallsOffPaddle();
    void moveBall(Ball& ball);
    [[nodiscard]] gamelib::core::Vec2 returnOffPaddle(gamelib::core::Vec2 at) const;
    void hitBrick(gamelib::core::Cell cell, gamelib::core::Vec2 at);
    void launchAttached();
    void movePowerUps();
    void catchPowerUp(PowerType type);
    void tickTimers();
    void loseBalls();
    void clearLevel();
    void addPoints(int points) { m_score += points; }
    [[nodiscard]] gamelib::core::Vec2 steepened(gamelib::core::Vec2 direction) const;

    Rules m_rules;
    std::uint64_t m_seed;
    gamelib::core::Rng m_rng;
    float m_dt;
    Phase m_phase = Phase::Playing;
    int m_level = 1;
    std::string m_levelName;
    int m_lives = 3;
    std::int64_t m_score = 0;
    int m_combo = 0;
    Bricks m_bricks{0, 0};
    int m_bricksLeft = 0;
    Paddle m_paddle;
    std::vector<Ball> m_balls;
    std::uint32_t m_nextBallId = 0;
    std::vector<PowerUp> m_powerUps;
    float m_levelSpeed = 0.0f; // the ball speed this level started with
    float m_speed = 0.0f;
    float m_speedFactor = 1.0f; // eases towards slowFactor while Slow is active
    gamelib::core::Countdown m_wide;
    gamelib::core::Countdown m_slow;
    gamelib::core::Countdown m_sticky;
    gamelib::core::Countdown m_clearDelay;
    gamelib::core::EventQueue<Event> m_events;
};

} // namespace breakout::game
