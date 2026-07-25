/*
 * Author: Yang
 * Pin map for ESP32-S3 Desktop Pixel Clock and Fluid Rhythm Terminal.
 */
#pragma once

#include <Arduino.h>
#include "hal/adc_types.h"

namespace Pinmap {
constexpr gpio_num_t WS2812_DATA = GPIO_NUM_15;
constexpr gpio_num_t DCDC_EN = GPIO_NUM_16;
constexpr gpio_num_t VBUS_ADC = GPIO_NUM_8;
constexpr gpio_num_t MIC_ADC = GPIO_NUM_9;
constexpr gpio_num_t LDR_ADC = GPIO_NUM_10;
constexpr gpio_num_t I2C_SDA = GPIO_NUM_14;
constexpr gpio_num_t I2C_SCL = GPIO_NUM_21;
constexpr gpio_num_t MPU_INT = GPIO_NUM_11;
constexpr gpio_num_t USER_BOOT = GPIO_NUM_0;

constexpr adc_unit_t ADC_UNIT = ADC_UNIT_1;
constexpr adc_channel_t VBUS_ADC_CH = ADC_CHANNEL_7; // GPIO8 on ESP32-S3.
constexpr adc_channel_t MIC_ADC_CH = ADC_CHANNEL_8;  // GPIO9 on ESP32-S3.
constexpr adc_channel_t LDR_ADC_CH = ADC_CHANNEL_9;  // GPIO10 on ESP32-S3.
} // namespace Pinmap

