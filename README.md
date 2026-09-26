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
- Two SSR-40DA digital outputs (active-high, safe OFF at boot)
- Four debounced float/reed inputs:
  - `float_1`
  - `float_2`
  - `float_3`
  - `float_4`
- Serial command interface at `115200` baud
- Wi-Fi telemetry to the local Kiosk and HTTP command endpoint
- Emergency-safe startup state: both SSR outputs are disabled

Pin mapping for the current ESP32 DevKit V1:

```text
Float 1        GPIO32
Float 2        GPIO33
Float 3        GPIO25
Float 4        GPIO26
SSR 1 input    GPIO27
SSR 2 input    GPIO14
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

For each SSR-40DA, connect ESP32 GPIO to input `3 (+)` and ESP32 GND to input
`4 (-)`. SSR output terminals `1/2` are the AC switching path; never connect
AC to input terminals `3/4`. The default float wiring uses `INPUT_PULLUP`,
with each reed switch wired between its GPIO and GND.

SSR output commands:

```text
on 1       # SSR 1 ON
off 1      # SSR 1 OFF
on 2       # SSR 2 ON
off 2      # SSR 2 OFF
alloff     # both SSRs OFF
status
```

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

