#ifndef POWER_METER_H
#define POWER_METER_H
#include <stdint.h>

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
    float minutes;
    float celsius;
};

void setupPowerMeter();
void pollPowerMeter();
void printPowerMeterStatus();
bool latestMeter(MeterReading *out);
bool latestMeterSample(MeterReading *out, uint64_t *measuredEpoch);

#endif
