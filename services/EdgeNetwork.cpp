#include "EdgeNetwork.h"

#include "../config/Settings.h"
#include "../drivers/DeviceController.h"
#include "../drivers/FloatController.h"

#include <ArduinoJson.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

namespace
{
Preferences preferences;
DNSServer dnsServer;
WebServer server(PROVISION_HTTP_PORT);
EdgeNetwork *instance = nullptr;
}

void EdgeNetwork::begin()
{
    instance = this;
    preferences.begin("edge", false);
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    kioskUrl = preferences.getString("kioskUrl", DEFAULT_KIOSK_URL);

    server.on("/", HTTP_GET, []() { instance->handleProvisionPage(); });
    server.on("/generate_204", HTTP_GET, []() { instance->handleProvisionPage(); });
    server.on("/hotspot-detect.html", HTTP_GET,
              []() { instance->handleProvisionPage(); });
    server.on("/connecttest.txt", HTTP_GET,
              []() { instance->handleProvisionPage(); });
    server.on("/ncsi.txt", HTTP_GET,
              []() { instance->handleProvisionPage(); });
    server.on("/success.txt", HTTP_GET,
              []() { instance->handleProvisionPage(); });
    server.on("/fwlink", HTTP_GET,
              []() { instance->handleProvisionPage(); });
    server.on("/api/info", HTTP_GET, []() { instance->handleInfo(); });
    server.on("/api/provision", HTTP_OPTIONS, []() { server.send(204); });
    server.on("/api/provision", HTTP_POST, []() { instance->handleProvision(); });
    server.on("/configure", HTTP_POST,
              []() { instance->handleProvisionForm(); });
    server.on("/api/command", HTTP_POST, []() { instance->handleCommand(); });
    server.on("/api/status", HTTP_GET, []() { instance->handleInfo(); });

    connectWifi();
    server.begin();
}

void EdgeNetwork::loop()
{
    if (provisioningMode)
        dnsServer.processNextRequest();
    server.handleClient();

    if (restartPending && millis() >= restartAt)
    {
        ESP.restart();
    }

    if (!WiFi.isConnected())
        return;

    if (millis() - lastTelemetryAt < TELEMETRY_INTERVAL_MS)
        return;

    lastTelemetryAt = millis();
    publishTelemetry();
}

void EdgeNetwork::connectWifi()
{
    if (ssid.isEmpty())
    {
        startProvisioningAp();
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting WiFi");
    const unsigned long startedAt = millis();
    while (!WiFi.isConnected()
           && millis() - startedAt < WIFI_CONNECT_TIMEOUT_MS)
    {
        delay(500);
        Serial.print(".");
    }

    if (!WiFi.isConnected())
    {
        Serial.println("\nWiFi connection failed");
        startProvisioningAp();
        return;
    }

    Serial.println("\nWiFi connected");
    Serial.print("ESP IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Kiosk URL: ");
    Serial.println(kioskUrl);
}

void EdgeNetwork::startProvisioningAp()
{
    provisioningMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName().c_str());
    dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.print("Provisioning AP: ");
    Serial.println(apName());
    Serial.println("POST /api/provision with ssid, password, kioskUrl");
}

