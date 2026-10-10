#include "session.hpp"

#include <gamelib/core/collision.hpp>

#include <algorithm>
#include <cmath>

namespace breakout::game {

using gamelib::core::Cell;
using gamelib::core::Rect;
using gamelib::core::Vec2;

namespace {

// Collider ids for moveAndCollide; bricks are kBrickId + their cell index.
constexpr int kWallId = 0;
constexpr int kPaddleId = 1;
constexpr int kBrickId = 100;

// The walls reach far beyond the field, so a ball can never slip around their ends.
constexpr Rect kLeftWall{-10'000.0f, -10'000.0f, 10'000.0f + kFieldLeft, 30'000.0f};
constexpr Rect kRightWall{kFieldRight, -10'000.0f, 10'000.0f, 30'000.0f};
constexpr Rect kTopWall{-10'000.0f, -10'000.0f, 30'000.0f, 10'000.0f + kFieldTop};

float _radians(const float degrees)
{
    return degrees * gamelib::core::kPi / 180.0f;
}

bool _comboMilestone(const int combo)
{
    return combo == 3 || combo == 5 || (combo >= 10 && combo % 5 == 0);
}

int _brickPoints(const BrickKind kind)
{
    switch (kind) {
    case BrickKind::Tough: return kToughBreakPoints;
    case BrickKind::PowerUp: return kPowerBrickPoints;
    case BrickKind::Normal:
    case BrickKind::None:
    case BrickKind::Unbreakable: break;
    }
    return kNormalPoints;
}

} // namespace

Session::Session(const std::uint64_t seed, const float tickSeconds, const Rules& rules, const int startLevel)
    : m_rules{rules}
    , m_seed{seed}
    , m_rng{seed}
    , m_dt{tickSeconds}
    , m_lives{rules.lives}
{
    m_paddle.width.snap(m_rules.paddleWidth);
    this->startLevel(std::max(startLevel, 1));
}

float Session::speedProgress() const
{
    const float range = m_rules.maxSpeed - m_levelSpeed;
    return range > 0.0f ? std::clamp((m_speed - m_levelSpeed) / range, 0.0f, 1.0f) : 1.0f;
}

float Session::powerSecondsLeft(const PowerType type) const
{
    switch (type) {
    case PowerType::Wide: return static_cast<float>(m_wide.remaining()) * m_dt;
    case PowerType::Slow: return static_cast<float>(m_slow.remaining()) * m_dt;
    case PowerType::Sticky: return static_cast<float>(m_sticky.remaining()) * m_dt;
    case PowerType::Multi: break;
    }
    return 0.0f;
}

void Session::startLevel(const int number)
{
    Level level = makeLevel(number, m_seed);
    m_level = number;
    m_levelName = std::move(level.name);
    m_bricks = std::move(level.bricks);
    m_bricksLeft = breakableCount(m_bricks);
    m_levelSpeed = std::min(m_rules.startSpeed + m_rules.levelSpeedUp * static_cast<float>(number - 1),
                            m_rules.maxSpeed * 0.85f);
    m_speed = m_levelSpeed;
    m_speedFactor = 1.0f;
    m_powerUps.clear();
    m_wide.stop();
    m_slow.stop();
    m_sticky.stop();
    m_phase = Phase::Playing;
    m_events.push({.kind = EventKind::LevelStarted});
    serve();
}

void Session::serve()
{
    m_balls.clear();
    m_combo = 0;
    // A small random offset, so a launch without moving is not always straight up.
    const float offset = m_rng.uniform(-0.3f, 0.3f) * m_paddle.width.value() / 2.0f;
    Ball ball{.attached = true, .attachOffset = offset};
    ball.position.snap({m_paddle.x.value() + offset, kPaddleY - kPaddleHeight / 2.0f - kBallRadius});
    m_balls.push_back(ball);
}

void Session::tick(const Input& input)
{
    m_paddle.x.beginTick();
    m_paddle.width.beginTick();
    for (Ball& ball : m_balls) {
        ball.position.beginTick();
    }
    for (PowerUp& powerUp : m_powerUps) {
        powerUp.position.beginTick();
    }

    switch (m_phase) {
    case Phase::GameOver: return;
    case Phase::LevelClear:
        movePaddle(input);
        m_clearDelay.tick();
        if (input.launch && canContinue()) {
            startLevel(m_level + 1);
        }
        return;
    case Phase::Playing: break;
    }

    movePaddle(input);
    // Attached balls ride on the paddle; a ball caught by Sticky launches by
    // itself after a while, so the game never stalls.
    bool autoRelease = false;
    for (Ball& ball : m_balls) {
        if (ball.attached) {
            ball.position.value() = {m_paddle.x.value() + ball.attachOffset,
                                     kPaddleY - kPaddleHeight / 2.0f - kBallRadius};
            ++ball.attachedTicks;
            autoRelease = autoRelease || (ball.caught && ball.attachedTicks >= ticksFor(m_rules.stickyReleaseSeconds));
        }
    }
    if (input.launch || autoRelease) {
        launchAttached();
    }
    pushBallsOffPaddle();

    const float targetFactor = m_slow.running() ? m_rules.slowFactor : 1.0f;
    m_speedFactor = gamelib::core::approach(m_speedFactor, targetFactor, 1.5f * m_dt);
    // Index loop: hitting a brick never adds balls, but keep it robust anyway.
    for (std::size_t i = 0; i < m_balls.size(); ++i) {
        moveBall(m_balls[i]);
    }
    loseBalls();
    if (m_phase != Phase::Playing) {
        return;
    }
    movePowerUps();
    tickTimers();
    if (m_bricksLeft == 0) {
        clearLevel();
    }
}

void Session::movePaddle(const Input& input)
{
    // Acceleration towards the wanted speed: a short run-up, a quicker stop.
    const float move = std::clamp(input.move, -1.0f, 1.0f);
    const float wanted = move * m_rules.paddleSpeed;
    const bool speedingUp = move != 0.0f && (m_paddle.velocity * wanted >= 0.0f)
                            && std::abs(wanted) > std::abs(m_paddle.velocity);
    const float rate = speedingUp ? m_rules.paddleAcceleration : m_rules.paddleDeceleration;
    m_paddle.velocity = gamelib::core::approach(m_paddle.velocity, wanted, rate * m_dt);

    const float targetWidth = m_wide.running() ? m_rules.wideWidth : m_rules.paddleWidth;
    float& width = m_paddle.width.value();
    width = gamelib::core::approach(width, targetWidth, m_rules.widthChangeSpeed * m_dt);

    float& x = m_paddle.x.value();
    x += m_paddle.velocity * m_dt;
    const float minX = kFieldLeft + width / 2.0f;
    const float maxX = kFieldRight - width / 2.0f;
    if (x < minX || x > maxX) {
        x = std::clamp(x, minX, maxX);
        m_paddle.velocity = 0.0f;
    }
}

void Session::pushBallsOffPaddle()
{
    // The paddle can move (or grow) into a ball; the sweep only handles the
    // ball moving. Push such a ball out: up and returned if it is above the
    // paddle's middle, sideways otherwise.
    const Rect paddle = m_paddle.rect();
    for (Ball& ball : m_balls) {
        Vec2& p = ball.position.value();
        if (ball.attached || !gamelib::core::overlaps(p, kBallRadius, paddle)) {
            continue;
        }
        if (p.y <= paddle.center().y) {
            p.y = paddle.top() - kBallRadius - 0.01f;
            if (ball.velocity.y > 0.0f) {
                ball.velocity = returnOffPaddle(p);
                m_combo = 0;
                m_events.push({.kind = EventKind::PaddleHit, .position = p});
            }
        } else {
            const bool left = p.x < paddle.center().x;
            p.x = std::clamp(left ? paddle.left() - kBallRadius - 0.01f : paddle.right() + kBallRadius + 0.01f,
                             kFieldLeft + kBallRadius, kFieldRight - kBallRadius);
            ball.velocity.x = (left ? -1.0f : 1.0f) * std::abs(ball.velocity.x);
        }
    }
}

void Session::moveBall(Ball& ball)
{
    if (ball.attached) {
        return;
    }
    std::vector<gamelib::core::Collider> colliders{
        {kLeftWall, kWallId}, {kRightWall, kWallId}, {kTopWall, kWallId}, {m_paddle.rect(), kPaddleId}};
    for (int y = 0; y < m_bricks.height(); ++y) {
        for (int x = 0; x < m_bricks.width(); ++x) {
            if (m_bricks.at({x, y}).present()) {
                colliders.push_back({brickRect({x, y}), kBrickId + y * kColumns + x});
            }
        }
    }

    bool caught = false;
    const float speed = ballSpeed();
    const gamelib::core::MoveResult moved = gamelib::core::moveAndCollide(
        ball.position.value(), kBallRadius, gamelib::core::normalized(ball.velocity) * speed, m_dt, colliders,
        [&](const gamelib::core::Contact& contact) -> Vec2 {
            if (contact.id == kWallId) {
                m_events.push({.kind = EventKind::WallHit, .position = contact.hit.center});
                return gamelib::core::bounce(contact);
            }
            if (contact.id == kPaddleId) {
                if (contact.hit.normal.y > -0.3f || contact.velocity.y <= 0.0f) {
                    return gamelib::core::bounce(contact); // the paddle's ends
                }
                m_combo = 0; // touching the paddle ends the combo
                m_events.push({.kind = EventKind::PaddleHit, .position = contact.hit.center});
                if (m_sticky.running()) {
                    caught = true;
                    return {}; // stops here; attached below
                }
                return returnOffPaddle(contact.hit.center) * speed;
            }
            const int index = contact.id - kBrickId;
            hitBrick({index % kColumns, index / kColumns}, contact.hit.center);
            return gamelib::core::bounce(contact);
        });

    ball.position.value() = moved.center;
    if (caught) {
        ball.attached = true;
        ball.caught = true;
        ball.attachedTicks = 0;
        ball.attachOffset = std::clamp(moved.center.x - m_paddle.x.value(), -m_paddle.width.value() / 2.0f,
                                       m_paddle.width.value() / 2.0f);
        ball.position.value() = {m_paddle.x.value() + ball.attachOffset, kPaddleY - kPaddleHeight / 2.0f - kBallRadius};
        return;
    }
    ball.velocity = steepened(moved.velocity) * speed;
}

Vec2 Session::returnOffPaddle(const Vec2 at) const
{
    // Where the ball meets the paddle, -1 (left end) to +1 (right end): the
    // further from the middle, the flatter the return - the player's aim.
    const float reach = m_paddle.width.value() / 2.0f + kBallRadius;
    const float offset = std::clamp((at.x - m_paddle.x.value()) / reach, -1.0f, 1.0f);
    const float angle = offset * _radians(m_rules.maxBounceDegrees);
    return {std::sin(angle), -std::cos(angle)};
}

Vec2 Session::steepened(const Vec2 velocity) const
{
    Vec2 direction = gamelib::core::normalized(velocity);
    if (direction == Vec2{}) {
        return {0.0f, -1.0f};
    }
    const float minY = std::sin(_radians(m_rules.minVerticalDegrees));
    if (std::abs(direction.y) < minY) {
        direction.y = direction.y < 0.0f ? -minY : minY;
        direction.x = (direction.x < 0.0f ? -1.0f : 1.0f) * std::sqrt(1.0f - minY * minY);
    }
    return direction;
}

void Session::hitBrick(const Cell cell, const Vec2 at)
{
    Brick& brick = m_bricks.at(cell);
    if (!brick.present()) {
        return; // broken earlier in this same move: the ball still bounces off where it was
    }
    const Vec2 center = brickRect(cell).center();
    if (brick.kind == BrickKind::Unbreakable) {
        m_events.push({.kind = EventKind::UnbreakableHit, .position = at, .brick = brick.kind, .row = cell.y});
        return;
    }
    m_speed = std::min(m_speed + m_rules.speedUpPerBrick, m_rules.maxSpeed);
    ++m_combo;
    const int bonus = std::min(kComboStep * (m_combo - 1), kMaxComboBonus);
    --brick.hits;
    if (brick.hits > 0) {
        const int points = kToughHitPoints + bonus;
        addPoints(points);
        m_events.push({.kind = EventKind::BrickHit,
                       .position = center,
                       .brick = brick.kind,
                       .row = cell.y,
                       .hitsLeft = brick.hits,
                       .points = points,
                       .combo = m_combo});
    } else {
        const int points = _brickPoints(brick.kind) + bonus;
        addPoints(points);
        m_events.push({.kind = EventKind::BrickBroken,
                       .position = center,
                       .brick = brick.kind,
                       .color = brick.color,
                       .power = brick.power,
                       .row = cell.y,
                       .points = points,
                       .combo = m_combo});
        if (brick.kind == BrickKind::PowerUp) {
            PowerUp powerUp{.type = brick.power};
            powerUp.position.snap(center);
            m_powerUps.push_back(powerUp);
            m_events.push({.kind = EventKind::PowerUpDropped, .position = center, .power = brick.power});
        }
        brick = Brick{};
        --m_bricksLeft;
    }
    if (_comboMilestone(m_combo)) {
        m_events.push({.kind = EventKind::Combo, .position = center, .combo = m_combo});
    }
}

void Session::launchAttached()
{
    for (Ball& ball : m_balls) {
        if (!ball.attached) {
            continue;
        }
        ball.attached = false;
        ball.caught = false;
        ball.velocity = returnOffPaddle(ball.position.value());
        m_events.push({.kind = EventKind::Launched, .position = ball.position.value()});
    }
}

void Session::movePowerUps()
{
    const Rect paddle = m_paddle.rect();
    std::vector<PowerType> caught;
    std::erase_if(m_powerUps, [&](PowerUp& powerUp) {
        powerUp.position.value().y += m_rules.powerUpFallSpeed * m_dt;
        if (gamelib::core::overlaps(powerUp.rect(), paddle)) {
            caught.push_back(powerUp.type);
            m_events.push({.kind = EventKind::PowerUpCaught, .position = powerUp.position.value(), .power = powerUp.type});
            return true;
        }
        if (powerUp.rect().top() > kFieldBottom) {
            m_events.push({.kind = EventKind::PowerUpMissed, .position = powerUp.position.value(), .power = powerUp.type});
            return true;
        }
        return false;
    });
    for (const PowerType type : caught) {
        catchPowerUp(type);
    }
}

void Session::catchPowerUp(const PowerType type)
{
    switch (type) {
    case PowerType::Wide: m_wide.start(ticksFor(m_rules.wideSeconds)); break;
    case PowerType::Slow: m_slow.start(ticksFor(m_rules.slowSeconds)); break;
    case PowerType::Sticky: m_sticky.start(ticksFor(m_rules.stickySeconds)); break;
    case PowerType::Multi: {
        if (std::ranges::all_of(m_balls, [](const Ball& b) { return b.attached; })) {
            launchAttached(); // something has to be in flight to split
        }
        const std::vector<Ball> flying = m_balls;
        for (const Ball& ball : flying) {
            if (ball.attached) {
                continue;
            }
            for (const float sign : {-1.0f, 1.0f}) {
                if (static_cast<int>(m_balls.size()) >= m_rules.maxBalls) {
                    break;
                }
                const float angle = sign * _radians(m_rules.multiSpreadDegrees);
                const Vec2 v = gamelib::core::normalized(ball.velocity);
                Ball split = ball;
                split.velocity = steepened({v.x * std::cos(angle) - v.y * std::sin(angle),
                                            v.x * std::sin(angle) + v.y * std::cos(angle)});
                m_balls.push_back(split);
            }
        }
        break;
    }
    }
}

void Session::tickTimers()
{
    if (m_wide.tick()) {
        m_events.push({.kind = EventKind::PowerUpEnded, .power = PowerType::Wide});
    }
    if (m_slow.tick()) {
        m_events.push({.kind = EventKind::PowerUpEnded, .power = PowerType::Slow});
    }
    if (m_sticky.tick()) {
        m_events.push({.kind = EventKind::PowerUpEnded, .power = PowerType::Sticky});
        // Release what Sticky caught; a ball waiting for its serve stays.
        for (Ball& ball : m_balls) {
            if (ball.caught) {
                ball.attachedTicks = ticksFor(m_rules.stickyReleaseSeconds);
            }
        }
    }
}

void Session::loseBalls()
{
    std::erase_if(m_balls, [&](const Ball& ball) {
        if (ball.position.value().y - kBallRadius > kFieldBottom) {
            m_events.push({.kind = EventKind::BallLost, .position = ball.position.value()});
            return true;
        }
        return false;
    });
    if (!m_balls.empty()) {
        return;
    }
    --m_lives;
    m_combo = 0;
    m_powerUps.clear();
    m_wide.stop();
    m_slow.stop();
    m_sticky.stop();
    m_speedFactor = 1.0f;
    m_speed = m_levelSpeed;
    m_events.push({.kind = EventKind::LifeLost, .position = {m_paddle.x.value(), kPaddleY}});
    if (m_lives <= 0) {
        m_lives = 0;
        m_phase = Phase::GameOver;
        m_events.push({.kind = EventKind::GameOver});
        return;
    }
    serve();
}

void Session::clearLevel()
{
    const int points = kLevelClearPoints * m_level;
    addPoints(points);
    m_phase = Phase::LevelClear;
    m_balls.clear();
    m_powerUps.clear();
    m_wide.stop();
    m_slow.stop();
    m_sticky.stop();
    m_clearDelay.start(ticksFor(m_rules.levelClearSeconds));
    m_events.push({.kind = EventKind::LevelCleared, .points = points});
}

} // namespace breakout::game
