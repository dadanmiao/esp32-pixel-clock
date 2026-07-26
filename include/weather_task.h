/*
 * Author: Yang
 * Open-Meteo weather fetch task.
 */
#pragma once

#include <Arduino.h>

void startWeatherTask();
void requestWeatherRefresh();
uint32_t getWeatherTaskStackWatermark();
