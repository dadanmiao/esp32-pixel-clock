/*
 * Author: Yang
 * ADC continuous DMA audio sampling and FFT on Core 0.
 */
#include "audio_task.h"

#include <Arduino.h>
#include <arduinoFFT.h>
#include <driver/adc.h>
#include <esp_err.h>

#include "adc_sample_bus.h"
#include "app_config.h"
#include "app_state.h"
#include "pinmap.h"

namespace {
constexpr size_t FftSize = AppConfig::AudioFftSize;
constexpr size_t DmaFrameBytes = 512;

TaskHandle_t audioTaskHandle = nullptr;
float vbusRawFiltered = 0.0f;
float ldrRawFiltered = 0.0f;

double vReal[FftSize];
double vImag[FftSize];
ArduinoFFT<double> fft(vReal, vImag, FftSize, AppConfig::AudioSampleRate);
float micBias = 2048.0f;
uint16_t lastMicRaw = 0;
uint16_t micMinRaw = 4095;
uint16_t micMaxRaw = 0;
uint32_t dmaReadCount = 0;
uint32_t dmaTimeoutCount = 0;
uint32_t micSampleCount = 0;
uint32_t vbusSampleCount = 0;
uint32_t ldrSampleCount = 0;
uint32_t lastMicSampleAtMs = 0;
bool usingAnalogFallback = false;

bool initAdcDma() {
  adc_digi_pattern_config_t pattern[3] = {};
  pattern[0].atten = ADC_ATTEN_DB_12;
  pattern[0].channel = Pinmap::MIC_ADC_CH;
  pattern[0].unit = Pinmap::ADC_UNIT;
  pattern[0].bit_width = ADC_WIDTH_BIT_12;
  pattern[1].atten = ADC_ATTEN_DB_12;
  pattern[1].channel = Pinmap::VBUS_ADC_CH;
  pattern[1].unit = Pinmap::ADC_UNIT;
  pattern[1].bit_width = ADC_WIDTH_BIT_12;
  pattern[2].atten = ADC_ATTEN_DB_12;
  pattern[2].channel = Pinmap::LDR_ADC_CH;
  pattern[2].unit = Pinmap::ADC_UNIT;
  pattern[2].bit_width = ADC_WIDTH_BIT_12;

  adc_digi_init_config_t initCfg = {};
  initCfg.max_store_buf_size = 2048;
  initCfg.conv_num_each_intr = DmaFrameBytes;
  initCfg.adc1_chan_mask = (1UL << Pinmap::MIC_ADC_CH) | (1UL << Pinmap::VBUS_ADC_CH) | (1UL << Pinmap::LDR_ADC_CH);
  initCfg.adc2_chan_mask = 0;

  esp_err_t err = adc_digi_initialize(&initCfg);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    Serial.printf("[audio] adc_digi_initialize failed: %s\n", esp_err_to_name(err));
    return false;
  }

  adc_digi_configuration_t adcCfg = {};
  adcCfg.conv_limit_en = false;
  adcCfg.conv_limit_num = 0;
  adcCfg.pattern_num = 3;
  adcCfg.adc_pattern = pattern;
  adcCfg.sample_freq_hz = AppConfig::AudioSampleRate * 3;
  adcCfg.conv_mode = ADC_CONV_SINGLE_UNIT_1;
  adcCfg.format = ADC_DIGI_OUTPUT_FORMAT_TYPE2;

  err = adc_digi_controller_configure(&adcCfg);
  if (err != ESP_OK) {
    Serial.printf("[audio] adc_digi_controller_configure failed: %s\n", esp_err_to_name(err));
    return false;
  }

  err = adc_digi_start();
  if (err != ESP_OK) {
    Serial.printf("[audio] adc_digi_start failed: %s\n", esp_err_to_name(err));
    return false;
  }

  return true;
}

