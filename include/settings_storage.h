/*
 * Author: Yang
 * Persistent user settings stored in ESP32 NVS.
 */
#pragma once

#include "app_state.h"

void loadSettingsFromNvs(ControlState &control);
void saveSettingsToNvs(const ControlState &control);
void clearSettingsNvs();

void requestSettingsSave();
void serviceSettingsSave();
