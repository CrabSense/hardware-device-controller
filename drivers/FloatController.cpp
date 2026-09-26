#include "FloatController.h"

#include "../config/Pins.h"
#include "../config/Settings.h"

namespace
{
struct FloatInput
{
    const char *sensorCode;
    uint8_t pin;
    bool stableState;
    bool lastRawState;
    unsigned long rawChangedAt;
};

FloatInput inputs[] = {
    {"tank_01_low", PIN_TANK_1_LOW, false, false, 0},
    {"tank_01_high", PIN_TANK_1_HIGH, false, false, 0},
    {"tank_02_low", PIN_TANK_2_LOW, false, false, 0},
    {"tank_02_high", PIN_TANK_2_HIGH, false, false, 0}};

constexpr size_t inputCount = sizeof(inputs) / sizeof(inputs[0]);

bool readActive(uint8_t pin)
{
    return digitalRead(pin) == FLOAT_ACTIVE_LEVEL;
}

void printReading(const FloatInput &input)
{
    Serial.print("{\"sensor\":\"");
    Serial.print(input.sensorCode);
    Serial.print("\",\"state\":");
    Serial.print(input.stableState ? "true" : "false");
    Serial.println("}");
}
}

void setupFloatController()
{
    const unsigned long now = millis();
    for (FloatInput &input : inputs)
    {
        pinMode(input.pin, INPUT_PULLUP);
        input.stableState = readActive(input.pin);
        input.lastRawState = input.stableState;
        input.rawChangedAt = now;
    }
}

void pollFloatInputs()
{
    const unsigned long now = millis();
    for (FloatInput &input : inputs)
    {
        const bool rawState = readActive(input.pin);
        if (rawState != input.lastRawState)
        {
            input.lastRawState = rawState;
            input.rawChangedAt = now;
            continue;
        }

        if (rawState != input.stableState
            && now - input.rawChangedAt >= FLOAT_DEBOUNCE_MS)
        {
            input.stableState = rawState;
            printReading(input);
        }
    }
}

void printFloatStatus()
{
    for (const FloatInput &input : inputs)
    {
        printReading(input);
    }
}
