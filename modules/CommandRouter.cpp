#include "CommandRouter.h"

#include "../drivers/DeviceController.h"
#include "../drivers/FloatController.h"

namespace
{
bool floatLogEnabled = true;
bool ssrTestEnabled = false;
bool ssrTestOn = false;
unsigned long ssrTestChangedAt = 0;
constexpr unsigned long SSR_TEST_INTERVAL_MS = 10000;

void stopSsrTest()
{
    ssrTestEnabled = false;
    ssrTestOn = false;
    ssrTestChangedAt = 0;
}

void reportOutputs()
{
    Serial.print("OUTPUTS:");
    for (uint8_t channel = 1; channel <= 2; ++channel)
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
    Serial.println("  on <1-2>       Enable SSR");
    Serial.println("  off <1-2>      Disable SSR");
    Serial.println("  alloff         Disable every output");
    Serial.println("  status         Show output states");
    Serial.println("  floats         Print all float states");
    Serial.println("  quiet          Stop repeating float log");
    Serial.println("  log            Resume repeating float log");
    Serial.println("  test           Toggle both SSRs every 10s");
    Serial.println("  stop           Stop SSR test");
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
        stopSsrTest();
        allOutputsOff();
        Serial.println("OK all outputs OFF");
        return;
    }

    if (command.equalsIgnoreCase("test"))
    {
        ssrTestEnabled = true;
        ssrTestOn = true;
        ssrTestChangedAt = millis();
        setOutput(1, true);
        setOutput(2, true);
        Serial.println("OK SSR test ON 10s / OFF 10s");
        reportOutputs();
        return;
    }

    if (command.equalsIgnoreCase("stop"))
    {
        stopSsrTest();
        allOutputsOff();
        Serial.println("OK SSR test stopped");
        reportOutputs();
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
        stopSsrTest();
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
        stopSsrTest();
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

void pollSsrTest()
{
    if (!ssrTestEnabled)
        return;

    if (millis() - ssrTestChangedAt < SSR_TEST_INTERVAL_MS)
        return;

    ssrTestChangedAt = millis();
    ssrTestOn = !ssrTestOn;
    setOutput(1, ssrTestOn);
    setOutput(2, ssrTestOn);
    Serial.print(ssrTestOn ? "TEST SSR ON 10s" : "TEST SSR OFF 10s");
    Serial.println();
    reportOutputs();
}
