#include "CommandRouter.h"

#include "../drivers/DeviceController.h"
#include "../drivers/FloatController.h"
#include "../drivers/PowerMeter.h"

namespace
{
bool floatLogEnabled = true;

void reportOutputs()
{
    Serial.print("OUTPUTS:");
    for (uint8_t channel = 1; channel <= getOutputCount(); ++channel)
    {
        Serial.print(" ");
        Serial.print(channel);
        Serial.print("=");
        Serial.print(isOutputEnabled(channel) ? "ON" : "OFF");
    }
    Serial.println();
}
}

void printCommandHelp()
{
    Serial.println("CrabSense: ON locked. Commands: off <channel>, alloff, status, floats, meter, quiet, log, help");
}

void handleSerialCommand(const String &command)
{
    if (command.equalsIgnoreCase("help"))
    {
        printCommandHelp();
        return;
    }

    if (command.equalsIgnoreCase("alloff"))
    {
        allOutputsOff();
        Serial.println("OK all outputs OFF");
        return;
    }

    if (command.equalsIgnoreCase("status"))
    {
        reportOutputs();
        return;
    }

    if (command.equalsIgnoreCase("floats"))
    {
        printFloatStatus();
        return;
    }

    if (command.equalsIgnoreCase("meter"))
    {
        printPowerMeterStatus();
        return;
    }

    if (command.equalsIgnoreCase("quiet"))
    {
        floatLogEnabled = false;
        Serial.println("OK float log OFF");
        return;
    }

    if (command.equalsIgnoreCase("log"))
    {
        floatLogEnabled = true;
        Serial.println("OK float log ON");
        return;
    }

    int channel = 0;
    if (sscanf(command.c_str(), "on %d", &channel) == 1)
    {
        Serial.println("ERR ON locked pending verified hardware interlocks");
        return;
    }

    if (sscanf(command.c_str(), "off %d", &channel) == 1)
    {
        if (!setOutput(channel, false))
        {
            Serial.println("ERR invalid channel");
            return;
        }
        Serial.println("OK output OFF");
        reportOutputs();
        return;
    }

    Serial.println("ERR unknown command");
}

bool isFloatLogEnabled()
{
    return floatLogEnabled;
}
