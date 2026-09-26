# CrabSense Hardware Device Controller

Firmware skeleton for one standalone ESP32-S3 device controller.

## Structure

```text
HardwareDeviceController.ino
config/
  Pins.h
  Settings.h
drivers/
  DeviceController.cpp
  DeviceController.h
  FloatController.cpp
  FloatController.h
modules/
  CommandRouter.cpp
  CommandRouter.h
platformio.ini
```

The layout follows `hardware-crabmonitor-ai`: board configuration is kept in
`config/`, hardware access in `drivers/`, and device workflows in `modules/`.

## Current scaffold

- PlatformIO + Arduino framework
- ESP32-S3 DevKitC-1 target
- Four safe-by-default digital outputs
- Four debounced float/reed inputs:
  - `tank_01_low`
  - `tank_01_high`
  - `tank_02_low`
  - `tank_02_high`
- Serial command interface at `115200` baud
- Emergency-safe startup state: all outputs are disabled

Commands:

```text
on 1
off 1
alloff
status
floats
help
```

Before connecting hardware, confirm the GPIO mapping and active-low/active-high
polarity in `config/Pins.h` and `config/Settings.h`. The default float wiring
uses `INPUT_PULLUP`, with the reed switch closing to GND.

When a float state changes, the firmware prints a BE-ready event:

```json
{"sensor":"tank_01_low","state":true}
```

`true` means the contact is closed/active. Whether that means "low", "high",
"empty", or "full" is a configuration decision outside the ESP firmware.

## Build and upload

```bash
pio run
pio run -t upload
pio device monitor
```

Network communication with CrabSense BE is intentionally not implemented in
this initial scaffold. Add the transport and device protocol after the ESP
pinout and command contract are confirmed.
