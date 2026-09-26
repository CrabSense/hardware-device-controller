#include "DeviceController.h"

#include "../config/Pins.h"
#include "../config/Settings.h"

namespace
{
constexpr uint8_t outputPins[] = {
    PIN_OUTPUT_1,
    PIN_OUTPUT_2,
    PIN_OUTPUT_3,
    PIN_OUTPUT_4};

bool outputState[sizeof(outputPins) / sizeof(outputPins[0])] = {};

bool validChannel(uint8_t channel)
{
    return channel >= 1 && channel <= (sizeof(outputPins) / sizeof(outputPins[0]));
}
}

void setupDeviceController()
{
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    for (uint8_t channel = 1; channel <= 4; ++channel)
    {
        pinMode(outputPins[channel - 1], OUTPUT);
    }

    allOutputsOff();
}

void allOutputsOff()
{
    for (uint8_t channel = 1; channel <= 4; ++channel)
    {
        digitalWrite(outputPins[channel - 1], OUTPUT_INACTIVE_LEVEL);
        outputState[channel - 1] = false;
    }
}

bool setOutput(uint8_t channel, bool enabled)
{
    if (!validChannel(channel))
    {
        return false;
    }

    digitalWrite(outputPins[channel - 1],
                 enabled ? OUTPUT_ACTIVE_LEVEL : OUTPUT_INACTIVE_LEVEL);
    outputState[channel - 1] = enabled;
    return true;
}

bool isOutputEnabled(uint8_t channel)
{
    return validChannel(channel) && outputState[channel - 1];
}
