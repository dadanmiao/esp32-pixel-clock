/*
 * Author: Yang
 * Non-blocking Wi-Fi provisioning for Pixel Clock.
 */
#pragma once

#include <Arduino.h>

bool startWiFiManager();
void serviceWiFiManager();
bool saveWiFiCredentialsAndConnect(const char *ssid, const char *password);
void resetWiFiSettingsAndRestart();
String getWiFiStatusJson();
String scanWiFiNetworksJson();
