/*
 * Author: Yang
 * Dynamic VBUS detection and LED power limit policy.
 */
#include "power_manager.h"

#include <Arduino.h>

#include "adc_sample_bus.h"
#include "app_config.h"
#include "app_state.h"
#include "pinmap.h"

namespace {
TaskHandle_t powerTaskHandle = nullptr;
float filteredVbus = 0.0f;
volatile bool systemFullyStarted = false;
uint32_t systemStartedAtMs = 0;

float readVbusVoltage() {
  uint16_t raw = 0;
  if (!readAdcRaw(Pinmap::VBUS_ADC_CH, raw)) {
    raw = analogRead(static_cast<uint8_t>(Pinmap::VBUS_ADC));
  }
  const float adcVoltage = static_cast<float>(raw) * 3.3f / 4095.0f;
  const float vbus = adcVoltage * AppConfig::VbusDividerRatio;
  if (filteredVbus <= 0.01f) {
    filteredVbus = vbus;
  } else {
    filteredVbus = filteredVbus * 0.85f + vbus * 0.15f;
  }
  return filteredVbus;
}

void applyPowerPolicy(float vbus, PowerState &power) {
  power.vbus = vbus;
  const bool dcdcAllowed = systemFullyStarted && (millis() - systemStartedAtMs >= AppConfig::DcdcEnableDelayMs);
  power.dcdcEnabled = dcdcAllowed;

  if (vbus >= AppConfig::Vbus12vThreshold) {
    power.highPower = true;
    power.maxMilliamps = 15000;
    power.brightnessCap = AppConfig::BrightnessCap12v;
  } else if (vbus >= AppConfig::Vbus5vThreshold) {
    power.highPower = false;
    power.maxMilliamps = 450;
    power.brightnessCap = AppConfig::BrightnessCap5v;
  } else {
    power.highPower = false;
    power.maxMilliamps = 250;
    power.brightnessCap = AppConfig::BrightnessCapUnknown;
  }

  digitalWrite(static_cast<uint8_t>(Pinmap::DCDC_EN), power.dcdcEnabled ? HIGH : LOW);
}

void powerTask(void *) {
  while (true) {
    RenderState state = copySharedState();
    applyPowerPolicy(readVbusVoltage(), state.power);
    updatePowerState(state.power);
    pushRenderSnapshot(0);
    vTaskDelay(pdMS_TO_TICKS(AppConfig::PowerPollMs));
  }
}
} // namespace

void initPowerManager() {
  pinMode(static_cast<uint8_t>(Pinmap::DCDC_EN), OUTPUT);
  digitalWrite(static_cast<uint8_t>(Pinmap::DCDC_EN), LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(static_cast<uint8_t>(Pinmap::VBUS_ADC), ADC_11db);
  analogSetPinAttenuation(static_cast<uint8_t>(Pinmap::MIC_ADC), ADC_11db);
  analogSetPinAttenuation(static_cast<uint8_t>(Pinmap::LDR_ADC), ADC_11db);

  RenderState state = copySharedState();
  applyPowerPolicy(readVbusVoltage(), state.power);
  updatePowerState(state.power);
}

void startPowerTask() {
  xTaskCreatePinnedToCore(powerTask, "power_core0", 8192, nullptr, 4, &powerTaskHandle, 0);
}

void markSystemFullyStarted() {
  systemStartedAtMs = millis() - AppConfig::DcdcEnableDelayMs;
  systemFullyStarted = true;

  RenderState state = copySharedState();
  applyPowerPolicy(readVbusVoltage(), state.power);
  updatePowerState(state.power);
  pushRenderSnapshot(0);
}
