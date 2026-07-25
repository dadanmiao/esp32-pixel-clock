/*
 * Author: Yang
 * Small fixed-size notification queue for display overlays.
 */
#include "notification_manager.h"

#include <cstring>

#include <Arduino.h>

#include "app_config.h"
#include "app_state.h"

namespace {
void copyNotificationText(char *dest, const char *src) {
  size_t out = 0;
  if (src) {
    while (out < AppConfig::NotificationTextMaxLen - 1 && src[out] != '\0') {
      const char c = src[out];
      dest[out] = c >= 0x20 && c <= 0x7E ? c : ' ';
      ++out;
    }
  }
  dest[out] = '\0';
}
} // namespace

bool enqueueNotification(
    const char *text,
    const CRGB &color,
    uint16_t durationMs,
    uint16_t speedMs) {
  if (!text || text[0] == '\0') {
    return false;
  }

  NotificationState state = copyNotificationState();
  NotificationItem item;
  copyNotificationText(item.text, text);
  item.color = color;
  item.durationMs = static_cast<uint16_t>(constrain(
      durationMs, static_cast<uint16_t>(1200), static_cast<uint16_t>(30000)));
  item.speedMs = static_cast<uint16_t>(constrain(
      speedMs, static_cast<uint16_t>(30), static_cast<uint16_t>(240)));

  if (!state.activeVisible) {
    state.active = item;
    state.activeVisible = true;
    state.activeStartedMs = millis();
  } else if (state.count < AppConfig::NotificationQueueDepth) {
    state.queue[state.count++] = item;
  } else {
    for (size_t i = 1; i < AppConfig::NotificationQueueDepth; ++i) {
      state.queue[i - 1] = state.queue[i];
    }
    state.queue[AppConfig::NotificationQueueDepth - 1] = item;
  }
  state.serial++;
  updateNotificationState(state);
  pushRenderSnapshot(0);
  return true;
}

void clearNotifications() {
  NotificationState state;
  state.serial = copyNotificationState().serial + 1;
  updateNotificationState(state);
  pushRenderSnapshot(0);
}

void serviceNotifications() {
  NotificationState state = copyNotificationState();
  if (!state.activeVisible ||
      millis() - state.activeStartedMs < state.active.durationMs) {
    return;
  }

  if (state.count == 0) {
    state.activeVisible = false;
    state.activeStartedMs = 0;
  } else {
    state.active = state.queue[0];
    for (uint8_t i = 1; i < state.count; ++i) {
      state.queue[i - 1] = state.queue[i];
    }
    state.count--;
    state.activeVisible = true;
    state.activeStartedMs = millis();
  }
  state.serial++;
  updateNotificationState(state);
  pushRenderSnapshot(0);
}

