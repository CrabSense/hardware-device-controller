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
#define PIN_TANK_1_LOW 13
#define PIN_TANK_1_HIGH 14
#define PIN_TANK_2_LOW 16
#define PIN_TANK_2_HIGH 17

#endif
