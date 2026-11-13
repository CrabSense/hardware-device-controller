# RAS controller inventory (Edge V2 RC2)
 
The authenticated `kind: inventory` request on /api/command returns a signed
manifest. The Kiosk verifies the HMAC, nonce, device and protocol before
forwarding it to BE with the dedicated device credential. No cloud secret
is sent in this request to the ESP32.
 
The manifest is generated from the actual firmware driver list and includes
SSR outputs, float inputs and the configured RS485 bus. These are declared
ports, not proof a physical load is attached. NVS keeps the descriptor and
monotonic revision across restarts. No GPIO scanning or test actuation.
 
To expand a board, append a verified GPIO to RAS_OUTPUT_PINS in config/Pins.h
and preserve existing channel numbering. Core supports up to 16 outputs.
Existing commissioned boards missing an old sequence slot fail closed. An
append-only firmware expansion initializes only new slots at the previous
maximum sequence; stores the count last; and refuses downgrade. Never erase
NVS or renumber existing channels. A new board provisions all configured slots.
Verify safe GPIO assignment and power-loss recovery before flashing.
 
ON remains locked at both firmware layers, as required by EDGE-V2.md.
The signed inventory advertises remoteControl=false. Physical interlocks,
wiring/polarity, dry-run and overflow tests are required before changing that.
The reference Python bridge is not the production Kiosk; use the updated
CrabSenseKiosk Edge V2 worker for signed inventory and dynamic channels.

Command envelopes must carry the current inventoryRevision; a stale revision is rejected before writing GPIO. The old reference bridge does not supply this field and must not be used with RC2.

## Flash layout
RC2 exceeds the old 1.25 MiB app partition. PlatformIO uses min_spiffs.csv for the 4 MB ESP32 DevKit (two larger app slots, smaller SPIFFS). Verify actual flash capacity. This changes app partition offsets: upgrade existing units by a controlled wired flash with loads isolated, preserving NVS and validating credential/sequence recovery. Do not OTA-upgrade across partition tables or erase flash. No SPIFFS data is used by this firmware, but verify any deployed custom data before migration.
