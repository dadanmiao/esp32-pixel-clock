/*
 * Author: Yang
 * Dynamic VBUS and LED power management.
 */
#pragma once

#include <Arduino.h>

void initPowerManager();
void startPowerTask();
void markSystemFullyStarted();
uint32_t getPowerTaskStackWatermark();
