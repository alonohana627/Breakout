#include "view.hpp"

#include <gamelib/core/color.hpp>

#include <string>

namespace breakout::app {

using gamelib::core::srgb;

void drawSession(gamelib::draw::DrawList& list, const game::Session& session, const float alpha)
{
    list.vignette({0, 0, 1920, 1080}, srgb(0x141A30));
    list.box({game::kFieldLeft, game::kFieldTop, game::kFieldWidth, game::kFieldBottom - game::kFieldTop},
             srgb(0x0B1022));
    const game::Bricks& bricks = session.bricks();
    for (int y = 0; y < bricks.height(); ++y) {
        for (int x = 0; x < bricks.width(); ++x) {
            const game::Brick& brick = bricks.at({x, y});
            if (brick.present()) {
                list.box(game::brickRect({x, y}), brick.kind == game::BrickKind::Unbreakable ? srgb(0x606880)
                                                                                           : srgb(0x31C7EF));
            }
        }
    }
    const game::Paddle& paddle = session.paddle();
    list.box(gamelib::core::Rect::fromCenter({paddle.x.at(alpha), game::kPaddleY},
                                             {paddle.width.at(alpha), game::kPaddleHeight}),
             srgb(0xF2F0E5), {.cornerRadius = 11.0f});
    for (const game::Ball& ball : session.balls()) {
        list.circle(ball.position.at(alpha), game::kBallRadius, srgb(0xFFF6E0));
    }
    for (const game::PowerUp& powerUp : session.powerUps()) {
        list.box(gamelib::core::Rect::fromCenter(powerUp.position.at(alpha), game::kPowerUpSize), srgb(0xFFD166),
                 {.cornerRadius = 14.0f});
    }
    list.text("SCORE " + std::to_string(session.score()), 40, 60, {.pixelSize = 5});
    list.text("LIVES " + std::to_string(session.lives()), 40, 120, {.pixelSize = 5});
    list.text("LEVEL " + std::to_string(session.level()), 40, 180, {.pixelSize = 5});
}

} // namespace breakout::app
