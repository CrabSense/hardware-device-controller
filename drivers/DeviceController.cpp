#include "DeviceController.h"

#include "../config/Pins.h"
#include "../config/Settings.h"

namespace
{
constexpr uint8_t outputPins[] = {
    RAS_OUTPUT_PINS};
constexpr uint8_t outputCount = sizeof(outputPins) / sizeof(outputPins[0]);
// ESP32 DevKit output pins: exclude flash, boot strapping, serial, input-only,
// and all existing sensor/bus pins. Verify board-specific wiring before use.
constexpr bool safeOutput(uint8_t p) {
    return (p==4 || p==18 || p==19 || p==21 || p==22 || p==23 || p==25 || p==26 || p==27);
}
constexpr bool uniqueFrom(unsigned i, unsigned j) {
    return j>=outputCount ? true : (outputPins[i]!=outputPins[j] && uniqueFrom(i,j+1));
}
constexpr bool validOutputs(unsigned i) {
    return i>=outputCount ? true : (safeOutput(outputPins[i]) && uniqueFrom(i,i+1) && validOutputs(i+1));
}
static_assert(outputCount>0 && outputCount<=16 && validOutputs(0), "Invalid or duplicate RAS GPIO");

bool outputState[outputCount] = {};

void updateStatusLed()
{
    bool anyOn = false;
    for (uint8_t i = 0; i < outputCount; ++i)
        anyOn = anyOn || outputState[i];
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
        digitalWrite(outputPins[channel - 1], OUTPUT_INACTIVE_LEVEL);
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
    if (enabled || !validChannel(channel))
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

uint8_t getOutputCount()
{
    return outputCount;
}

uint8_t getOutputPin(uint8_t index)
{
    return index < outputCount ? outputPins[index] : 0;
}
