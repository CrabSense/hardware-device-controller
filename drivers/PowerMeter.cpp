#include "PowerMeter.h"

#include "../config/Pins.h"
#include "../config/Settings.h"

#include <HardwareSerial.h>

namespace
{
HardwareSerial rs485(2);
unsigned long lastPollAt = 0;
uint8_t rxBuf[64];
size_t rxLen = 0;
MeterReading lastReading{};

uint16_t crc16(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; ++b)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
    }
    return crc;
}

void printHex(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        if (data[i] < 16)
            Serial.print('0');
        Serial.print(data[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
}

void drainRx()
{
    while (rs485.available() > 0 && rxLen < sizeof(rxBuf))
        rxBuf[rxLen++] = static_cast<uint8_t>(rs485.read());
}

void waitRx(unsigned long ms)
{
    const unsigned long start = millis();
    while (millis() - start < ms)
    {
        drainRx();
        delay(1);
    }
}

void sendRead(uint8_t fn, uint16_t start, uint16_t count)
{
    uint8_t req[8] = {
        RS485_SLAVE_ADDR,
        fn,
        static_cast<uint8_t>(start >> 8),
        static_cast<uint8_t>(start),
        static_cast<uint8_t>(count >> 8),
        static_cast<uint8_t>(count),
        0,
        0};
    const uint16_t crc = crc16(req, 6);
    req[6] = static_cast<uint8_t>(crc);
    req[7] = static_cast<uint8_t>(crc >> 8);

    Serial.print("RS485 TX: ");
    printHex(req, sizeof(req));
    rs485.write(req, sizeof(req));
    rs485.flush();
}

uint16_t regAt(uint8_t i)
{
    return static_cast<uint16_t>((rxBuf[3 + i * 2] << 8) | rxBuf[4 + i * 2]);
}

void parseIfPossible()
{
    if (rxLen < 7)
        return;
    if (rxBuf[0] != RS485_SLAVE_ADDR)
        return;
    if (rxBuf[1] != 0x03 && rxBuf[1] != 0x04)
        return;

    const uint8_t bytes = rxBuf[2];
    const size_t frame = static_cast<size_t>(5 + bytes);
    if (frame > sizeof(rxBuf) || rxLen < frame)
        return;
    const uint8_t n = bytes / 2;
    const uint16_t got = static_cast<uint16_t>(rxBuf[frame - 2] | (rxBuf[frame - 1] << 8));
    if (crc16(rxBuf, frame - 2) != got)
        return;
    // KWS-AC301 holding registers from 0x000E, 17 words.
    if (n >= 17)
    {
        const uint32_t energy = regAt(9) | (static_cast<uint32_t>(regAt(10)) << 16);
        lastReading.ok = true;
        lastReading.volts = regAt(0) / 10.0f;
        lastReading.amps = regAt(1) / 1000.0f;
        lastReading.watts = regAt(3) / 10.0f;
        lastReading.va = regAt(7) / 10.0f;
        lastReading.kwh = energy / 1000.0f;
        lastReading.minutes = regAt(11);
        lastReading.celsius = static_cast<int16_t>(regAt(12));
        lastReading.pf = regAt(15);
        lastReading.hertz = regAt(16) / 10.0f;
    }

    Serial.println("meter:");
    if (n > 0)
    {
        Serial.print("  V=");
        Serial.println(regAt(0) / 10.0, 2);
    }
    if (n > 1)
    {
        Serial.print("  A=");
        Serial.println(regAt(1) / 1000.0, 3);
    }
    if (n > 3)
    {
        Serial.print("  W=");
        Serial.println(regAt(3) / 10.0, 1);
    }
    if (n > 7)
    {
        Serial.print("  VA=");
        Serial.println(regAt(7) / 10.0, 1);
    }
    if (n > 9)
    {
        Serial.print("  kWh=");
        Serial.println(regAt(9) / 1000.0, 3);
    }
    if (n > 11)
    {
        Serial.print("  min=");
        Serial.println(regAt(11));
    }
    if (n > 12)
    {
        Serial.print("  C=");
        Serial.println(static_cast<int16_t>(regAt(12)));
    }
    if (n > 15)
    {
        Serial.print("  PF=");
        Serial.print(regAt(15));
        Serial.println('%');
    }
    if (n > 16)
    {
        Serial.print("  Hz=");
        Serial.println(regAt(16) / 10.0, 1);
    }
}
}

void setupPowerMeter()
{
    rs485.begin(RS485_BAUD_RATE, SERIAL_8N1, PIN_RS485_RX, PIN_RS485_TX);
    Serial.print("RS485 UART2 RX=");
    Serial.print(PIN_RS485_RX);
    Serial.print(" TX=");
    Serial.print(PIN_RS485_TX);
    Serial.print(" baud=");
    Serial.println(RS485_BAUD_RATE);
}

void pollPowerMeter()
{
    const unsigned long now = millis();
    if (now - lastPollAt < RS485_POLL_INTERVAL_MS)
        return;
    lastPollAt = now;

    while (rs485.available() > 0)
        rs485.read();
    rxLen = 0;

    sendRead(0x03, 0x000E, 17);
    waitRx(300);

    if (rxLen > 0)
    {
        Serial.print("UART RX ");
        Serial.print(rxLen);
        Serial.print("B: ");
        printHex(rxBuf, rxLen);
        parseIfPossible();
        rxLen = 0;
    }
    else
    {
        Serial.println("UART RX: (empty)");
    }
}

void printPowerMeterStatus()
{
    pollPowerMeter();
}

bool latestMeter(MeterReading *out)
{
    if (out == nullptr || !lastReading.ok)
        return false;
    *out = lastReading;
    return true;
}
