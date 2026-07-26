/*
 * Author: Yang
 * Pixel game logic for the Game display mode.
 */
#include "game_logic.h"

#include <cmath>

#include <Arduino.h>

namespace {
bool isOpposite(GameDirection a, GameDirection b) {
  return (a == GameDirection::Up && b == GameDirection::Down) ||
         (a == GameDirection::Down && b == GameDirection::Up) ||
         (a == GameDirection::Left && b == GameDirection::Right) ||
         (a == GameDirection::Right && b == GameDirection::Left);
}

bool samePoint(GamePoint a, GamePoint b) {
  return a.x == b.x && a.y == b.y;
}

GamePoint nextPoint(GamePoint point, GameDirection dir) {
  switch (dir) {
    case GameDirection::Up:
      point.y--;
      break;
    case GameDirection::Down:
      point.y++;
      break;
    case GameDirection::Left:
      point.x--;
      break;
    case GameDirection::Right:
    default:
      point.x++;
      break;
  }
  return point;
}

GamePoint wrapPoint(GamePoint point) {
  if (point.x < 0) {
    point.x = GameBoardW - 1;
  } else if (point.x >= GameBoardW) {
    point.x = 0;
  }
  if (point.y < 0) {
    point.y = GameBoardH - 1;
  } else if (point.y >= GameBoardH) {
    point.y = 0;
  }
  return point;
}

bool pointInBounds(GamePoint point) {
  return point.x >= 0 && point.x < GameBoardW && point.y >= 0 && point.y < GameBoardH;
}

uint16_t boundedGameSpeed(uint16_t speedMs) {
  return constrain(speedMs, static_cast<uint16_t>(60), static_cast<uint16_t>(600));
}

bool gameMotionDetected(GameState &game, const EnvironmentState &env) {
  constexpr float MotionThreshold = 0.85f;
  constexpr float MotionReleaseThreshold = 0.45f;
  constexpr uint32_t MotionDebounceMs = 140;

  if (!env.mpuOnline) {
    return false;
  }

  const float motion = fabsf(env.accelX) + fabsf(env.accelY) + fabsf(env.accelZ - 1.0f) +
                       (fabsf(env.gyroX) + fabsf(env.gyroY) + fabsf(env.gyroZ)) * 0.015f;
  const uint32_t now = millis();
  if (motion < MotionReleaseThreshold) {
    game.motionRestartActive = false;
  }
  if (motion < MotionThreshold || game.motionRestartActive || now - game.motionRestartLastMs < MotionDebounceMs) {
    return false;
  }

  game.motionRestartActive = true;
  game.motionRestartLastMs = now;
  return true;
}

uint32_t breakoutFullBrickMask() {
  uint32_t mask = 0;
  for (uint8_t i = 0; i < BreakoutBrickCount; ++i) {
    mask |= 1UL << i;
  }
  return mask;
}

void updateHighScore(GameState &game) {
  if (game.score > game.highScore) {
    game.highScore = game.score;
  }
}

void addGameScore(GameState &game) {
  game.score++;
  updateHighScore(game);
}

void finishGame(GameState &game) {
  game.runState = GameRunState::GameOver;
  updateHighScore(game);
}

void placeFood(GameState &game) {
  for (uint8_t tries = 0; tries < 120; ++tries) {
    const GamePoint food{
        static_cast<int8_t>(random(0, GameBoardW)),
        static_cast<int8_t>(random(0, GameBoardH)),
    };

    bool occupied = false;
    for (uint16_t i = 0; i < game.snakeLen; ++i) {
      if (samePoint(game.snake[i], food)) {
        occupied = true;
        break;
      }
    }

    if (!occupied) {
      game.food = food;
      return;
    }
  }

  game.food = {0, 0};
}

bool snakeHitsSelf(const GameState &game, GamePoint head, bool eating) {
  const uint16_t checkLen = eating || game.snakeLen == 0 ? game.snakeLen : game.snakeLen - 1;
  for (uint16_t i = 0; i < checkLen; ++i) {
    if (samePoint(game.snake[i], head)) {
      return true;
    }
  }
  return false;
}

void resetSnake(GameState &game) {
  game.snakeLen = 3;
  game.snake[0] = {16, 4};
  game.snake[1] = {15, 4};
  game.snake[2] = {14, 4};
  game.dir = GameDirection::Right;
  game.nextDir = GameDirection::Right;
  placeFood(game);
}

void resetGravityBall(GameState &game) {
  game.ball = {16, 4};
  game.target = {25, 4};
  game.nextDir = GameDirection::Right;
}

void resetBreakout(GameState &game) {
  game.breakoutPaddleX = static_cast<int8_t>((GameBoardW - BreakoutPaddleWidth) / 2);
  game.ball = {static_cast<int8_t>(GameBoardW / 2), static_cast<int8_t>(GameBoardH - 3)};
  game.breakoutVelX = random(0, 2) == 0 ? -1 : 1;
  game.breakoutVelY = -1;
  game.breakoutBricks = breakoutFullBrickMask();
  game.nextDir = GameDirection::Up;
}

void resetPong(GameState &game) {
  game.pongPaddleX = static_cast<int8_t>((GameBoardW - BreakoutPaddleWidth) / 2);
  game.ball = {static_cast<int8_t>(GameBoardW / 2), static_cast<int8_t>(GameBoardH - 3)};
  game.pongVelX = random(0, 2) == 0 ? -1 : 1;
  game.pongVelY = -1;
  game.nextDir = GameDirection::Up;
}

void resetReaction(GameState &game) {
  game.reactionReady = false;
  game.reactionStartMs = millis();
  game.reactionWaitMs = random(1500, 5000);
}

int breakoutBrickIndexAt(int x, int y) {
  if (x < 0 || x >= GameBoardW || y < 0 || y >= BreakoutBrickRows) {
    return -1;
  }
  return y * BreakoutBrickCols + x / BreakoutBrickWidth;
}

bool hasBreakoutBrick(const GameState &game, int brickIndex) {
  if (brickIndex < 0 || brickIndex >= BreakoutBrickCount) {
    return false;
  }
  return (game.breakoutBricks & (1UL << brickIndex)) != 0;
}

void clearBreakoutBrick(GameState &game, int brickIndex) {
  if (brickIndex < 0 || brickIndex >= BreakoutBrickCount) {
    return;
  }
  game.breakoutBricks &= ~(1UL << brickIndex);
}

void moveBreakoutPaddle(GameState &game, int8_t dx) {
  if (dx == 0) {
    return;
  }
  game.breakoutPaddleX = static_cast<int8_t>(
      constrain(static_cast<int>(game.breakoutPaddleX) + dx, 0, GameBoardW - BreakoutPaddleWidth));
}

void updateDirectionFromMpu(GameState &game, const EnvironmentState &env) {
  const float ax = env.accelX;
  const float ay = env.accelY;
  if (fabsf(ax) < 0.25f && fabsf(ay) < 0.25f) {
    return;
  }

  if (fabsf(ax) > fabsf(ay)) {
    gameSetDirection(game, ax > 0.25f ? GameDirection::Right : GameDirection::Left);
  } else {
    gameSetDirection(game, ay > 0.25f ? GameDirection::Down : GameDirection::Up);
  }
}

void updateSnake(GameState &game) {
  game.dir = game.nextDir;
  // Snake is intentionally a wrap-around game: crossing any edge continues
  // from the opposite side instead of ending the round.
  const GamePoint newHead = wrapPoint(nextPoint(game.snake[0], game.dir));
  const bool eating = samePoint(newHead, game.food);

  if (snakeHitsSelf(game, newHead, eating)) {
    finishGame(game);
    return;
  }

  uint16_t newLen = game.snakeLen;
  if (eating && newLen < SnakeMaxLen) {
    newLen++;
  }

  for (int i = static_cast<int>(newLen) - 1; i > 0; --i) {
    game.snake[i] = game.snake[i - 1];
  }
  game.snake[0] = newHead;
  game.snakeLen = newLen;

  if (!eating) {
    return;
  }

  game.score++;
  updateHighScore(game);
  if (game.snakeLen >= SnakeMaxLen) {
    finishGame(game);
    return;
  }
  placeFood(game);
}

void updateGravityBall(GameState &game, const EnvironmentState &env, bool useMpu) {
  int8_t dx = 0;
  int8_t dy = 0;

  if (useMpu) {
    if (env.accelX > 0.25f) {
      dx = 1;
    } else if (env.accelX < -0.25f) {
      dx = -1;
    }
    if (env.accelY > 0.25f) {
      dy = 1;
    } else if (env.accelY < -0.25f) {
      dy = -1;
    }
  } else {
    const GamePoint step = nextPoint({0, 0}, game.nextDir);
    dx = step.x;
    dy = step.y;
  }

  game.ball.x = constrain(static_cast<int>(game.ball.x + dx), 0, GameBoardW - 1);
  game.ball.y = constrain(static_cast<int>(game.ball.y + dy), 0, GameBoardH - 1);

  if (!samePoint(game.ball, game.target)) {
    return;
  }

  addGameScore(game);
  for (uint8_t tries = 0; tries < 16; ++tries) {
    game.target = {
        static_cast<int8_t>(random(0, GameBoardW)),
        static_cast<int8_t>(random(0, GameBoardH)),
    };
    if (!samePoint(game.ball, game.target)) {
      break;
    }
  }
}

void updateBreakout(GameState &game, const EnvironmentState &env, bool useMpu) {
  int8_t paddleDx = 0;
  if (useMpu) {
    if (env.accelX > 0.25f) {
      paddleDx = 2;
    } else if (env.accelX < -0.25f) {
      paddleDx = -2;
    }
  } else if (game.nextDir == GameDirection::Left) {
    paddleDx = -2;
  } else if (game.nextDir == GameDirection::Right) {
    paddleDx = 2;
  }
  moveBreakoutPaddle(game, paddleDx);
  if (!useMpu && (game.nextDir == GameDirection::Left || game.nextDir == GameDirection::Right)) {
    game.nextDir = GameDirection::Up;
  }

  int nextX = game.ball.x + game.breakoutVelX;
  int nextY = game.ball.y + game.breakoutVelY;

  if (nextX < 0) {
    nextX = 0;
    game.breakoutVelX = 1;
  } else if (nextX >= GameBoardW) {
    nextX = GameBoardW - 1;
    game.breakoutVelX = -1;
  }

  if (nextY < 0) {
    nextY = 0;
    game.breakoutVelY = 1;
  }

  const int brickIndex = breakoutBrickIndexAt(nextX, nextY);
  if (hasBreakoutBrick(game, brickIndex)) {
    clearBreakoutBrick(game, brickIndex);
    addGameScore(game);
    game.breakoutVelY = 1;
    nextY = constrain(static_cast<int>(game.ball.y + game.breakoutVelY), 0, GameBoardH - 1);
    if (game.breakoutBricks == 0) {
      game.breakoutBricks = breakoutFullBrickMask();
    }
  }

  constexpr int PaddleY = GameBoardH - 1;
  if (nextY >= PaddleY) {
    const int paddleLeft = game.breakoutPaddleX;
    const int paddleRight = paddleLeft + BreakoutPaddleWidth - 1;
    if (nextX < paddleLeft || nextX > paddleRight) {
      finishGame(game);
      return;
    }

    nextY = PaddleY - 1;
    game.breakoutVelY = -1;
    const int hitOffset = nextX - paddleLeft;
    if (hitOffset <= 1) {
      game.breakoutVelX = -1;
    } else if (hitOffset >= BreakoutPaddleWidth - 2) {
      game.breakoutVelX = 1;
    }
  }

  game.ball = {static_cast<int8_t>(nextX), static_cast<int8_t>(nextY)};
}

void updatePong(GameState &game, const EnvironmentState &env, bool useMpu) {
  int8_t paddleDx = 0;
  if (useMpu) {
    if (env.accelX > 0.25f) {
      paddleDx = 2;
    } else if (env.accelX < -0.25f) {
      paddleDx = -2;
    }
  } else if (game.nextDir == GameDirection::Left) {
    paddleDx = -2;
  } else if (game.nextDir == GameDirection::Right) {
    paddleDx = 2;
  }
  if (paddleDx != 0) {
    game.pongPaddleX = static_cast<int8_t>(
        constrain(static_cast<int>(game.pongPaddleX) + paddleDx, 0, GameBoardW - BreakoutPaddleWidth));
  }
  if (!useMpu && (game.nextDir == GameDirection::Left || game.nextDir == GameDirection::Right)) {
    game.nextDir = GameDirection::Up;
  }

  int nextX = game.ball.x + game.pongVelX;
  int nextY = game.ball.y + game.pongVelY;
  if (nextX < 0) {
    nextX = 0;
    game.pongVelX = 1;
  } else if (nextX >= GameBoardW) {
    nextX = GameBoardW - 1;
    game.pongVelX = -1;
  }
  if (nextY < 0) {
    nextY = 0;
    game.pongVelY = 1;
  }

  constexpr int PaddleY = GameBoardH - 1;
  if (nextY >= PaddleY) {
    const int left = game.pongPaddleX;
    const int right = left + BreakoutPaddleWidth - 1;
    if (nextX < left || nextX > right) {
      finishGame(game);
      return;
    }
    nextY = PaddleY - 1;
    game.pongVelY = -1;
    const int hit = nextX - left;
    if (hit <= 1) {
      game.pongVelX = -1;
    } else if (hit >= BreakoutPaddleWidth - 2) {
      game.pongVelX = 1;
    }
    addGameScore(game);
  }
  game.ball = {static_cast<int8_t>(nextX), static_cast<int8_t>(nextY)};
}

void updateReaction(GameState &game) {
  if (!game.reactionReady && millis() - game.reactionStartMs >= game.reactionWaitMs) {
    game.reactionReady = true;
    game.reactionStartMs = millis();
  }
}
} // namespace

