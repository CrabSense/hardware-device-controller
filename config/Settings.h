#ifndef SETTINGS_H
#define SETTINGS_H

#define SERIAL_BAUD_RATE 115200
#define COMMAND_BUFFER_SIZE 64
#define FLOAT_DEBOUNCE_MS 50
#define FLOAT_LOG_INTERVAL_MS 1000

// Keep outputs safe until the wiring and polarity are confirmed.
#define OUTPUT_ACTIVE_LEVEL LOW
#define OUTPUT_INACTIVE_LEVEL HIGH

// Reed switch closed to GND is treated as an active float signal.
#define FLOAT_ACTIVE_LEVEL LOW

#endif
