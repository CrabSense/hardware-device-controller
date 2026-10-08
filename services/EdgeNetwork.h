#ifndef EDGE_NETWORK_H
#define EDGE_NETWORK_H

#include <Arduino.h>

class EdgeNetwork
{
public:
    void begin();
    void loop();

private:
    void connectWifi();
    void startProvisioningAp();
    void handleProvisionPage();
    void handleCaptiveRedirect();
    void handleProvisionForm();
    void handleInfo();
    void handleRestart();
    void handleProvision();
    void handleCommand();
    void publishTelemetry();
    void registerWithKiosk();
    String deviceCode() const;
    String apName() const;
    String macAddress() const;

    String ssid;
    String password;
    String kioskUrl;
    String controllerSecret;
    bool provisioningMode = false;
    bool restartPending = false;
    unsigned long restartAt = 0;
    unsigned long lastTelemetryAt = 0;
    unsigned long lastRegisterAt = 0;
};

#endif