void gameReset(GameState &game, const ControlState &control) {
  const uint32_t highScore = game.highScore;
  game = GameState();
  game.highScore = highScore;
  game.type = control.gameType;
  game.runState = GameRunState::Idle;
  game.score = 0;
  game.lastStepMs = millis();
  game.stepIntervalMs = boundedGameSpeed(control.gameSpeedMs);

  switch (game.type) {
    case GameType::Breakout:
      resetBreakout(game);
      break;
    case GameType::GravityBall:
      resetGravityBall(game);
      break;
    case GameType::Reaction:
      resetReaction(game);
      break;
    case GameType::Pong:
      resetPong(game);
      break;
    case GameType::Snake:
    default:
      resetSnake(game);
      break;
  }
}

void gameStart(GameState &game) {
  if (game.type == GameType::Reaction) {
    resetReaction(game);
  }
  game.runState = GameRunState::Running;
  game.lastStepMs = millis();
}

void gamePause(GameState &game) {
  if (game.runState == GameRunState::Running) {
    game.runState = GameRunState::Paused;
  }
}

void gameResume(GameState &game) {
  if (game.runState == GameRunState::Paused) {
    game.runState = GameRunState::Running;
    game.lastStepMs = millis();
  }
}

void gameSetDirection(GameState &game, GameDirection dir) {
  if (game.type == GameType::Reaction && game.runState == GameRunState::Running) {
    if (!game.reactionReady) {
      finishGame(game);
      return;
    }
    addGameScore(game);
    resetReaction(game);
    return;
  }
  if (game.type == GameType::Snake && game.runState == GameRunState::Running && isOpposite(game.dir, dir)) {
    return;
  }
  game.nextDir = dir;
}

