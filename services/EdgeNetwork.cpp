#include "EdgeNetwork.h"

#include "../config/Settings.h"
#include "../drivers/DeviceController.h"
#include "../drivers/FloatController.h"
#include "../drivers/PowerMeter.h"

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
    controllerSecret = preferences.getString("controllerSecret", "");

    server.on("/", HTTP_GET, []() { instance->handleProvisionPage(); });
    server.on("/generate_204", HTTP_GET, []() { instance->handleCaptiveRedirect(); });
    server.on("/hotspot-detect.html", HTTP_GET,
              []() { instance->handleCaptiveRedirect(); });
    server.on("/connecttest.txt", HTTP_GET,
              []() { instance->handleCaptiveRedirect(); });
    server.on("/ncsi.txt", HTTP_GET,
              []() { instance->handleCaptiveRedirect(); });
    server.on("/success.txt", HTTP_GET,
              []() { instance->handleCaptiveRedirect(); });
    server.on("/fwlink", HTTP_GET,
              []() { instance->handleCaptiveRedirect(); });
    server.on("/redirect", HTTP_GET,
              []() { instance->handleCaptiveRedirect(); });
    server.onNotFound([]() {
        if (instance->provisioningMode)
            instance->handleCaptiveRedirect();
        else
            server.send(404, "text/plain", "Not found");
    });
    server.on("/api/info", HTTP_GET, []() { instance->handleInfo(); });
    server.on("/api/provision", HTTP_OPTIONS, []() { server.send(204); });
    server.on("/api/provision", HTTP_POST, []() { instance->handleProvision(); });
    server.on("/configure", HTTP_POST,
              []() { instance->handleProvisionForm(); });
    server.on("/api/command", HTTP_POST, []() { instance->handleCommand(); });
    server.on("/api/restart", HTTP_POST, []() { instance->handleRestart(); });
    server.on("/api/reboot", HTTP_POST, []() { instance->handleRestart(); });
    server.on("/api/status", HTTP_GET, []() { instance->handleInfo(); });

    connectWifi();
    server.begin();
}

void EdgeNetwork::loop()
{
    if (provisioningMode)
    {
        for (uint8_t i = 0; i < 20; ++i)
            dnsServer.processNextRequest();
    }
    server.handleClient();

    if (restartPending && millis() >= restartAt)
    {
        ESP.restart();
    }

    if (!WiFi.isConnected())
        return;

    registerWithKiosk();

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
    WiFi.setSleep(false);
    const bool apUp = WiFi.softAP(apName().c_str(), nullptr, 11);
    dnsServer.setTTL(0);
    dnsServer.start(53, "*", WiFi.softAPIP());
    if (!apUp)
        Serial.println("Provisioning AP failed");
    Serial.print("Provisioning AP: ");
    Serial.println(apName());
    Serial.println("POST /api/provision with ssid, password, kioskUrl");
}

void EdgeNetwork::handleCaptiveRedirect()
{
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.send(302, "text/html",
                "<html><head><meta http-equiv=\"refresh\" "
                "content=\"0;url=http://192.168.4.1/\"></head></html>");
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
    if (newSsid.isEmpty())
    {
        server.send(400, "text/plain; charset=utf-8", "SSID là bắt buộc");
        return;
    }

    preferences.putString("ssid", newSsid);
    preferences.putString("password", newPassword);
    if (!newKioskUrl.isEmpty())
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
    doc["apName"] = apName();
    doc["controllerType"] = "ras_controller";
    doc["firmware"] = "1.0.0";
    doc["board"] = "ESP32 DevKit V1";
    doc["mac"] = macAddress();
    doc["ipAddress"] = WiFi.localIP().toString();
    doc["staIp"] = WiFi.localIP().toString();
    doc["wifiSsid"] = WiFi.SSID();
    doc["kioskUrl"] = kioskUrl;
    doc["connected"] = WiFi.isConnected();
    doc["provisioning"] = provisioningMode;
    doc["provisioned"] = !provisioningMode;
    doc["outputCount"] = getOutputCount();
    doc["sensorCount"] = getFloatCount();

    JsonArray outputs = doc["outputs"].to<JsonArray>();
    for (uint8_t i = 0; i < getOutputCount(); ++i)
    {
        JsonObject item = outputs.add<JsonObject>();
        item["channel"] = i + 1;
        item["gpio"] = getOutputPin(i);
        item["on"] = isOutputEnabled(i + 1);
    }

    JsonArray sensors = doc["sensors"].to<JsonArray>();
    for (uint8_t i = 0; i < getFloatCount(); ++i)
    {
        JsonObject item = sensors.add<JsonObject>();
        const uint8_t pin = getFloatPin(i);
        item["sensorCode"] = getFloatSensorCode(i);
        item["suffix"] = getFloatSensorCode(i);
        item["gpio"] = pin;
        item["interface"] = "GPIO";
        item["channel"] = String("GPIO") + pin;
        item["sensorType"] = "Float";
        item["unit"] = "state";
    }

    String body;
    serializeJson(doc, body);
    server.send(200, "application/json", body);
}

