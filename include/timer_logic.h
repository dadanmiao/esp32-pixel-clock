/*
 * Author: Yang
 * Timer, countdown, and stopwatch state helpers.
 */
#pragma once

#include <Arduino.h>

#include "app_state.h"

uint32_t timerRemainingSec(const ControlState &control, uint32_t nowMs);
uint32_t stopwatchElapsedSec(const ControlState &control, uint32_t nowMs);

void setTimerMode(ControlState &control, TimerMode mode);
void setCountdownMinutes(ControlState &control, uint16_t minutes);
void setPomodoroFocusMinutes(ControlState &control, uint16_t minutes);
void setPomodoroBreakMinutes(ControlState &control, uint16_t minutes);

void startTimer(ControlState &control, uint32_t nowMs);
void pauseTimer(ControlState &control, uint32_t nowMs);
void resumeTimer(ControlState &control, uint32_t nowMs);
void resetTimer(ControlState &control);
void applyTimerAction(ControlState &control, const char *action, uint32_t nowMs);

bool serviceTimerState(uint32_t nowMs);
