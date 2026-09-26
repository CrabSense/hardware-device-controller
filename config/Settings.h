#ifndef SETTINGS_H
#define SETTINGS_H

#define SERIAL_BAUD_RATE 115200
#define COMMAND_BUFFER_SIZE 64
#define FLOAT_DEBOUNCE_MS 50
#define FLOAT_LOG_INTERVAL_MS 2000

// Keep outputs safe until the wiring and polarity are confirmed.
#define OUTPUT_ACTIVE_LEVEL LOW
#define OUTPUT_INACTIVE_LEVEL HIGH

// Reed switch closed to GND is treated as an active float signal.
#define FLOAT_ACTIVE_LEVEL LOW
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define TELEMETRY_INTERVAL_MS 5000
#define PROVISION_HTTP_PORT 80
#define DEFAULT_KIOSK_URL "http://192.168.1.10:8090"

#endif