void gameUpdate(GameState &game, const ControlState &control, const EnvironmentState &env) {
  if (game.type != control.gameType) {
    gameReset(game, control);
  }

  game.stepIntervalMs = boundedGameSpeed(control.gameSpeedMs);
  if (game.runState == GameRunState::GameOver) {
    if (control.gameUseMpuControl && gameMotionDetected(game, env)) {
      gameReset(game, control);
      gameStart(game);
    }
    return;
  }
  if (game.runState == GameRunState::Idle) {
    if (control.gameUseMpuControl && gameMotionDetected(game, env)) {
      gameStart(game);
    } else {
      return;
    }
  }
  if (control.gameUseMpuControl && game.type != GameType::Breakout &&
      game.type != GameType::Pong && game.type != GameType::Reaction) {
    updateDirectionFromMpu(game, env);
  }
  if (game.runState != GameRunState::Running) {
    return;
  }

  if (game.type == GameType::Reaction) {
    updateReaction(game);
    return;
  }

  const uint32_t now = millis();
  if (now - game.lastStepMs < game.stepIntervalMs) {
    return;
  }
  game.lastStepMs = now;

  switch (game.type) {
    case GameType::Breakout:
      updateBreakout(game, env, control.gameUseMpuControl);
      break;
    case GameType::GravityBall:
      updateGravityBall(game, env, control.gameUseMpuControl);
      break;
    case GameType::Pong:
      updatePong(game, env, control.gameUseMpuControl);
      break;
    case GameType::Snake:
      updateSnake(game);
      break;
    default:
      break;
  }
}
