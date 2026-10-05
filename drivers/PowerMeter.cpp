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

void parseIfPossible()
{
    if (rxLen < 7)
        return;
    if (rxBuf[0] != RS485_SLAVE_ADDR)
        return;
    if (rxBuf[1] != 0x03 && rxBuf[1] != 0x04)
        return;

    const uint8_t bytes = rxBuf[2];
    if (rxLen < static_cast<size_t>(5 + bytes))
        return;

    Serial.println("RS485 parsed:");
    for (uint8_t i = 0; i + 1 < bytes; i += 2)
    {
        const uint16_t reg = (rxBuf[3 + i] << 8) | rxBuf[4 + i];
        Serial.print("  reg[");
        Serial.print(i / 2);
        Serial.print("] = ");
        Serial.println(reg);
    }
    if (bytes >= 8)
    {
        const uint16_t voltage = (rxBuf[3] << 8) | rxBuf[4];
        const uint16_t current = (rxBuf[5] << 8) | rxBuf[6];
        const uint16_t power = (rxBuf[7] << 8) | rxBuf[8];
        Serial.print("  V=");
        Serial.print(voltage / 10.0);
        Serial.print("  A=");
        Serial.print(current / 100.0);
        Serial.print("  W=");
        Serial.println(power / 10.0);
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
    drainRx();

    const unsigned long now = millis();
    if (now - lastPollAt < RS485_POLL_INTERVAL_MS)
        return;
    lastPollAt = now;

    if (rxLen > 0)
    {
        Serial.print("RS485 RX ");
        Serial.print(rxLen);
        Serial.print("B: ");
        printHex(rxBuf, rxLen);
        parseIfPossible();
        rxLen = 0;
    }
    else
    {
        Serial.println("RS485 RX: (empty)");
    }

    sendRead(0x03, 0x0000, 8);
}

void printPowerMeterStatus()
{
    pollPowerMeter();
}
