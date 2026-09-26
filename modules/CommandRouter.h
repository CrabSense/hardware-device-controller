#ifndef COMMAND_ROUTER_H
#define COMMAND_ROUTER_H

#include <Arduino.h>

void printCommandHelp();
void handleSerialCommand(const String &command);
bool isFloatLogEnabled();
void pollSsrTest();

#endif
