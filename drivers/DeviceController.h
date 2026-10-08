#ifndef DEVICE_CONTROLLER_H
#define DEVICE_CONTROLLER_H

#include <Arduino.h>

void setupDeviceController();
void allOutputsOff();
bool setOutput(uint8_t channel, bool enabled);
bool isOutputEnabled(uint8_t channel);
uint8_t getOutputCount();
uint8_t getOutputPin(uint8_t index);

#endif
