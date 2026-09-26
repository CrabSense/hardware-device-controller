# CrabSense Hardware Device Controller

Firmware skeleton for one standalone ESP32 DevKit V1 (ESP32-WROOM-32)
device controller.

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
- ESP32 DevKit V1 (`esp32dev`) target
- Four safe-by-default digital outputs
- Four debounced float/reed inputs:
  - `float_1`
  - `float_2`
  - `float_3`
  - `float_4`
- Serial command interface at `115200` baud
- Wi-Fi telemetry to the local Kiosk and HTTP command endpoint
- Emergency-safe startup state: all outputs are disabled

Pin mapping for the current ESP32 DevKit V1:

```text
Float 1        GPIO13
Float 2        GPIO14
Float 3        GPIO16
Float 4        GPIO17
Output 1       GPIO4
Output 2       GPIO5
Output 3       GPIO18
Output 4       GPIO19
```

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
{"sensor":"float_1","state":true}
```

`true` means the contact is closed/active. Whether that means "low", "high",
"empty", or "full" is assigned later by BE/FE, outside the ESP firmware.

## Kiosk provisioning

On first boot, connect to the ESP access point `CrabSense-XXXX` and send:

```json
{
  "ssid": "farm-wifi",
  "password": "wifi-password",
  "kioskUrl": "http://192.168.1.50:8090"
}
```

to `POST http://192.168.4.1/api/provision`.

After joining Wi-Fi, the ESP posts float telemetry to
`POST <kioskUrl>/api/telemetry` and accepts Kiosk commands at
`POST http://<esp-ip>/api/command`.

## Build and upload

```bash
pio run
pio run -t upload
pio device monitor
```

Network communication with CrabSense BE is intentionally not implemented in
this initial scaffold. Add the transport and device protocol after the ESP
pinout and command contract are confirmed.
