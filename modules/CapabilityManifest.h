#pragma once
#include <ArduinoJson.h>
#include "../config/Pins.h"
#include "../config/Settings.h"
#include "../drivers/DeviceController.h"
#include "../drivers/FloatController.h"

// Derived from actual initialized drivers, never from a hardcoded DB channel list.
// Increase when physical port layout changes; keep stable port IDs across versions.
constexpr unsigned CAPABILITY_CONFIG_VERSION = 1;
inline void appendCapabilityManifest(JsonObject doc, const String& deviceCode) {
    doc["deviceCode"]=deviceCode;
    doc["hardwareModel"]=HARDWARE_MODEL;
    doc["firmwareVersion"]=FIRMWARE_VERSION;
    doc["configVersion"]=CAPABILITY_CONFIG_VERSION;
    JsonArray ports=doc["ports"].to<JsonArray>();
    for(unsigned i=0;i<getOutputCount();++i) {
        JsonObject p=ports.add<JsonObject>();
        char code[16]; snprintf(code,sizeof(code),"OUT-%02u",i+1);
        p["portCode"]=code; p["type"]="DIGITAL_OUTPUT"; p["channel"]=i+1;
        p["gpio"]=getOutputPin(i); p["activeLevel"]="HIGH"; p["on"]=isOutputEnabled(i+1);
    }
    for(unsigned i=0;i<getFloatCount();++i) {
        JsonObject p=ports.add<JsonObject>();
        char code[16]; snprintf(code,sizeof(code),"IN-%02u",i+1);
        p["portCode"]=code; p["type"]="DIGITAL_INPUT"; p["channel"]=i+1;
        p["gpio"]=getFloatPin(i); p["activeLevel"]="LOW"; p["sensorCode"]=getFloatSensorCode(i);
    }
    JsonObject bus=ports.add<JsonObject>();
    bus["portCode"]="RS485-01"; bus["type"]="RS485";
    bus["gpio"]=PIN_RS485_RX; bus["secondaryGpio"]=PIN_RS485_TX;
}
