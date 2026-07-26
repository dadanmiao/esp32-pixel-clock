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
#include "competition_metrics.h"
#include "settings_storage.h"

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
    const uint8_t c = static_cast<uint8_t>(*src++);
    dest[out++] = c >= 0x20 ? static_cast<char>(c) : ' ';
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

String urlEncode(const char *text) {
  String encoded;
  if (!text) {
    return encoded;
  }

  constexpr char Hex[] = "0123456789ABCDEF";
  while (*text) {
    const uint8_t c = static_cast<uint8_t>(*text++);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += static_cast<char>(c);
    } else {
      encoded += '%';
      encoded += Hex[c >> 4];
      encoded += Hex[c & 0x0F];
    }
  }
  return encoded;
}

bool geocodeWeatherCity(const char *city, float &latitude, float &longitude, WeatherState &weather) {
  if (!city || city[0] == '\0') {
    setLastError(weather, "城市名称不能为空");
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(8000);

  const String url = "https://geocoding-api.open-meteo.com/v1/search?name=" +
                     urlEncode(city) + "&count=1&language=zh&format=json";
  if (!http.begin(client, url)) {
    setLastError(weather, "城市定位服务不可用");
    return false;
  }

  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    recordCompetitionExternalRequest();
    const String error = "城市定位 HTTP " + String(status);
    setLastError(weather, error.c_str());
    http.end();
    return false;
  }

  const String payload = http.getString();
  recordCompetitionExternalRequest(payload.length());
  JsonDocument doc;
  const DeserializationError jsonErr = deserializeJson(doc, payload);
  http.end();
  if (jsonErr) {
    setLastError(weather, "城市定位数据解析失败");
    return false;
  }

  JsonArray results = doc["results"].as<JsonArray>();
  if (results.isNull() || results.size() == 0) {
    setLastError(weather, "未找到该城市");
    return false;
  }

  JsonObject location = results[0];
  latitude = location["latitude"] | 0.0f;
  longitude = location["longitude"] | 0.0f;
  if (latitude == 0.0f && longitude == 0.0f) {
    setLastError(weather, "城市坐标无效");
    return false;
  }
  return true;
}

bool fetchWeatherOnce(ControlState &control, WeatherState &weather, bool &locationChanged) {
  weather.online = false;
  copyWeatherCity(weather.city, control.weatherCity);
  weather.lastUpdateMs = millis();
  locationChanged = false;

  if (WiFi.status() != WL_CONNECTED) {
    setLastError(weather, "Wi-Fi 未连接");
    return false;
  }

  float latitude = control.weatherLatitude;
  float longitude = control.weatherLongitude;
  if (control.weatherAutoLocate) {
    if (!geocodeWeatherCity(control.weatherCity, latitude, longitude, weather)) {
      return false;
    }
    locationChanged = fabsf(latitude - control.weatherLatitude) > 0.00001f ||
                      fabsf(longitude - control.weatherLongitude) > 0.00001f;
    control.weatherLatitude = latitude;
    control.weatherLongitude = longitude;
  }
  weather.latitude = latitude;
  weather.longitude = longitude;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(8000);

  const String url = buildWeatherUrl(latitude, longitude);
  if (!http.begin(client, url)) {
    setLastError(weather, "天气服务不可用");
    return false;
  }

  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    recordCompetitionExternalRequest();
    String err = "天气 HTTP " + String(status);
    setLastError(weather, err.c_str());
    http.end();
    return false;
  }

  const String payload = http.getString();
  recordCompetitionExternalRequest(payload.length());
  http.end();

  JsonDocument doc;
  const DeserializationError jsonErr = deserializeJson(doc, payload);
  if (jsonErr) {
    setLastError(weather, "天气数据解析失败");
    return false;
  }

  JsonObject current = doc["current"];
  if (current.isNull()) {
    setLastError(weather, "天气数据不完整");
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

  Serial.printf("[weather] %s %.1fC %s (%.4f, %.4f)\n", weather.city, weather.temperature,
                weatherCodeToText(weather.weatherCode), weather.latitude, weather.longitude);
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
    ControlState control = state.control;

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
      bool locationChanged = false;
      if (!fetchWeatherOnce(control, weather, locationChanged)) {
        weather.failCount++;
        weather.lastUpdateMs = millis();
      }
      if (locationChanged) {
        RenderState latest = copySharedState();
        if (latest.control.weatherAutoLocate &&
            strcmp(latest.control.weatherCity, control.weatherCity) == 0) {
          latest.control.weatherLatitude = control.weatherLatitude;
          latest.control.weatherLongitude = control.weatherLongitude;
          updateControlState(latest.control);
          requestSettingsSave();
        }
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

uint32_t getWeatherTaskStackWatermark() {
  return weatherTaskHandle ? static_cast<uint32_t>(uxTaskGetStackHighWaterMark(weatherTaskHandle)) : 0;
}

void requestWeatherRefresh() {
  forceRefresh = true;
}
