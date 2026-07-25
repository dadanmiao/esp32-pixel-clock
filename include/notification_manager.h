/*
 * Author: Yang
 * Small fixed-size notification queue for display overlays.
 */
#pragma once

#include <FastLED.h>

bool enqueueNotification(
    const char *text,
    const CRGB &color = CRGB(0xFF, 0xB8, 0x6B),
    uint16_t durationMs = 5000,
    uint16_t speedMs = 72);
void clearNotifications();
void serviceNotifications();

