#ifndef PINS_H
#define PINS_H

// SSR-40DA control inputs: ESP32 GPIO -> SSR pin 3 (+), GND -> SSR pin 4 (-).
#define PIN_STATUS_LED 2

// SSR channels. These pins drive only the low-voltage SSR inputs.
#define PIN_OUTPUT_1 27
#define PIN_OUTPUT_2 14

// Float/reed inputs. Wire each switch between the GPIO and GND.
#define PIN_FLOAT_1 32
#define PIN_FLOAT_2 33
#define PIN_FLOAT_3 25
#define PIN_FLOAT_4 26

#endif