void publishAudioState() {
  RenderState state = copySharedState();
  const ControlState control = state.control;
  AudioState audio = state.audio;
  double sumSquares = 0.0;
  double peak = 0.0;

  for (size_t i = 0; i < FftSize; ++i) {
    const double centered = vReal[i];
    const double mag = fabs(centered);
    sumSquares += centered * centered;
    if (mag > peak) {
      peak = mag;
    }
    vImag[i] = 0.0;
  }

  audio.rms = static_cast<float>(sqrt(sumSquares / FftSize) / 2048.0);
  audio.peak = static_cast<float>(peak / 2048.0);
  if (audio.noiseFloor <= 0.0005f) {
    audio.noiseFloor = max(audio.rms, 0.002f);
  } else {
    const float noiseAlpha = audio.rms < audio.noiseFloor * 1.8f ? 0.992f : 0.9995f;
    audio.noiseFloor = audio.noiseFloor * noiseAlpha + audio.rms * (1.0f - noiseAlpha);
  }
  audio.noiseFloor = constrain(audio.noiseFloor, 0.0015f, 0.18f);

  const float signalRms = max(0.0f, audio.rms - audio.noiseFloor * 1.12f);
  audio.signalPresent = signalRms > max(0.0045f, audio.noiseFloor * 0.32f);
  float targetGain = 1.0f;
  if (control.audioAutoGain && audio.signalPresent) {
    targetGain = constrain(0.115f / max(signalRms, 0.004f), 0.65f, 8.0f);
  }
  const float gainAlpha = targetGain > audio.autoGain ? 0.965f : 0.92f;
  audio.autoGain = audio.autoGain <= 0.01f
                       ? 1.0f
                       : audio.autoGain * gainAlpha + targetGain * (1.0f - gainAlpha);
  if (!control.audioAutoGain) {
    audio.autoGain = 1.0f;
  }
  audio.micRaw = lastMicRaw;
  audio.micMin = micMinRaw;
  audio.micMax = micMaxRaw;
  audio.micBias = micBias;
  audio.dmaReadCount = dmaReadCount;
  audio.dmaTimeoutCount = dmaTimeoutCount;
  audio.micSampleCount = micSampleCount;
  audio.vbusSampleCount = vbusSampleCount;
  audio.ldrSampleCount = ldrSampleCount;
  audio.usingAnalogFallback = usingAnalogFallback;

  fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  fft.compute(FFTDirection::Forward);
  fft.complexToMagnitude();

  const size_t usableBins = FftSize / 2;
  const float releaseSmoothing = constrain(control.audioSmoothing / 255.0f, 0.08f, 0.94f);
  const float attackSmoothing = releaseSmoothing * 0.38f;
  const float gate = audio.signalPresent
                         ? 1.0f
                         : constrain(signalRms / 0.0045f, 0.0f, 1.0f);
  float low = 0.0f;
  float mid = 0.0f;
  float high = 0.0f;

  for (size_t band = 0; band < AppConfig::SpectrumBins; ++band) {
    const float startRatio = static_cast<float>(band) / AppConfig::SpectrumBins;
    const float endRatio = static_cast<float>(band + 1) / AppConfig::SpectrumBins;
    size_t start = 1 + static_cast<size_t>(
                           lroundf((usableBins - 2) * powf(startRatio, 1.72f)));
    size_t end = 1 + static_cast<size_t>(
                         lroundf((usableBins - 2) * powf(endRatio, 1.72f)));
    start = min(start, usableBins - 1);
    end = min(max(end, start + 1), usableBins);

    double acc = 0.0;
    for (size_t i = start; i < end; ++i) {
      acc += vReal[i];
    }
    const size_t binCount = max(static_cast<size_t>(1), end - start);
    const float magnitude = static_cast<float>(acc / binCount);
    const float linear = magnitude * audio.autoGain / 6200.0f;
    const float compressed = log1pf(max(0.0f, linear) * 5.0f) / log1pf(5.0f);
    const float raw = constrain(compressed * gate, 0.0f, 1.0f);
    audio.spectrum[band] = raw;
    const float smoothing = raw > audio.smoothSpectrum[band]
                                ? attackSmoothing
                                : releaseSmoothing;
    audio.smoothSpectrum[band] =
        audio.smoothSpectrum[band] * smoothing + raw * (1.0f - smoothing);

    const float value = audio.smoothSpectrum[band];
    if (band < 6) {
      low += value;
    } else if (band < 18) {
      mid += value;
    } else {
      high += value;
    }
  }

  audio.lowEnergy = low / 6.0f;
  audio.midEnergy = mid / 12.0f;
  audio.highEnergy = high / (AppConfig::SpectrumBins > 18 ? static_cast<float>(AppConfig::SpectrumBins - 18) : 1.0f);
  audio.energy = (low + mid + high) / static_cast<float>(AppConfig::SpectrumBins);

  const float previousAvg = audio.energyAvg;
  audio.energyAvg = previousAvg <= 0.001f ? audio.lowEnergy : previousAvg * 0.95f + audio.lowEnergy * 0.05f;
  const uint32_t now = millis();
  audio.beat = false;
  if (audio.signalPresent &&
      audio.energyAvg > 0.012f &&
      audio.lowEnergy > audio.energyAvg * 1.55f &&
      audio.lowEnergy > 0.02f &&
      now - audio.lastBeatMs > 180) {
    audio.beat = true;
    audio.lastBeatMs = now;
  }

  audio.frameCounter = state.audio.frameCounter + 1;
  updateAudioState(audio);
  pushRenderSnapshot(0);
}

