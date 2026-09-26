#include "CommandRouter.h"

#include "../drivers/DeviceController.h"

namespace
{
void reportOutputs()
{
    Serial.print("OUTPUTS:");
    for (uint8_t channel = 1; channel <= 4; ++channel)
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
    Serial.println("  on <1-4>       Enable output");
    Serial.println("  off <1-4>      Disable output");
    Serial.println("  alloff         Disable every output");
    Serial.println("  status         Show output states");
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

    int channel = 0;
    if (sscanf(command.c_str(), "on %d", &channel) == 1)
    {
        Serial.println(setOutput(channel, true) ? "OK output ON" : "ERR invalid channel");
        return;
    }

    if (sscanf(command.c_str(), "off %d", &channel) == 1)
    {
        Serial.println(setOutput(channel, false) ? "OK output OFF" : "ERR invalid channel");
        return;
    }

    Serial.println("ERR unknown command");
}
