/*
 * Author: Yang
 * Lightweight pixel game state helpers.
 */
#pragma once

#include "app_state.h"

void gameReset(GameState &game, const ControlState &control);
void gameStart(GameState &game);
void gamePause(GameState &game);
void gameResume(GameState &game);
void gameSetDirection(GameState &game, GameDirection dir);
void gameUpdate(GameState &game, const ControlState &control, const EnvironmentState &env);
