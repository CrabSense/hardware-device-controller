#include "EdgeV2.h"
#include "EdgeV2Core.h"
#include "CapabilityManifest.h"
#include "../drivers/DeviceController.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <mbedtls/md.h>
#include <time.h>

namespace {
Preferences nvs;
String key;
bool storageOk = false;
uint64_t marks[edgev2::MAX_OUTPUTS] = {};
unsigned long heartbeatAt = 0;
bool heartbeatValid = false;
uint64_t lastHeartbeatEpoch = 0;
constexpr unsigned long WATCHDOG_MS = 8000;
constexpr uint64_t MIN_EPOCH = 1700000000ULL;
// ON is deliberately locked until physical wiring and independent interlocks are verified.
bool permitOn(unsigned) { return false; }
String signature(const String& body) {
    unsigned char digest[32];
    const auto* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md || mbedtls_md_hmac(md, reinterpret_cast<const unsigned char*>(key.c_str()), key.length(),
            reinterpret_cast<const unsigned char*>(body.c_str()), body.length(), digest) != 0) return "";
    char hex[65]; for (unsigned i=0;i<32;++i) snprintf(hex+2*i,3,"%02x",digest[i]);
    return String(hex);
}
bool same(const String& a, const String& b) {
    if (a.length()!=64 || b.length()!=64) return false;
    unsigned char diff=0; for(unsigned i=0;i<64;++i) diff |= a[i]^b[i]; return diff==0;
}
bool validKey(const String& s) {
    if(s.length()!=64) return false;
    for(unsigned i=0;i<s.length();++i) if (!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f'))) return false;
    return true;
}
String slot(unsigned ch) { return String("seq")+ch; }
struct BoardIO : edgev2::IO {
    uint64_t high(unsigned ch) override { return marks[ch-1]; }
    bool persist(unsigned ch, uint64_t seq) override {
        if (!storageOk || nvs.putULong64(slot(ch).c_str(),seq)!=sizeof(uint64_t) || nvs.getULong64(slot(ch).c_str(),0)!=seq) return false;
        marks[ch-1]=seq; return true;
    }
    bool state(unsigned ch) override { return isOutputEnabled(ch); }
    bool write(unsigned ch,bool on) override { return (!on || (heartbeatValid && permitOn(ch))) && setOutput(ch,on); }
    void safeOff() override { allOutputsOff(); heartbeatValid=false; }
} board;
void reply(WebServer& server, JsonDocument& d, int status) {
    String body; serializeJson(d,body); String sig=signature(body);
    if(sig.length()==64) server.sendHeader("X-Kiosk-Signature",sig);
    server.send(status,"application/json",body);
}
void reject(WebServer& server,int code,const char* reason) { JsonDocument d; d["success"]=false; d["message"]=reason; reply(server,d,code); }
}
namespace edgev2fw {
void begin(WebServer& server) {
    allOutputsOff(); storageOk=getOutputCount()>0 && getOutputCount()<=edgev2::MAX_OUTPUTS && nvs.begin("edgev2",false);
    if(storageOk) {
        key=nvs.getString("key","");
        bool commissioned=nvs.getBool("commissioned",false);
        for(unsigned i=1;i<=getOutputCount();++i) {
            if(commissioned && !nvs.isKey(slot(i).c_str())) storageOk=false;
            marks[i-1]=nvs.getULong64(slot(i).c_str(),0);
        }
        if(!commissioned && !key.isEmpty()) storageOk=false;
    }
    const char* headers[]={"X-Kiosk-Signature"}; server.collectHeaders(headers,1);
    configTime(0,0,"pool.ntp.org","time.nist.gov");
}
void tick(bool connected) {
    if(!connected || !heartbeatValid || millis()-heartbeatAt>=WATCHDOG_MS || time(nullptr)<static_cast<time_t>(MIN_EPOCH)) {
        allOutputsOff(); heartbeatValid=false;
    }
    for(unsigned ch=1;ch<=getOutputCount();++ch) if(isOutputEnabled(ch) && !permitOn(ch)) { allOutputsOff(); heartbeatValid=false; }
}
bool serial(const String& line) {
    if(!line.startsWith("edgekey ")) return false;
    allOutputsOff(); heartbeatValid=false;
    String candidate=line.substring(8); candidate.trim();
    if(!storageOk || !validKey(candidate) || !key.isEmpty() || nvs.getBool("commissioned",false)) { Serial.println("ERR edge key provisioning denied"); return true; }
    for(unsigned ch=1;ch<=getOutputCount();++ch) if(nvs.putULong64(slot(ch).c_str(),0)!=sizeof(uint64_t)) { storageOk=false; Serial.println("ERR sequence storage"); return true; }
    if(!nvs.putBool("commissioned",true) || nvs.putString("key",candidate)!=candidate.length()) {storageOk=false; Serial.println("ERR key storage"); return true;}
    key=candidate; Serial.println("OK edge key provisioned; ON remains locked"); return true;
}
void handle(WebServer& server,const String& deviceCode) {
    if(!storageOk || !validKey(key)) { reject(server,503,"not_commissioned"); return; }
    String body=server.arg("plain");
    if(body.length()>2048 || !same(signature(body),server.header("X-Kiosk-Signature"))) { reject(server,401,"invalid_signature"); return; }
    JsonDocument doc;
    if(deserializeJson(doc,body) || String(doc["deviceCode"]|"")!=deviceCode || (doc["protocol"]|0)!=2) {reject(server,400,"invalid_envelope");return;}
    const time_t clock=time(nullptr);
    if(clock<static_cast<time_t>(MIN_EPOCH) || !doc["expiresEpoch"].is<uint64_t>()) {reject(server,409,"clock_or_expiry_invalid");return;}
    uint64_t now=static_cast<uint64_t>(clock), expires=doc["expiresEpoch"].as<uint64_t>();
    if(expires<=now || expires-now>60) {reject(server,409,"expired_or_invalid_deadline");return;}
    String kind=doc["kind"]|""; String nonce=doc["nonce"]|"";
    if(nonce.length()!=36) {reject(server,400,"nonce_required");return;}
    JsonDocument out; out["nonce"]=nonce; out["deviceCode"]=deviceCode; out["protocol"]=2;
    bool success=false; const char* result="invalid_command";
    if(kind=="heartbeat") {
        if(expires-now<=5 && expires>lastHeartbeatEpoch) {lastHeartbeatEpoch=expires; heartbeatAt=millis(); heartbeatValid=true; success=true; result="heartbeat";}
        else result="stale_heartbeat";
    } else if(kind=="command") {
        String id=doc["id"]|"", attempt=doc["deliveryAttemptId"]|"", cmd=doc["command"]|"";
        out["id"]=id; out["deliveryAttemptId"]=attempt;
        if(id.length()!=36 || attempt.length()!=36 || !doc["commandSequence"].is<uint64_t>() || !doc["channel"].is<unsigned>()) {reject(server,400,"invalid_command_fields");return;}
        uint64_t seq=doc["commandSequence"].as<uint64_t>(); unsigned ch=doc["channel"].as<unsigned>(); out["commandSequence"]=seq;
        if(seq>INT64_MAX || (cmd!="on"&&cmd!="off"&&cmd!="alloff")) {reject(server,400,"invalid_command");return;}
        auto r=edgev2::execute(board,getOutputCount(),ch,seq,cmd=="on",cmd=="alloff");
        success=r==edgev2::Result::Applied||r==edgev2::Result::Duplicate; result=edgev2::message(r);
    }
    out["success"]=success; out["message"]=result;
    if(kind=="heartbeat" && success) appendCapabilityManifest(out["capabilities"].to<JsonObject>(), deviceCode);
    JsonArray states=out["outputs"].to<JsonArray>(); for(unsigned i=1;i<=getOutputCount();++i) {auto item=states.add<JsonObject>();item["channel"]=i;item["on"]=isOutputEnabled(i);}
    reply(server,out,200);
}
}
