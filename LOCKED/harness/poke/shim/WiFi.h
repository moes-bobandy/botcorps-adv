/* cyd-sim WiFi shim. The cyd env builds with BOTCORPS_LIVE_SCAN=0, so only probe.cpp's WiFi.mode(WIFI_OFF) reaches here. */
#pragma once
#include "Arduino.h"
enum { WIFI_OFF = 0, WIFI_STA = 1 };
struct SimWiFi { void mode(int) {} void disconnect(bool = false) {} int scanNetworks(bool = false, bool = false) { return 0; } void scanDelete(void) {} };
extern SimWiFi WiFi;