void EdgeNetwork::handleRestart()
{
    server.send(200, "application/json",
                "{\"success\":true,\"message\":\"Restart scheduled\"}");
    restartPending = true;
    restartAt = millis() + 500;
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
    const String newKioskUrl = String(doc["kioskUrl"] | "");
    if (newSsid.isEmpty())
    {
        if (newKioskUrl.isEmpty())
        {
            server.send(400, "application/json",
                        "{\"success\":false,\"message\":\"ssid or kioskUrl is required\"}");
            return;
        }
        kioskUrl = newKioskUrl;
        preferences.putString("kioskUrl", kioskUrl);
        lastRegisterAt = 0;
        server.send(200, "application/json",
                    "{\"success\":true,\"message\":\"Kiosk URL saved\"}");
        return;
    }

    preferences.putString("ssid", newSsid);
    preferences.putString("password", newPassword);
    if (!newKioskUrl.isEmpty())
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
    command.trim();

    int channel = 0;
    JsonVariant channelValue = doc["channel"];
    if (channelValue.is<int>())
        channel = channelValue.as<int>();
    else
    {
        const char *rawChannel = channelValue.as<const char *>();
        String channelText = rawChannel ? String(rawChannel) : String("");
        channelText.toLowerCase();
        channelText.trim();
        if (channelText == "1" || channelText == "ssr1" || channelText == "pump1"
            || channelText == "pump_01" || channelText == "pump_1")
            channel = 1;
        else if (channelText == "2" || channelText == "ssr2" || channelText == "pump2"
                 || channelText == "pump_02" || channelText == "pump_2")
            channel = 2;
        else
            channel = channelText.toInt();
    }

    bool success = true;
    if (command == "alloff")
    {
        allOutputsOff();
    }
    else if (command == "on" || command == "off" || command == "toggle")
    {
        if (channel < 1 || channel > 2)
            success = false;
        else
        {
            const bool enabled = command == "on"
                ? true
                : command == "off" ? false : !isOutputEnabled(channel);
            success = setOutput(channel, enabled);
        }
    }
    else if (command == "status")
    {
        success = true;
    }
    else
    {
        success = false;
    }

    JsonDocument response;
    response["success"] = success;
    response["command"] = command;
    response["channel"] = channel;
    response["output1"] = isOutputEnabled(1);
    response["output2"] = isOutputEnabled(2);
    response["message"] = success ? "Applied" : "Invalid command or channel";
    String body;
    serializeJson(response, body);
    server.send(success ? 200 : 400, "application/json", body);
    Serial.print("API ");
    Serial.println(body);
}

void EdgeNetwork::registerWithKiosk()
{
    if (lastRegisterAt != 0 && millis() - lastRegisterAt < 15000)
        return;
    lastRegisterAt = millis();

    String base = kioskUrl;
    base.trim();
    while (base.endsWith("/"))
        base.remove(base.length() - 1);
    if (base.isEmpty())
        return;

    JsonDocument doc;
    doc["deviceCode"] = deviceCode();
    doc["controller_id"] = deviceCode();
    doc["mac"] = macAddress();
    doc["firmware"] = "1.0.0";
    doc["hardware"] = "ESP32-V1";
    doc["ipAddress"] = WiFi.localIP().toString();
    String payload;
    serializeJson(doc, payload);

    HTTPClient http;
    if (!http.begin(base + "/api/controllers/register"))
        return;
    http.addHeader("Content-Type", "application/json");
    const int code = http.POST(payload);
    if (code == 200 || code == 201)
    {
        JsonDocument response;
        if (deserializeJson(response, http.getString()) == DeserializationError::Ok)
        {
            const char *secret = response["secret"];
            if (secret != nullptr && secret[0] != '\0')
            {
                controllerSecret = secret;
                preferences.putString("controllerSecret", controllerSecret);
                Serial.println("Controller credential saved");
            }
            const char *status = response["status"];
            if (status != nullptr)
            {
                Serial.print("Controller status ");
                Serial.println(status);
            }
        }
    }
    else
    {
        Serial.print("Kiosk register ERR ");
        Serial.println(code);
    }
    http.end();
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
        reading["unit"] = "state";
    }
    MeterReading meter{};
    if (latestMeter(&meter))
    {
        auto add = [&](const char *code, float value, const char *unit) {
            JsonObject reading = readings.add<JsonObject>();
            reading["pin"] = 33;
            reading["val"] = value;
            reading["sensor"] = code;
            reading["unit"] = unit;
        };
        add("meter_v", meter.volts, "V");
        add("meter_a", meter.amps, "A");
        add("meter_w", meter.watts, "W");
        add("meter_va", meter.va, "VA");
        add("meter_kwh", meter.kwh, "kWh");
        add("meter_hz", meter.hertz, "Hz");
        add("meter_pf", meter.pf, "%");
    }

    String payload;
    serializeJson(doc, payload);
    HTTPClient http;
    if (!http.begin(base + "/api/telemetry"))
        return;
    http.addHeader("Content-Type", "application/json");
    const int code = http.POST(payload);
    if (code != 200 && code != 201)
    {
        Serial.print("Kiosk telemetry ERR ");
        Serial.println(code);
    }
    http.end();
}

String EdgeNetwork::deviceCode() const
{
    return "CrabSense-C115";
}

String EdgeNetwork::apName() const
{
    return "CrabSense-C115";
}

String EdgeNetwork::macAddress() const
{
    return WiFi.macAddress();
}
