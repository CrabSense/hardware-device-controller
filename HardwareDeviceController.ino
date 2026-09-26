#include <Arduino.h>

#include "config/Settings.h"
#include "drivers/DeviceController.h"
#include "drivers/FloatController.h"
#include "modules/CommandRouter.h"

namespace
{
String commandBuffer;
unsigned long lastFloatLogAt = 0;

void readSerialCommands()
{
    while (Serial.available() > 0)
    {
        const char value = static_cast<char>(Serial.read());
        if (value == '\r' || value == '\n')
        {
            if (commandBuffer.length() > 0)
            {
                commandBuffer.trim();
                handleSerialCommand(commandBuffer);
                commandBuffer.clear();
            }
            continue;
        }

        if (commandBuffer.length() < COMMAND_BUFFER_SIZE - 1)
        {
            commandBuffer += value;
        }
    }
}
}

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);

    setupDeviceController();
    setupFloatController();
    Serial.println("READY");
    printCommandHelp();
    printFloatStatus();
}

void loop()
{
    pollFloatInputs();
    readSerialCommands();

    const unsigned long now = millis();
    if (now - lastFloatLogAt >= FLOAT_LOG_INTERVAL_MS)
    {
        lastFloatLogAt = now;
        printFloatStatus();
    }
}
