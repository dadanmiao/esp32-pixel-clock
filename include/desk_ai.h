/*
 * Author: Yang
 * Lightweight on-device desk-state classifier and calibration helpers.
 */
#pragma once

#include "app_state.h"

void initializeDeskAiProfile(ControlState &control);
void resetDeskAiProfile(ControlState &control);
bool calibrateDeskAiProfile(ControlState &control, DeskAiState &deskAi, DeskState label);
bool recordDeskAiEvaluation(DeskAiState &deskAi, DeskState actualLabel);
bool resolveDeskAiFeedback(ControlState &control, DeskAiState &deskAi, DeskState actualLabel);
void resetDeskAiEvaluation(DeskAiState &deskAi);
void refreshDeskAiProfileMetrics(const ControlState &control, DeskAiState &deskAi);
void serviceDeskAi();
const char *deskStateToString(DeskState state);
