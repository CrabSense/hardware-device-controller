#include "FloatController.h"

#include "../config/Pins.h"
#include "../config/Settings.h"

namespace
{
struct FloatInput
{
    const char *displayName;
    const char *sensorCode;
    uint8_t pin;
    bool stableState;
    bool lastRawState;
    unsigned long rawChangedAt;
};

FloatInput inputs[] = {
    {"Phao 1", "float_1", PIN_FLOAT_1, false, false, 0},
    {"Phao 2", "float_2", PIN_FLOAT_2, false, false, 0},
    {"Phao 3", "float_3", PIN_FLOAT_3, false, false, 0},
    {"Phao 4", "float_4", PIN_FLOAT_4, false, false, 0}};

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
        Serial.print(input.displayName);
        Serial.print(" [GPIO");
        Serial.print(input.pin);
        Serial.print("]: ");
        Serial.println(input.stableState ? "ON" : "OFF");
    }
}
