#ifndef POWER_METER_H
#define POWER_METER_H

struct MeterReading
{
    bool ok;
    float volts;
    float amps;
    float watts;
    float va;
    float kwh;
    float hertz;
    float pf;
};

void setupPowerMeter();
void pollPowerMeter();
void printPowerMeterStatus();
bool latestMeter(MeterReading *out);

#endif
