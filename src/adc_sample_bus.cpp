/*
 * Author: Yang
 * Lock-light ADC sample cache shared between Core 0 tasks.
 */
#include "adc_sample_bus.h"

namespace {
portMUX_TYPE adcMux = portMUX_INITIALIZER_UNLOCKED;
uint16_t latestRaw[10] = {};
bool hasRaw[10] = {};
} // namespace

void publishAdcRaw(adc_channel_t channel, uint16_t raw) {
  const uint8_t index = static_cast<uint8_t>(channel);
  if (index >= 10) {
    return;
  }
  portENTER_CRITICAL(&adcMux);
  latestRaw[index] = raw;
  hasRaw[index] = true;
  portEXIT_CRITICAL(&adcMux);
}

bool readAdcRaw(adc_channel_t channel, uint16_t &raw) {
  const uint8_t index = static_cast<uint8_t>(channel);
  if (index >= 10) {
    return false;
  }
  bool ok = false;
  portENTER_CRITICAL(&adcMux);
  ok = hasRaw[index];
  raw = latestRaw[index];
  portEXIT_CRITICAL(&adcMux);
  return ok;
}

