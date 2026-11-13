#pragma once
#include <ArduinoJson.h>
#include <Preferences.h>
#include <mbedtls/md.h>
#include <cstring>
#include "../config/Pins.h"
#include "../config/Settings.h"
#include "../drivers/DeviceController.h"
#include "../drivers/FloatController.h"

// Derived from actual initialized drivers, never from a hardcoded DB channel list.
// Version is derived from the actual driver layout, persisted atomically in NVS.
// Firmware version changes alone do not invalidate an unchanged wiring layout.
struct CapabilityRecord { uint32_t version; uint8_t hash[32]; };
static CapabilityRecord capabilityRecord{};
inline bool initializeCapabilityManifest(Preferences& nvs) {
    // Compact canonical bytes: counts, ordered pins, length-delimited input names.
    // The HMAC authenticates the manifest; SHA-256 fingerprints the layout.
    uint8_t layout[1024];
    size_t used=0;
    auto add=[&](const void* data,size_t size) {
        if(size>sizeof(layout)-used) return false;
        memcpy(layout+used,data,size); used+=size; return true;
    };
    auto byte=[&](uint8_t value) {return add(&value,1);};
    if(!add(HARDWARE_MODEL,sizeof(HARDWARE_MODEL)) || !byte(getOutputCount())) return false;
    for(unsigned i=0;i<getOutputCount();++i) if(!byte(getOutputPin(i))) return false;
    if(!byte(getFloatCount())) return false;
    for(unsigned i=0;i<getFloatCount();++i) {
        const char* name=getFloatSensorCode(i);
        if(!byte(getFloatPin(i)) || !add(name,strlen(name)+1)) return false;
    }
    const uint8_t reserved[]={PIN_RS485_RX,PIN_RS485_TX,OUTPUT_ACTIVE_LEVEL,FLOAT_ACTIVE_LEVEL};
    if(!add(reserved,sizeof(reserved))) return false;
    uint8_t hash[32];
    const auto* md=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if(!md || mbedtls_md(md,layout,used,hash)!=0) return false;
    CapabilityRecord stored{};
    size_t length=nvs.getBytesLength("caplayout");
    if(length!=0 && (length!=sizeof(stored) || nvs.getBytes("caplayout",&stored,sizeof(stored))!=sizeof(stored) || stored.version==0)) return false;
    if(length==sizeof(stored) && memcmp(stored.hash,hash,sizeof(hash))==0) { capabilityRecord=stored; return true; }
    if(stored.version>=INT32_MAX) return false;
    CapabilityRecord next{}; next.version=stored.version+1; memcpy(next.hash,hash,sizeof(hash));
    if(nvs.putBytes("caplayout",&next,sizeof(next))!=sizeof(next)) return false;
    CapabilityRecord read{};
    if(nvs.getBytes("caplayout",&read,sizeof(read))!=sizeof(read) || memcmp(&read,&next,sizeof(next))!=0) return false;
    capabilityRecord=next; return true;
}
inline void appendCapabilityManifest(JsonObject doc, const String& deviceCode) {
    doc["deviceCode"]=deviceCode;
    doc["hardwareModel"]=HARDWARE_MODEL;
    doc["firmwareVersion"]=FIRMWARE_VERSION;
    doc["configVersion"]=capabilityRecord.version;
    JsonArray ports=doc["ports"].to<JsonArray>();
    for(unsigned i=0;i<getOutputCount();++i) {
        JsonObject p=ports.add<JsonObject>();
        char code[16]; snprintf(code,sizeof(code),"OUT-%02u",i+1);
        p["portCode"]=code; p["type"]="DIGITAL_OUTPUT"; p["channel"]=i+1;
        p["gpio"]=getOutputPin(i); p["activeLevel"]=OUTPUT_ACTIVE_LEVEL==HIGH?"HIGH":"LOW"; p["on"]=isOutputEnabled(i+1);
    }
    for(unsigned i=0;i<getFloatCount();++i) {
        JsonObject p=ports.add<JsonObject>();
        char code[16]; snprintf(code,sizeof(code),"IN-%02u",i+1);
        p["portCode"]=code; p["type"]="DIGITAL_INPUT"; p["channel"]=i+1;
        p["gpio"]=getFloatPin(i); p["activeLevel"]=FLOAT_ACTIVE_LEVEL==HIGH?"HIGH":"LOW"; p["sensorCode"]=getFloatSensorCode(i);
    }
    JsonObject bus=ports.add<JsonObject>();
    bus["portCode"]="RS485-01"; bus["type"]="RS485";
    bus["gpio"]=PIN_RS485_RX; bus["secondaryGpio"]=PIN_RS485_TX;
}
