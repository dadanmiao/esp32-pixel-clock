/*
 * Author: Yang
 * Non-blocking Wi-Fi provisioning that keeps the Pixel Clock console alive.
 */
#include "wifi_manager_app.h"

#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>

#include "app_config.h"

namespace {
constexpr const char *SetupApName = "PixelClock-Setup";
constexpr const char *SetupApPassword = "pixelclock";
constexpr const char *WifiPrefsNamespace = "wifi";
constexpr uint32_t ReconnectIntervalMs = 30000;

String savedSsid;
String savedPassword;
bool hasSavedCredentials = false;
bool setupApActive = false;
bool timeConfigured = false;
bool lastConnected = false;
uint32_t lastReconnectAttemptMs = 0;

bool apEnabled() {
  const wifi_mode_t mode = WiFi.getMode();
  return mode == WIFI_AP || mode == WIFI_AP_STA;
}

String authModeName(wifi_auth_mode_t mode) {
  switch (mode) {
    case WIFI_AUTH_OPEN:
      return "open";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2-Enterprise";
    case WIFI_AUTH_WPA3_PSK:
      return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2/WPA3";
    default:
      return "unknown";
  }
}

void loadSavedCredentials() {
  Preferences prefs;
  if (!prefs.begin(WifiPrefsNamespace, true)) {
    hasSavedCredentials = false;
    return;
  }
  savedSsid = prefs.getString("ssid", "");
  savedPassword = prefs.getString("pass", "");
  prefs.end();
  hasSavedCredentials = savedSsid.length() > 0;
}

void startSetupAp() {
  if (setupApActive && apEnabled()) {
    return;
  }

  const bool stationActive = WiFi.getMode() == WIFI_STA || WiFi.getMode() == WIFI_AP_STA;
  WiFi.mode(stationActive ? WIFI_AP_STA : WIFI_AP);
  setupApActive = WiFi.softAP(SetupApName, SetupApPassword);
  if (setupApActive) {
    Serial.printf("[wifi] setup AP: %s / %s\n", SetupApName, WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("[wifi] failed to start setup AP");
  }
}

bool beginStationConnect() {
  if (!hasSavedCredentials || savedSsid.length() == 0) {
    return false;
  }

  WiFi.mode(setupApActive ? WIFI_AP_STA : WIFI_STA);
  WiFi.setHostname(AppConfig::Hostname);
  WiFi.begin(savedSsid.c_str(), savedPassword.c_str());
  lastReconnectAttemptMs = millis();
  timeConfigured = false;
  Serial.printf("[wifi] connecting to %s\n", savedSsid.c_str());
  return true;
}

String activeIpAddress() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  if (apEnabled()) {
    return WiFi.softAPIP().toString();
  }
  return "0.0.0.0";
}

String activeModeName() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  const bool ap = apEnabled();
  if (connected && ap) {
    return "AP+STA";
  }
  if (connected) {
    return "STA";
  }
  if (ap) {
    return "AP";
  }
  return "offline";
}

void configureTimeIfConnected() {
  if (timeConfigured || WiFi.status() != WL_CONNECTED) {
    return;
  }
  configTime(AppConfig::GmtOffsetSec, AppConfig::DaylightOffsetSec, AppConfig::NtpServer1, AppConfig::NtpServer2);
  timeConfigured = true;
  Serial.println("[time] NTP configured");
}
} // namespace

bool startWiFiManager() {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  WiFi.setHostname(AppConfig::Hostname);
  loadSavedCredentials();

  if (!hasSavedCredentials) {
    Serial.println("[wifi] no saved credentials, starting setup AP");
    startSetupAp();
    return false;
  }

  startSetupAp();
  beginStationConnect();
  return WiFi.status() == WL_CONNECTED;
}

void serviceWiFiManager() {
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected != lastConnected) {
    lastConnected = connected;
    if (connected) {
      Serial.printf("[wifi] connected: %s (%s)\n", WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());
    } else {
      Serial.println("[wifi] station disconnected");
    }
  }

  if (connected) {
    configureTimeIfConnected();
    return;
  }

  if (!setupApActive || !apEnabled()) {
    startSetupAp();
  }

  if (hasSavedCredentials && millis() - lastReconnectAttemptMs > ReconnectIntervalMs) {
    beginStationConnect();
  }
}

bool saveWiFiCredentialsAndConnect(const char *ssid, const char *password) {
  if (!ssid || strlen(ssid) == 0) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(WifiPrefsNamespace, false)) {
    return false;
  }
  prefs.putString("ssid", ssid);
  prefs.putString("pass", password ? password : "");
  prefs.end();

  savedSsid = ssid;
  savedPassword = password ? password : "";
  hasSavedCredentials = true;
  if (!setupApActive || !apEnabled()) {
    startSetupAp();
  }
  return beginStationConnect();
}

void resetWiFiSettingsAndRestart() {
  Preferences prefs;
  if (prefs.begin(WifiPrefsNamespace, false)) {
    prefs.clear();
    prefs.end();
  }
  WiFi.disconnect(true, true);
  delay(500);
  ESP.restart();
}

String getWiFiStatusJson() {
  JsonDocument doc;
  const bool connected = WiFi.status() == WL_CONNECTED;
  doc["connected"] = connected;
  doc["mode"] = activeModeName();
  doc["ssid"] = connected ? WiFi.SSID() : "";
  doc["ip"] = activeIpAddress();
  doc["rssi"] = connected ? WiFi.RSSI() : 0;
  doc["hostname"] = AppConfig::Hostname;
  doc["hasSavedCredentials"] = hasSavedCredentials;
  doc["setupApActive"] = apEnabled();
  doc["setupApSsid"] = SetupApName;
  doc["setupApIp"] = apEnabled() ? WiFi.softAPIP().toString() : "";

  String body;
  serializeJson(doc, body);
  return body;
}

String scanWiFiNetworksJson() {
  if (WiFi.getMode() == WIFI_AP) {
    WiFi.mode(WIFI_AP_STA);
    setupApActive = true;
  } else if (!apEnabled() && WiFi.status() != WL_CONNECTED) {
    startSetupAp();
  }

  WiFi.scanDelete();
  const int count = WiFi.scanNetworks(false, true);
  JsonDocument doc;
  doc["count"] = count > 0 ? count : 0;
  JsonArray networks = doc["networks"].to<JsonArray>();
  if (count > 0) {
    for (int i = 0; i < count; ++i) {
      JsonObject item = networks.add<JsonObject>();
      const wifi_auth_mode_t auth = WiFi.encryptionType(i);
      item["ssid"] = WiFi.SSID(i);
      item["rssi"] = WiFi.RSSI(i);
      item["channel"] = WiFi.channel(i);
      item["secure"] = auth != WIFI_AUTH_OPEN;
      item["auth"] = authModeName(auth);
    }
  }
  WiFi.scanDelete();

  String body;
  serializeJson(doc, body);
  return body;
}
