#include "DeviceController.h"

#include "../config/Pins.h"
#include "../config/Settings.h"

namespace
{
constexpr uint8_t outputPins[] = {
    PIN_OUTPUT_1,
    PIN_OUTPUT_2};
constexpr uint8_t outputCount = sizeof(outputPins) / sizeof(outputPins[0]);

bool outputState[outputCount] = {};

void updateStatusLed()
{
    const bool anyOn = outputState[0] || (outputCount > 1 && outputState[1]);
    digitalWrite(PIN_STATUS_LED, anyOn ? HIGH : LOW);
}

bool validChannel(uint8_t channel)
{
    return channel >= 1 && channel <= outputCount;
}
}

void setupDeviceController()
{
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    for (uint8_t channel = 1; channel <= outputCount; ++channel)
    {
        pinMode(outputPins[channel - 1], OUTPUT);
    }

    allOutputsOff();
}

void allOutputsOff()
{
    for (uint8_t channel = 1; channel <= outputCount; ++channel)
    {
        digitalWrite(outputPins[channel - 1], OUTPUT_INACTIVE_LEVEL);
        outputState[channel - 1] = false;
    }
    updateStatusLed();
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
    updateStatusLed();
    return true;
}

bool isOutputEnabled(uint8_t channel)
{
    return validChannel(channel) && outputState[channel - 1];
}
