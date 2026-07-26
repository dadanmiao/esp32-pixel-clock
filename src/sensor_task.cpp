/*
 * Author: Yang
 * I2C environmental polling and ambient light adaptation on Core 0.
 */
#include "sensor_task.h"

#include <Arduino.h>
#include <Wire.h>

#include "adc_sample_bus.h"
#include "app_config.h"
#include "app_state.h"
#include "pinmap.h"

namespace {
constexpr uint8_t HTU21D_ADDR = 0x40;
constexpr uint8_t HTU21D_TEMP_NOHOLD = 0xF3;
constexpr uint8_t HTU21D_HUM_NOHOLD = 0xF5;

constexpr uint8_t MPU6500_ADDR = 0x68;
constexpr uint8_t MPU_PWR_MGMT_1 = 0x6B;
constexpr uint8_t MPU_CONFIG = 0x1A;
constexpr uint8_t MPU_ACCEL_CONFIG = 0x1C;
constexpr uint8_t MPU_GYRO_CONFIG = 0x1B;
constexpr uint8_t MPU_ACCEL_XOUT_H = 0x3B;

TaskHandle_t sensorTaskHandle = nullptr;
float ldrFiltered = AppConfig::DefaultBrightness;

bool writeReg(uint8_t addr, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readRegs(uint8_t addr, uint8_t reg, uint8_t *buf, size_t len) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  const size_t read = Wire.requestFrom(addr, len);
  if (read != len) {
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    buf[i] = Wire.read();
  }
  return true;
}

bool readHtu21d(uint8_t cmd, float &value, bool humidity) {
  Wire.beginTransmission(HTU21D_ADDR);
  Wire.write(cmd);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  vTaskDelay(pdMS_TO_TICKS(humidity ? 30 : 60));

  if (Wire.requestFrom(HTU21D_ADDR, static_cast<uint8_t>(3)) != 3) {
    return false;
  }

  const uint16_t raw = (static_cast<uint16_t>(Wire.read()) << 8) | Wire.read();
  Wire.read(); // CRC byte. Framework keeps this lightweight; add CRC if production requires it.
  const uint16_t cleanRaw = raw & 0xFFFC;

  if (humidity) {
    value = -6.0f + 125.0f * static_cast<float>(cleanRaw) / 65536.0f;
  } else {
    value = -46.85f + 175.72f * static_cast<float>(cleanRaw) / 65536.0f;
  }
  return true;
}

void initMpu6500() {
  writeReg(MPU6500_ADDR, MPU_PWR_MGMT_1, 0x00);
  vTaskDelay(pdMS_TO_TICKS(50));
  writeReg(MPU6500_ADDR, MPU_CONFIG, 0x03);
  writeReg(MPU6500_ADDR, MPU_ACCEL_CONFIG, 0x00); // +/-2g.
  writeReg(MPU6500_ADDR, MPU_GYRO_CONFIG, 0x00);  // +/-250 dps.
}

bool readMpu6500(EnvironmentState &env) {
  uint8_t buf[14] = {};
  if (!readRegs(MPU6500_ADDR, MPU_ACCEL_XOUT_H, buf, sizeof(buf))) {
    return false;
  }

  const auto i16 = [](uint8_t hi, uint8_t lo) {
    return static_cast<int16_t>((static_cast<uint16_t>(hi) << 8) | lo);
  };

  const int16_t ax = i16(buf[0], buf[1]);
  const int16_t ay = i16(buf[2], buf[3]);
  const int16_t az = i16(buf[4], buf[5]);
  const int16_t gx = i16(buf[8], buf[9]);
  const int16_t gy = i16(buf[10], buf[11]);
  const int16_t gz = i16(buf[12], buf[13]);

  env.accelX = static_cast<float>(ax) / 16384.0f;
  env.accelY = static_cast<float>(ay) / 16384.0f;
  env.accelZ = static_cast<float>(az) / 16384.0f;
  env.gyroX = static_cast<float>(gx) / 131.0f;
  env.gyroY = static_cast<float>(gy) / 131.0f;
  env.gyroZ = static_cast<float>(gz) / 131.0f;
  return true;
}

uint8_t mapBrightness(uint16_t rawLdr, const ControlState &control) {
  const uint16_t lo = control.lowLightThreshold < control.highLightThreshold ? control.lowLightThreshold : control.highLightThreshold;
  const uint16_t hi = control.lowLightThreshold > control.highLightThreshold ? control.lowLightThreshold : control.highLightThreshold;
  const uint16_t clamped = constrain(rawLdr, lo, hi);
  const uint16_t span = hi > lo ? hi - lo : 1;
  const float ratio = static_cast<float>(clamped - lo) / static_cast<float>(span);
  const float target = AppConfig::MaxAutoBrightness - ratio * (AppConfig::MaxAutoBrightness - AppConfig::MinAutoBrightness);
  ldrFiltered = ldrFiltered * 0.92f + target * 0.08f;
  return static_cast<uint8_t>(constrain(lroundf(ldrFiltered), AppConfig::MinAutoBrightness, AppConfig::MaxAutoBrightness));
}

void sensorTask(void *) {
  Wire.begin(static_cast<int>(Pinmap::I2C_SDA), static_cast<int>(Pinmap::I2C_SCL), 400000);
  pinMode(static_cast<uint8_t>(Pinmap::MPU_INT), INPUT_PULLUP);
  initMpu6500();

  uint32_t slowCounter = 0;
  while (true) {
    RenderState state = copySharedState();
    EnvironmentState env = state.environment;

    uint16_t rawLdr = env.rawLdr;
    if (!readAdcRaw(Pinmap::LDR_ADC_CH, rawLdr)) {
      rawLdr = analogRead(static_cast<uint8_t>(Pinmap::LDR_ADC));
    }
    env.rawLdr = rawLdr;
    env.adaptiveBrightness = mapBrightness(env.rawLdr, state.control);

    if (readMpu6500(env)) {
      env.mpuOnline = true;
      env.mpuReadCount++;
    } else {
      env.mpuOnline = false;
      env.mpuFailCount++;
    }

    if ((slowCounter++ % 20) == 0) {
      float value = 0.0f;
      if (readHtu21d(HTU21D_TEMP_NOHOLD, value, false)) {
        env.temperatureC = value;
      }
      if (readHtu21d(HTU21D_HUM_NOHOLD, value, true)) {
        env.humidityRh = value;
      }
    }

    time_t unixTime = 0;
    time(&unixTime);
    updateEnvironmentState(env, unixTime);
    pushRenderSnapshot(0);
    vTaskDelay(pdMS_TO_TICKS(AppConfig::SensorPollMs));
  }
}
} // namespace

void startSensorTask() {
  xTaskCreatePinnedToCore(sensorTask, "i2c_env_core0", 8192, nullptr, 3, &sensorTaskHandle, 0);
}
