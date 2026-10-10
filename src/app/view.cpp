#include "view.hpp"

#include "palette.hpp"

#include <gamelib/draw/text.hpp>

#include <algorithm>
#include <bit>
#include <cmath>
#include <string>

namespace breakout::app {

using gamelib::core::Color;
using gamelib::core::Rect;
using gamelib::core::Vec2;
using gamelib::draw::Align;

namespace {

constexpr float kBrickGlow = 10.0f;
constexpr float kCapsuleGlow = 14.0f;
constexpr float kLeftHud = game::kFieldLeft / 2.0f;                        // center of the left panel
constexpr float kRightHud = game::kFieldRight + (1920.0f - game::kFieldRight) / 2.0f; // center of the right panel

// The letter on a capsule and in the HUD.
constexpr std::string_view _letter(const game::PowerType type)
{
    switch (type) {
    case game::PowerType::Wide: return "W";
    case game::PowerType::Multi: return "M";
    case game::PowerType::Slow: return "S";
    case game::PowerType::Sticky: return "C";
    }
    return "?";
}


// Overshoots a little past 1 and settles: a playful landing.
float _easeOutBack(const float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    const float u = t - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}
std::uint32_t _seed(const gamelib::core::Cell cell, const int level)
{
    std::uint32_t h = static_cast<std::uint32_t>(cell.x * 73856093) ^ static_cast<std::uint32_t>(cell.y * 19349663)
                      ^ static_cast<std::uint32_t>(level * 83492791);
    h ^= h >> 13;
    h *= 0x5bd1e995u;
    return h ^ (h >> 15);
}

std::uint64_t _floatBits(const float low, const float high)
{
    return std::uint64_t{std::bit_cast<std::uint32_t>(low)} | (std::uint64_t{std::bit_cast<std::uint32_t>(high)} << 32);
}

Rect _shifted(const Rect& r, const Vec2 offset)
{
    return {r.x + offset.x, r.y + offset.y, r.width, r.height};
}

void _brick(gamelib::draw::DrawList& list, const ViewState& view, const gamelib::core::Cell cell,
            const game::Brick& brick)
{
    const Palette& colors = palette();
    Color body = colors.bricks[brick.color % colors.bricks.size()];
    Color light = body.scaled(1.25f);
    std::uint32_t kind = 0;
    float damage = 0.0f;
    float innerLight = 1.0f;
    float glowAlpha = 0.35f;
    switch (brick.kind) {
    case game::BrickKind::Tough:
        kind = 1;
        body = brick.maxHits >= 3 ? colors.tough3 : colors.tough2;
        light = colors.crackLight;
        damage = 1.0f - static_cast<float>(brick.hits) / static_cast<float>(brick.maxHits);
        innerLight = 0.4f;
        glowAlpha = 0.18f;
        break;
    case game::BrickKind::Unbreakable:
        kind = 2;
        body = colors.unbreakable;
        light = colors.rivet;
        innerLight = 0.0f;
        glowAlpha = 0.08f;
        break;
    case game::BrickKind::PowerUp:
        kind = 3;
        body = colors.powerBrick;
        light = powerColor(brick.power);
        innerLight = 0.6f;
        glowAlpha = 0.45f;
        break;
    case game::BrickKind::Normal:
    case game::BrickKind::None: break;
    }
    // The level intro: rows drop in from above one after another and settle
    // with a small overshoot.
    const float rowDelay = 0.04f * static_cast<float>(cell.y) + 0.02f * static_cast<float>(cell.x % 3);
    const float t = std::clamp((view.intro - rowDelay) / 0.55f, 0.0f, 1.0f);
    const Vec2 drop{0.0f, -(1.0f - _easeOutBack(t)) * (110.0f + 14.0f * static_cast<float>(cell.y))};
    const float appear = std::min(1.0f, t * 2.0f);
    list.add({.rect = _shifted(game::brickRect(cell).expanded(kBrickGlow), view.shake + drop),
              .color = body.withAlpha(appear),
              .color2 = light.withAlpha(glowAlpha * appear),
              .shape = view.shapes.brick,
              .params = {damage, view.effects != nullptr ? view.effects->brickFlash(cell) : 0.0f, kBrickGlow, innerLight},
              .bits = std::uint64_t{_seed(cell, view.session.level())} | (std::uint64_t{kind} << 32)});
}

void _label(gamelib::draw::DrawList& list, const std::string_view text, const float x, const float y)
{
    list.text(text, x, y, {.pixelSize = 4.0f, .color = palette().textDim, .align = Align::Center});
}

} // namespace

Color brickColor(const game::BrickKind kind, const std::uint8_t slot, const game::PowerType power)
{
    const Palette& colors = palette();
    switch (kind) {
    case game::BrickKind::Tough: return colors.tough2;
    case game::BrickKind::Unbreakable: return colors.unbreakable;
    case game::BrickKind::PowerUp: return powerColor(power);
    case game::BrickKind::Normal:
    case game::BrickKind::None: break;
    }
    return colors.bricks[slot % colors.bricks.size()];
}

void drawBackdrop(gamelib::draw::DrawList& list, const ViewState& view)
{
    const Palette& colors = palette();
    list.add({.rect = {0.0f, 0.0f, 1920.0f, 1080.0f},
              .color = colors.field,
              .color2 = colors.wall,
              .shape = view.shapes.backdrop,
              .params = {game::kFieldLeft + view.shake.x, game::kFieldTop + view.shake.y, game::kFieldRight + view.shake.x,
                         game::kFieldBottom + view.shake.y},
              .bits = _floatBits(view.wallPulse, view.alarm)});
}

void drawField(gamelib::draw::DrawList& list, const ViewState& view)
{
    const Palette& colors = palette();
    const game::Session& session = view.session;
    const float alpha = view.alpha;

    const game::Bricks& bricks = session.bricks();
    for (int y = 0; y < bricks.height(); ++y) {
        for (int x = 0; x < bricks.width(); ++x) {
            const game::Brick& brick = bricks.at({x, y});
            if (brick.present()) {
                _brick(list, view, {x, y}, brick);
            }
        }
    }
    if (view.effects != nullptr) {
        view.effects->drawDebris(list, view.shake);
    }

    for (const game::PowerUp& powerUp : session.powerUps()) {
        const Vec2 center = powerUp.position.at(alpha) + view.shake;
        const Color color = powerColor(powerUp.type);
        list.add({.rect = Rect::fromCenter(center, game::kPowerUpSize).expanded(kCapsuleGlow),
                  .color = color,
                  .color2 = color.scaled(1.2f).withAlpha(0.6f),
                  .shape = view.shapes.capsule,
                  .params = {kCapsuleGlow, 0.0f, 0.0f, 0.0f}});
        list.text(_letter(powerUp.type), center.x, center.y - 10.5f,
                  {.pixelSize = 3.0f, .color = gamelib::core::srgb(0x10142A), .align = Align::Center});
    }

    const game::Paddle& paddle = session.paddle();
    const bool sticky = session.powerSecondsLeft(game::PowerType::Sticky) > 0.0f;
    const Color rim = sticky ? powerColor(game::PowerType::Sticky) : colors.paddleGlow;
    const Rect paddleRect = Rect::fromCenter({paddle.x.at(alpha), game::kPaddleY},
                                             {paddle.width.at(alpha), game::kPaddleHeight});
    const float flash = view.effects != nullptr ? view.effects->paddleFlash.value() : 0.0f;
    list.box(_shifted(paddleRect.expanded(2.0f * flash), view.shake), gamelib::core::mix(colors.paddle, Color{1.0f, 1.0f, 1.0f, 1.0f}, flash),
             {.cornerRadius = game::kPaddleHeight / 2.0f,
              .borderWidth = 4.0f,
              .borderColor = rim,
              .glowWidth = 22.0f,
              .glowColor = rim.withAlpha(0.55f + 0.4f * flash)});

    if (view.effects != nullptr) {
        view.effects->drawTrails(list, view.shake, colors.ballGlow);
    }
    if (view.launchGuide) {
        // Dots along the path a waiting ball would take: the paddle's aim.
        for (const game::Ball& ball : session.balls()) {
            if (!ball.attached) {
                continue;
            }
            const Vec2 from = ball.position.at(alpha) + view.shake;
            const Vec2 direction = session.launchDirection(ball);
            for (int i = 1; i <= 6; ++i) {
                const float fade = 1.0f - static_cast<float>(i) / 7.0f;
                list.circle(from + direction * (34.0f * static_cast<float>(i)), 3.5f, colors.ball.withAlpha(0.55f * fade));
            }
        }
    }
    // Faster balls glow hotter.
    const float heat = session.speedProgress();
    for (const game::Ball& ball : session.balls()) {
        list.circle(ball.position.at(alpha) + view.shake, game::kBallRadius, colors.ball,
                    {.glowWidth = 14.0f + 10.0f * heat,
                     .glowColor = gamelib::core::mix(colors.ballGlow, colors.danger, heat * 0.6f).withAlpha(0.65f + 0.3f * heat)});
    }
}

void drawHud(gamelib::draw::DrawList& list, const ViewState& view)
{
    const Palette& colors = palette();
    const game::Session& session = view.session;

    // Left panel: score, best, level.
    _label(list, "SCORE", kLeftHud, 110.0f);
    list.number(session.score(), kLeftHud, 150.0f,
                {.height = 64.0f, .color = colors.text, .glowColor = colors.paddleGlow.withAlpha(0.45f),
                 .glowWidth = 12.0f, .align = Align::Center});
    _label(list, "BEST", kLeftHud, 270.0f);
    list.number(std::max(view.best, session.score()), kLeftHud, 310.0f,
                {.height = 40.0f, .color = colors.accent, .glowColor = colors.accent.withAlpha(0.35f),
                 .glowWidth = 8.0f, .align = Align::Center});
    _label(list, "LEVEL", kLeftHud, 420.0f);
    list.number(session.level(), kLeftHud, 460.0f,
                {.height = 64.0f, .color = colors.text, .glowColor = colors.paddleGlow.withAlpha(0.45f),
                 .glowWidth = 12.0f, .align = Align::Center});
    list.text(session.levelName(), kLeftHud, 548.0f,
              {.pixelSize = 4.0f, .color = colors.paddleGlow, .align = Align::Center});

    // Right panel: lives, power-ups, combo.
    _label(list, "LIVES", kRightHud, 110.0f);
    const int lives = session.lives();
    for (int i = 0; i < lives; ++i) {
        const float x = kRightHud + (static_cast<float>(i) - static_cast<float>(lives - 1) / 2.0f) * 48.0f;
        list.circle({x, 172.0f}, 13.0f, colors.ball, {.glowWidth = 12.0f, .glowColor = colors.ballGlow.withAlpha(0.6f)});
    }

    _label(list, "POWER", kRightHud, 270.0f);
    float y = 320.0f;
    for (int i = 0; i < game::kPowerTypeCount; ++i) {
        const auto type = static_cast<game::PowerType>(i);
        const float seconds = session.powerSecondsLeft(type);
        if (seconds <= 0.0f) {
            continue;
        }
        const Color color = powerColor(type);
        list.text(game::kPowerNames.name(type), kRightHud - 130.0f, y, {.pixelSize = 4.0f, .color = color});
        float total = session.rules().wideSeconds;
        if (type == game::PowerType::Slow) {
            total = session.rules().slowSeconds;
        } else if (type == game::PowerType::Sticky) {
            total = session.rules().stickySeconds;
        }
        const float fraction = std::clamp(seconds / total, 0.0f, 1.0f);
        list.box({kRightHud - 130.0f, y + 40.0f, 260.0f, 10.0f}, color.withAlpha(0.15f), {.cornerRadius = 5.0f});
        list.box({kRightHud - 130.0f, y + 40.0f, 260.0f * fraction, 10.0f}, color,
                 {.cornerRadius = 5.0f, .glowWidth = 8.0f, .glowColor = color.withAlpha(0.4f)});
        y += 90.0f;
    }

    if (session.combo() >= 2) {
        _label(list, "COMBO", kRightHud, 720.0f);
        list.text("X" + std::to_string(session.combo()), kRightHud, 760.0f,
                  {.pixelSize = 9.0f, .color = colors.accent, .align = Align::Center, .shadow = true});
    }

    // Ball speed, a thin meter at the bottom of the right panel.
    _label(list, "SPEED", kRightHud, 940.0f);
    const float speed = std::clamp(session.ballSpeed() / session.rules().maxSpeed, 0.0f, 1.0f);
    list.box({kRightHud - 130.0f, 980.0f, 260.0f, 10.0f}, colors.textDim.withAlpha(0.2f), {.cornerRadius = 5.0f});
    list.box({kRightHud - 130.0f, 980.0f, 260.0f * speed, 10.0f}, gamelib::core::mix(colors.paddleGlow, colors.danger, speed),
             {.cornerRadius = 5.0f});
}

} // namespace breakout::app
