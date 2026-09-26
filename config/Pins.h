#ifndef PINS_H
#define PINS_H

// Assign the real board wiring before connecting actuators.
#define PIN_STATUS_LED 2

// Output channels available on the ESP32 DevKit V1 shown in the wiring photo.
#define PIN_OUTPUT_1 4
#define PIN_OUTPUT_2 5
#define PIN_OUTPUT_3 18
#define PIN_OUTPUT_4 19

// Float/reed inputs. Wire each switch between the GPIO and GND.
#define PIN_FLOAT_1 13
#define PIN_FLOAT_2 14
#define PIN_FLOAT_3 16
#define PIN_FLOAT_4 17

#endif