void ingestMicRaw(uint16_t raw, size_t &sampleIndex) {
  publishAdcRaw(Pinmap::MIC_ADC_CH, raw);
  lastMicRaw = raw;
  if (raw < micMinRaw) {
    micMinRaw = raw;
  }
  if (raw > micMaxRaw) {
    micMaxRaw = raw;
  }
  micSampleCount++;
  lastMicSampleAtMs = millis();

  micBias = micBias * 0.995f + static_cast<float>(raw) * 0.005f;
  vReal[sampleIndex++] = static_cast<double>(static_cast<float>(raw) - micBias);

  if (sampleIndex >= FftSize) {
    publishAudioState();
    sampleIndex = 0;
    micMinRaw = 4095;
    micMaxRaw = 0;
  }
}

void stopDmaAndUseAnalogFallback() {
  if (usingAnalogFallback) {
    return;
  }
  adc_digi_stop();
  adc_digi_deinitialize();
  usingAnalogFallback = true;
  analogReadResolution(12);
  analogSetPinAttenuation(static_cast<uint8_t>(Pinmap::MIC_ADC), ADC_11db);
  analogSetPinAttenuation(static_cast<uint8_t>(Pinmap::VBUS_ADC), ADC_11db);
  analogSetPinAttenuation(static_cast<uint8_t>(Pinmap::LDR_ADC), ADC_11db);
  Serial.println("[audio] no MIC samples from ADC DMA, switched to analogRead fallback");
}

void initAnalogAudioInput() {
  usingAnalogFallback = true;
  analogReadResolution(12);
  analogSetPinAttenuation(static_cast<uint8_t>(Pinmap::MIC_ADC), ADC_11db);
  analogSetPinAttenuation(static_cast<uint8_t>(Pinmap::VBUS_ADC), ADC_11db);
  analogSetPinAttenuation(static_cast<uint8_t>(Pinmap::LDR_ADC), ADC_11db);
  Serial.println("[audio] using analogRead(GPIO9) audio input");
}

