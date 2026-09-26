#ifndef FLOAT_CONTROLLER_H
#define FLOAT_CONTROLLER_H

#include <Arduino.h>

void setupFloatController();
void pollFloatInputs();
void printFloatStatus();
uint8_t getFloatCount();
uint8_t getFloatPin(uint8_t index);
const char *getFloatSensorCode(uint8_t index);
bool getFloatState(uint8_t index);

#endif
