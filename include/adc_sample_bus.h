/*
 * Author: Yang
 * Latest raw ADC samples produced by the continuous DMA audio task.
 */
#pragma once

#include <Arduino.h>
#include "hal/adc_types.h"

void publishAdcRaw(adc_channel_t channel, uint16_t raw);
bool readAdcRaw(adc_channel_t channel, uint16_t &raw);

