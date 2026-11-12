#pragma once
#include <Arduino.h>
#include <WebServer.h>
namespace edgev2fw {
void begin(WebServer& server);
void tick(bool wifiConnected);
void handle(WebServer& server, const String& deviceCode);
bool serial(const String& line);
}