void readSlowAnalogHousekeeping() {
  const uint16_t rawVbus = analogRead(static_cast<uint8_t>(Pinmap::VBUS_ADC));
  vbusRawFiltered = vbusRawFiltered <= 0.1f ? rawVbus : vbusRawFiltered * 0.98f + rawVbus * 0.02f;
  publishAdcRaw(Pinmap::VBUS_ADC_CH, static_cast<uint16_t>(lroundf(vbusRawFiltered)));
  vbusSampleCount++;

  const uint16_t rawLdr = analogRead(static_cast<uint8_t>(Pinmap::LDR_ADC));
  ldrRawFiltered = ldrRawFiltered <= 0.1f ? rawLdr : ldrRawFiltered * 0.98f + rawLdr * 0.02f;
  publishAdcRaw(Pinmap::LDR_ADC_CH, static_cast<uint16_t>(lroundf(ldrRawFiltered)));
  ldrSampleCount++;
}

void runAnalogAudioLoop(size_t &sampleIndex) {
  uint32_t housekeepingDivider = 0;
  while (true) {
    for (uint8_t i = 0; i < 64; ++i) {
      ingestMicRaw(static_cast<uint16_t>(analogRead(static_cast<uint8_t>(Pinmap::MIC_ADC))), sampleIndex);

      if ((housekeepingDivider++ & 0x3F) == 0) {
        readSlowAnalogHousekeeping();
      }

      delayMicroseconds(1000000UL / AppConfig::AudioSampleRate);
    }

    vTaskDelay(1);
  }
}

void audioTask(void *) {
  size_t sampleIndex = 0;

  if (!AppConfig::AudioUseAdcDma) {
    initAnalogAudioInput();
    runAnalogAudioLoop(sampleIndex);
  }

  if (!initAdcDma()) {
    initAnalogAudioInput();
    runAnalogAudioLoop(sampleIndex);
  }

  uint8_t dmaBuffer[DmaFrameBytes] = {};
  lastMicSampleAtMs = millis();

  while (true) {
    if (usingAnalogFallback) {
      runAnalogAudioLoop(sampleIndex);
    }

    uint32_t bytesRead = 0;
    esp_err_t err = adc_digi_read_bytes(dmaBuffer, sizeof(dmaBuffer), &bytesRead, 100);
    if (err == ESP_ERR_TIMEOUT) {
      dmaTimeoutCount++;
      if (millis() - lastMicSampleAtMs > 1500) {
        stopDmaAndUseAnalogFallback();
      }
      continue;
    }
    if (err != ESP_OK) {
      Serial.printf("[audio] adc read failed: %s\n", esp_err_to_name(err));
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    dmaReadCount++;

    for (uint32_t offset = 0; offset + sizeof(adc_digi_output_data_t) <= bytesRead; offset += sizeof(adc_digi_output_data_t)) {
      auto *sample = reinterpret_cast<adc_digi_output_data_t *>(&dmaBuffer[offset]);
      const int raw = static_cast<int>(sample->type2.data);
      const adc_channel_t channel = static_cast<adc_channel_t>(sample->type2.channel);

      if (channel == Pinmap::VBUS_ADC_CH) {
        vbusRawFiltered = vbusRawFiltered <= 0.1f ? raw : vbusRawFiltered * 0.98f + raw * 0.02f;
        publishAdcRaw(channel, static_cast<uint16_t>(lroundf(vbusRawFiltered)));
        vbusSampleCount++;
        continue;
      }
      if (channel == Pinmap::LDR_ADC_CH) {
        ldrRawFiltered = ldrRawFiltered <= 0.1f ? raw : ldrRawFiltered * 0.98f + raw * 0.02f;
        publishAdcRaw(channel, static_cast<uint16_t>(lroundf(ldrRawFiltered)));
        ldrSampleCount++;
        continue;
      }
      if (channel != Pinmap::MIC_ADC_CH) {
        continue;
      }

      ingestMicRaw(static_cast<uint16_t>(raw), sampleIndex);
    }

    if (millis() - lastMicSampleAtMs > 1500) {
      stopDmaAndUseAnalogFallback();
    }
  }
}
} // namespace

void startAudioTask() {
  xTaskCreatePinnedToCore(audioTask, "audio_fft_core0", 10240, nullptr, 1, &audioTaskHandle, 0);
}
