/*
 * Author: Yang
 * Open-Meteo weather fetcher pinned to Core 0.
 */
#include "weather_task.h"

#include <cmath>
#include <cstring>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "app_state.h"

namespace {
TaskHandle_t weatherTaskHandle = nullptr;
volatile bool forceRefresh = true;

constexpr uint32_t MinUpdateIntervalMs = 5UL * 60UL * 1000UL;
constexpr uint32_t RetryIntervalMs = 60UL * 1000UL;
constexpr uint32_t WeatherLoopDelayMs = 2000;

void copyWeatherCity(char *dest, const char *src) {
  if (!src) {
    src = "";
  }
  size_t out = 0;
  while (src[0] != '\0' && out < WeatherCityMaxLen - 1) {
    const char c = *src++;
    dest[out++] = (c >= 0x20 && c <= 0x7E) ? c : ' ';
  }
  dest[out] = '\0';
}

void setLastError(WeatherState &weather, const char *message) {
  if (!message) {
    message = "";
  }
  strncpy(weather.lastError, message, sizeof(weather.lastError) - 1);
  weather.lastError[sizeof(weather.lastError) - 1] = '\0';
}

const char *weatherCodeToText(int code) {
  switch (code) {
    case 0:
      return "Clear";
    case 1:
    case 2:
    case 3:
      return "Cloudy";
    case 45:
    case 48:
      return "Fog";
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
      return "Drizzle";
    case 61:
    case 63:
    case 65:
    case 66:
    case 67:
      return "Rain";
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:
      return "Snow";
    case 80:
    case 81:
    case 82:
      return "Showers";
    case 95:
    case 96:
    case 99:
      return "Thunder";
    default:
      return "Unknown";
  }
}

String buildWeatherUrl(float lat, float lon) {
  String url = "https://api.open-meteo.com/v1/forecast?";
  url += "latitude=" + String(lat, 6);
  url += "&longitude=" + String(lon, 6);
  url += "&current=temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,weather_code,cloud_cover,wind_speed_10m";
  url += "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max";
  url += "&forecast_days=1&timezone=auto";
  return url;
}

bool fetchWeatherOnce(const ControlState &control, WeatherState &weather) {
  weather.online = false;
  weather.latitude = control.weatherLatitude;
  weather.longitude = control.weatherLongitude;
  copyWeatherCity(weather.city, control.weatherCity);
  weather.lastUpdateMs = millis();

  if (WiFi.status() != WL_CONNECTED) {
    setLastError(weather, "WiFi not connected");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(8000);

  const String url = buildWeatherUrl(control.weatherLatitude, control.weatherLongitude);
  if (!http.begin(client, url)) {
    setLastError(weather, "HTTP begin failed");
    return false;
  }

  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    String err = "HTTP " + String(status);
    setLastError(weather, err.c_str());
    http.end();
    return false;
  }

  const String payload = http.getString();
  http.end();

  JsonDocument doc;
  const DeserializationError jsonErr = deserializeJson(doc, payload);
  if (jsonErr) {
    setLastError(weather, "JSON parse failed");
    return false;
  }

  JsonObject current = doc["current"];
  if (current.isNull()) {
    setLastError(weather, "missing current");
    return false;
  }

  weather.online = true;
  weather.hasData = true;
  weather.temperature = current["temperature_2m"] | 0.0f;
  weather.apparentTemperature = current["apparent_temperature"] | 0.0f;
  weather.relativeHumidity = current["relative_humidity_2m"] | 0;
  weather.weatherCode = current["weather_code"] | 0;
  weather.precipitation = current["precipitation"] | 0.0f;
  weather.cloudCover = current["cloud_cover"] | 0;
  weather.windSpeed = current["wind_speed_10m"] | 0.0f;

  JsonObject daily = doc["daily"];
  if (!daily.isNull()) {
    weather.todayTempMax = static_cast<int>(lroundf(daily["temperature_2m_max"][0] | weather.temperature));
    weather.todayTempMin = static_cast<int>(lroundf(daily["temperature_2m_min"][0] | weather.temperature));
    weather.todayPrecipProb = daily["precipitation_probability_max"][0] | 0;
  }

  weather.lastSuccessMs = millis();
  setLastError(weather, "");

  Serial.printf("[weather] %s %.1fC %s\n", weather.city, weather.temperature, weatherCodeToText(weather.weatherCode));
  return true;
}

void publishWeatherResult(const WeatherState &weather) {
  updateWeatherState(weather);
  pushRenderSnapshot(0);
}

void weatherTask(void *) {
  uint32_t lastAttemptMs = 0;

  while (true) {
    RenderState state = copySharedState();
    const ControlState control = state.control;

    if (!control.weatherEnabled) {
      state.weather.online = false;
      copyWeatherCity(state.weather.city, control.weatherCity);
      state.weather.latitude = control.weatherLatitude;
      state.weather.longitude = control.weatherLongitude;
      publishWeatherResult(state.weather);
      vTaskDelay(pdMS_TO_TICKS(WeatherLoopDelayMs));
      continue;
    }

    uint32_t intervalMs = static_cast<uint32_t>(control.weatherUpdateIntervalMin) * 60UL * 1000UL;
    if (intervalMs < MinUpdateIntervalMs) {
      intervalMs = MinUpdateIntervalMs;
    }

    const uint32_t now = millis();
    bool shouldFetch = false;
    if (forceRefresh) {
      forceRefresh = false;
      shouldFetch = true;
    } else if (!state.weather.hasData && now - lastAttemptMs >= RetryIntervalMs) {
      shouldFetch = true;
    } else if (state.weather.hasData && now - state.weather.lastSuccessMs >= intervalMs) {
      shouldFetch = true;
    }

    if (shouldFetch) {
      lastAttemptMs = now;
      WeatherState weather = state.weather;
      if (!fetchWeatherOnce(control, weather)) {
        weather.failCount++;
        weather.lastUpdateMs = millis();
      }
      publishWeatherResult(weather);
    }

    vTaskDelay(pdMS_TO_TICKS(WeatherLoopDelayMs));
  }
}
} // namespace

void startWeatherTask() {
  if (weatherTaskHandle) {
    return;
  }

  xTaskCreatePinnedToCore(
      weatherTask,
      "weather_core0",
      12288,
      nullptr,
      1,
      &weatherTaskHandle,
      0);
}

void requestWeatherRefresh() {
  forceRefresh = true;
}
