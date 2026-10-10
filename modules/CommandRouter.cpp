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
    Serial.println();
    Serial.println("CrabSense device controller");
    Serial.println("Commands:");
    Serial.println("  on <1-4>       Enable SSR");
    Serial.println("  off <1-4>      Disable SSR");
    Serial.println("  alloff         Disable every output");
    Serial.println("  status         Show output states");
    Serial.println("  floats         Print all float states");
    Serial.println("  meter          Poll RS485 power meter");
    Serial.println("  quiet          Stop repeating float log");
    Serial.println("  log            Resume repeating float log");
    Serial.println("  help            Show this help");
    Serial.println();
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
        if (!setOutput(channel, true))
        {
            Serial.println("ERR invalid channel");
            return;
        }
        Serial.println("OK output ON");
        reportOutputs();
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