void EdgeNetwork::handleProvisionPage()
{
    String html = R"rawliteral(
<!doctype html>
<html lang="vi">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CrabSense C115 - Cau hinh Wi-Fi</title>
<style>
body{font-family:Arial,sans-serif;background:#eef8f5;color:#173b4d;margin:0;padding:24px}
main{max-width:460px;margin:auto;background:white;border-radius:18px;padding:24px;
box-shadow:0 8px 30px #174b5626}h1{margin-top:0;color:#087f68}
label{display:block;margin-top:16px;font-weight:bold}input{box-sizing:border-box;
width:100%;padding:12px;margin-top:6px;border:1px solid #b7d9d0;border-radius:9px;
font-size:16px}button{width:100%;padding:13px;margin-top:22px;border:0;border-radius:9px;
background:#07866d;color:white;font-size:16px;font-weight:bold}small{color:#607d85}
</style>
<main>
<h1>CrabSense-C115</h1>
<p>Cấu hình Wi-Fi cho bộ điều khiển ESP32.</p>
<form method="post" action="/configure">
<label>Wi-Fi trại</label>
<input name="ssid" required placeholder="Tên Wi-Fi">
<label>Mật khẩu Wi-Fi</label>
<input id="password" name="password" type="password" placeholder="Mật khẩu">
<label style="font-weight:normal"><input id="showPassword" type="checkbox"
onchange="password.type=this.checked?'text':'password'"> Hiện mật khẩu</label>
<label>Kiosk URL</label>
<input name="kioskUrl" required value="http://192.168.1.95:8090">
<small>Ví dụ: http://192.168.1.95:8090</small>
<button type="submit">Lưu và kết nối</button>
</form>
</main>
</html>
)rawliteral";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html; charset=utf-8", html);
}

void EdgeNetwork::handleProvisionForm()
{
    const String newSsid = server.arg("ssid");
    const String newPassword = server.arg("password");
    const String newKioskUrl = server.arg("kioskUrl");
    if (newSsid.isEmpty() || newKioskUrl.isEmpty())
    {
        server.send(400, "text/plain; charset=utf-8",
                    "SSID và Kiosk URL là bắt buộc");
        return;
    }

    preferences.putString("ssid", newSsid);
    preferences.putString("password", newPassword);
    preferences.putString("kioskUrl", newKioskUrl);
    server.send(200, "text/html; charset=utf-8",
                "<h2>Đã lưu cấu hình</h2><p>ESP đang khởi động lại. "
                "Bạn có thể đóng trang này.</p>");
    restartPending = true;
    restartAt = millis() + 1000;
}

void EdgeNetwork::handleInfo()
{
    JsonDocument doc;
    doc["deviceCode"] = deviceCode();
    doc["mac"] = macAddress();
    doc["ipAddress"] = WiFi.localIP().toString();
    doc["kioskUrl"] = kioskUrl;
    doc["connected"] = WiFi.isConnected();
    doc["provisioning"] = provisioningMode;

    String body;
    serializeJson(doc, body);
    server.send(200, "application/json", body);
}

void EdgeNetwork::handleProvision()
{
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain")))
    {
        server.send(400, "application/json",
                    "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }

    const String newSsid = String(doc["ssid"] | "");
    const String newPassword = String(doc["password"] | "");
    const String newKioskUrl = String(doc["kioskUrl"] | kioskUrl);
    if (newSsid.isEmpty() || newKioskUrl.isEmpty())
    {
        server.send(400, "application/json",
                    "{\"success\":false,\"message\":\"ssid and kioskUrl are required\"}");
        return;
    }

    preferences.putString("ssid", newSsid);
    preferences.putString("password", newPassword);
    preferences.putString("kioskUrl", newKioskUrl);

    server.send(200, "application/json",
                "{\"success\":true,\"message\":\"Saved; restarting\"}");
    restartPending = true;
    restartAt = millis() + 1000;
}

void EdgeNetwork::handleCommand()
{
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain")))
    {
        server.send(400, "application/json",
                    "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
    }

    String command = String(doc["command"] | "");
    command.toLowerCase();
    const String channelText = String(doc["channel"] | "");
    const int channel = channelText.toInt();
    bool success = true;

    if (command == "alloff")
    {
        allOutputsOff();
    }
    else if (command == "on" || command == "off" || command == "toggle")
    {
        if (channel < 1 || channel > 4)
            success = false;
        else
        {
            const bool enabled = command == "on"
                ? true
                : command == "off" ? false : !isOutputEnabled(channel);
            success = setOutput(channel, enabled);
        }
    }
    else
    {
        success = false;
    }

    JsonDocument response;
    response["success"] = success;
    response["command"] = command;
    response["channel"] = channelText;
    response["message"] = success ? "Applied" : "Invalid command or channel";
    String body;
    serializeJson(response, body);
    server.send(success ? 200 : 400, "application/json", body);
}

void EdgeNetwork::publishTelemetry()
{
    String base = kioskUrl;
    base.trim();
    while (base.endsWith("/"))
        base.remove(base.length() - 1);

    if (base.isEmpty())
        return;

    JsonDocument doc;
    doc["deviceCode"] = deviceCode();
    doc["mac"] = macAddress();
    doc["ipAddress"] = WiFi.localIP().toString();
    JsonArray readings = doc["readings"].to<JsonArray>();
    for (uint8_t i = 0; i < getFloatCount(); ++i)
    {
        JsonObject reading = readings.add<JsonObject>();
        reading["pin"] = getFloatPin(i);
        reading["val"] = getFloatState(i) ? 1 : 0;
        reading["sensor"] = getFloatSensorCode(i);
    }

    String payload;
    serializeJson(doc, payload);
    HTTPClient http;
    if (!http.begin(base + "/api/telemetry"))
        return;
    http.addHeader("Content-Type", "application/json");
    const int code = http.POST(payload);
    Serial.print("Kiosk telemetry ");
    Serial.println(code);
    http.end();
}

String EdgeNetwork::deviceCode() const
{
    const uint64_t chipId = ESP.getEfuseMac();
    char value[24];
    snprintf(value, sizeof(value), "ESP32-%04X", static_cast<uint16_t>(chipId));
    return String(value);
}

String EdgeNetwork::apName() const
{
    return "CrabSense-C115";
}

String EdgeNetwork::macAddress() const
{
    return WiFi.macAddress();
}
