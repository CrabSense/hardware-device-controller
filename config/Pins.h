#ifndef PINS_H
#define PINS_H

// Assign the real board wiring before connecting actuators.
#define PIN_STATUS_LED 2

// Example actuator channels for this controller.
#define PIN_OUTPUT_1 4
#define PIN_OUTPUT_2 5
#define PIN_OUTPUT_3 6
#define PIN_OUTPUT_4 7

// Float/reed inputs. Wire each switch between the GPIO and GND.
#define PIN_TANK_1_LOW 8
#define PIN_TANK_1_HIGH 9
#define PIN_TANK_2_LOW 10
#define PIN_TANK_2_HIGH 11

#endif
