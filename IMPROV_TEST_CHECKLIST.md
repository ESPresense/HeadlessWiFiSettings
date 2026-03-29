# SerialImprov PR Testing Checklist

Quick checklist for testing the Home Assistant SerialImprov integration PR.

## Pre-Testing Setup

- [ ] ESP32 board available
- [ ] USB cable (data, not charge-only)
- [ ] PlatformIO or Arduino IDE installed
- [ ] Home Assistant instance accessible (or Python with pyserial)

## Build and Flash

- [ ] Checkout the PR branch
- [ ] Build the library successfully
- [ ] Flash the example sketch (examples/SerialImprov/SerialImprov.ino)
- [ ] Verify serial output shows "SerialImprov Active"

## Core Functionality Tests

### Discovery

- [ ] Connect ESP32 to Home Assistant host via USB
- [ ] Device appears in Home Assistant integrations
- [ ] Device shows "Improv via Serial" badge/option

### Provisioning Flow

- [ ] Enter WiFi SSID in HA interface
- [ ] Enter WiFi password
- [ ] Device begins provisioning (state transition visible in serial log)
- [ ] Device connects to WiFi successfully
- [ ] Device reports success state
- [ ] Redirect URL is provided (if applicable)

### Post-Provisioning

- [ ] Device remains connected to WiFi
- [ ] Device shows up in HA after provisioning
- [ ] JSON endpoints are accessible at device IP
- [ ] Configuration persists after reboot

## Protocol Tests (using test_improv.py)

- [ ] Run: `./test_improv.py /dev/ttyUSB0`
- [ ] Device responds with current state
- [ ] IDENTIFY command works (LED blinks if configured)
- [ ] Run: `./test_improv.py /dev/ttyUSB0 "TestSSID" "password123"`
- [ ] Provisioning completes successfully

## Error Handling Tests

- [ ] Wrong WiFi password → ERROR_UNABLE_TO_CONNECT
- [ ] Non-existent SSID → ERROR_UNABLE_TO_CONNECT
- [ ] Invalid packet → device doesn't crash
- [ ] Serial disconnect during provisioning → device recovers

## UTF-8 and Edge Cases

- [ ] SSID with spaces: "My Network"
- [ ] SSID with special chars: "Café WiFi"
- [ ] Empty password (open network)
- [ ] Maximum length SSID (32 chars)
- [ ] Maximum length password (63 chars)

## Integration with Existing Features

- [ ] JSON GET /wifi/main returns configured parameters
- [ ] JSON POST /wifi/main still works
- [ ] Settings from Improv visible in JSON endpoints
- [ ] Settings from JSON visible after Improv provisioning
- [ ] Portal mode (HeadlessWiFiSettings.portal()) still works
- [ ] Custom parameters (string, integer, checkbox) work
- [ ] Callbacks fire correctly (onSuccess, onFailure, etc.)

## Browser Testing (Optional)

- [ ] Visit https://www.improv-wifi.com/
- [ ] Connect to ESP32 via Web Serial
- [ ] Provision WiFi through browser interface
- [ ] Verify successful connection

## Performance and Stability

- [ ] Device boots quickly with Improv enabled
- [ ] No memory leaks over multiple provisions
- [ ] Re-provisioning works (provision, reset, provision again)
- [ ] Concurrent access (Serial commands while HTTP endpoints active)

## Documentation

- [ ] README mentions SerialImprov support
- [ ] Example sketch is well-commented
- [ ] API documentation updated (if public API changes)

## Issues to Look For

Watch for these common problems:

- [ ] ❌ Checksum calculation errors
- [ ] ❌ Buffer overflow with long strings
- [ ] ❌ UTF-8 encoding/decoding issues
- [ ] ❌ Race conditions between Serial and WiFi events
- [ ] ❌ Memory leaks in packet handling
- [ ] ❌ State machine getting stuck
- [ ] ❌ Conflicts with existing WiFiSettings code

## Final Verification

- [ ] All core tests pass
- [ ] No compiler warnings
- [ ] Serial output is clean (no spam or errors)
- [ ] Memory usage is reasonable
- [ ] Code follows existing library style

## Sign-off

- [ ] Tested on ESP32 hardware: ________________
- [ ] Tested with Home Assistant version: ________________
- [ ] All critical tests pass
- [ ] Any issues documented in PR comments

---

## Quick Test Commands

```bash
# Build and upload (PlatformIO)
pio run -t upload -t monitor

# Build and upload (Arduino CLI)
arduino-cli compile --fqbn esp32:esp32:esp32 examples/SerialImprov
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 examples/SerialImprov

# Python protocol test
python3 test_improv.py /dev/ttyUSB0 "YourSSID" "YourPassword"

# Monitor serial
screen /dev/ttyUSB0 115200
# or
pio device monitor
```

## Resources

- Improv WiFi Spec: https://www.improv-wifi.com/serial/
- Test Tool: https://www.improv-wifi.com/
- Home Assistant ESPHome: https://esphome.io/
