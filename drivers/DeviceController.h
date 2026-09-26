#ifndef DEVICE_CONTROLLER_H
#define DEVICE_CONTROLLER_H

#include <Arduino.h>

void setupDeviceController();
void allOutputsOff();
bool setOutput(uint8_t channel, bool enabled);
bool isOutputEnabled(uint8_t channel);

#endif
