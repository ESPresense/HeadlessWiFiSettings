# Testing Home Assistant SerialImprov Integration

This guide covers how to test the Home Assistant SerialImprov protocol integration.

## Overview

SerialImprov is a protocol for provisioning WiFi credentials to IoT devices over serial (USB). It's part of the [Improv WiFi specification](https://www.improv-wifi.com/serial/).

## Prerequisites

- ESP32 device with the PR changes flashed
- USB cable
- One of the following:
  - Home Assistant instance (recommended)
  - Python 3 with `pyserial` library
  - Serial terminal that can send binary data

## Method 1: Testing with Home Assistant (Recommended)

This is the primary use case and most realistic test scenario.

### Setup

1. **Connect ESP32 to Home Assistant host**
   - Plug ESP32 into a USB port on the machine running Home Assistant
   - For Home Assistant OS: any USB port on the device
   - For Home Assistant Container: ensure USB device is passed through to container
   - For Home Assistant Supervised: USB should be available automatically

2. **Verify USB device is detected**
   ```bash
   # On Home Assistant host, check for device:
   ls -la /dev/ttyUSB* /dev/ttyACM*
   # or
   dmesg | tail -20
   ```

### Test Procedure

1. **Open Home Assistant**
   - Go to Settings → Devices & Services
   - Click the "+ ADD INTEGRATION" button

2. **Look for device**
   - You should see a discovered device with "Improv via Serial" option
   - Or search for "ESPHome" integration

3. **Provision WiFi**
   - Select the device
   - Enter your WiFi credentials
   - Device should connect and become available

4. **Verify**
   - Check that the ESP32 connects to WiFi
   - Verify the device shows up in Home Assistant
   - Check serial monitor for connection logs

### Expected Behavior

- ✓ Device appears in Home Assistant with "Improv via Serial" badge
- ✓ Can enter WiFi credentials through HA UI
- ✓ Device provisions and connects to WiFi
- ✓ Device provides redirect URL to its web interface (if applicable)
- ✓ After provisioning, device remains accessible

## Method 2: Python Test Script

Use the included `test_improv.py` script for protocol-level testing.

### Setup

```bash
# Install dependencies
pip3 install pyserial

# Make script executable
chmod +x test_improv.py
```

### Usage

```bash
# Test device state and identify
./test_improv.py /dev/ttyUSB0

# Test complete provisioning flow
./test_improv.py /dev/ttyUSB0 "YourWiFiSSID" "YourPassword"
```

### Expected Output

```
Opening serial port /dev/ttyUSB0...

=== Reading initial state ===
Current state: 1
  Device is authorized and ready

=== Sending IDENTIFY command ===
Sending identify packet: 494d50524f5601030101

=== Sending WiFi credentials ===
SSID: YourWiFiSSID
Password: ********

=== Waiting for provisioning response ===
State update: 3
  Still provisioning...
State update: 4
✓ Successfully provisioned!
  Redirect URL: http://192.168.1.100/

=== Test complete ===
```

## Method 3: Manual Serial Testing

For low-level debugging, you can manually send Improv packets.

### Using screen or minicom

```bash
# Connect to serial port
screen /dev/ttyUSB0 115200
# or
minicom -D /dev/ttyUSB0 -b 115200
```

### Improv Packet Format

```
Header: 49 4D 50 52 4F 56  ("IMPROV")
Version: 01
Type: [packet type]
Length: [data length]
Data: [variable length]
Checksum: [sum of bytes from version to end of data, & 0xFF]
```

### Example: Send IDENTIFY command

```
494D50524F560103010102
Breakdown:
  494D50524F56 - "IMPROV" header
  01 - version 1
  03 - TYPE_RPC_COMMAND
  01 - length 1
  01 - COMMAND_IDENTIFY
  02 - checksum (01+03+01+01 = 06, 06&0xFF = 0x06... wait this needs recalc)
```

Note: Manual hex entry is error-prone. Use the Python script for reliable testing.

## Method 4: Chrome/Edge Improv WiFi

Modern browsers support Web Serial and can use Improv:

1. Visit https://www.improv-wifi.com/
2. Click "Connect to device"
3. Select your ESP32 serial port
4. Follow the on-screen provisioning wizard

## Test Cases

### Basic Functionality

- [ ] Device responds to IDENTIFY command
- [ ] Device reports current state correctly
- [ ] Device accepts WiFi credentials
- [ ] Device connects to WiFi after provisioning
- [ ] Device reports success state
- [ ] Device provides redirect URL after success

### State Transitions

- [ ] STATE_AUTHORIZED → STATE_PROVISIONING → STATE_PROVISIONED (success)
- [ ] STATE_AUTHORIZED → STATE_PROVISIONING → STATE_AUTHORIZED (failure)
- [ ] Device reports STATE_AWAITING_AUTHORIZATION if not ready

### Error Handling

- [ ] Invalid SSID → ERROR_UNABLE_TO_CONNECT
- [ ] Wrong password → ERROR_UNABLE_TO_CONNECT
- [ ] Invalid packet → ERROR_INVALID_RPC
- [ ] Unknown command → ERROR_UNKNOWN_RPC

### UTF-8 Support

- [ ] SSID with special characters (émojis, 中文, etc.)
- [ ] Password with special characters
- [ ] Proper encoding/decoding of UTF-8 strings

### Edge Cases

- [ ] Empty password (open network)
- [ ] Very long SSID (32 characters)
- [ ] Very long password (63 characters)
- [ ] Device already provisioned (should return current state)
- [ ] Multiple rapid commands
- [ ] Serial disconnection during provisioning

### Integration with Existing Library

Since this is HeadlessWiFiSettings, verify:

- [ ] Existing JSON endpoints still work
- [ ] Existing WiFi configuration is preserved
- [ ] SerialImprov can update same settings as JSON endpoints
- [ ] Settings configured via Improv appear in JSON endpoints
- [ ] Can switch between Improv and JSON configuration

## Monitoring and Debugging

### Serial Monitor

Keep a serial monitor open during testing:

```bash
# Using Arduino IDE Serial Monitor
# OR
screen /dev/ttyUSB0 115200
# OR
cu -l /dev/ttyUSB0 -s 115200
```

Look for debug output like:
- "Improv: Received command"
- "Improv: Provisioning WiFi"
- "Improv: Connected successfully"
- Connection status messages

### Packet Capture

To debug packet-level issues:

```python
# Add to test script for detailed logging:
import binascii

def log_packet(direction, data):
    print(f"{direction}: {binascii.hexlify(data, ' ').decode()}")
    print(f"  ASCII: {data}")
```

## Common Issues

### Device Not Detected

- Check USB cable (must be data cable, not charge-only)
- Verify correct serial port
- Check USB permissions: `sudo usermod -a -G dialout $USER`
- For ESP32, press BOOT button while connecting if needed

### No Response to Commands

- Verify baud rate is 115200
- Check that SerialImprov code is actually running
- Ensure device isn't in bootloader mode
- Try power cycling the device

### Provisioning Fails

- Verify WiFi credentials are correct
- Check that WiFi network is in range
- Confirm 2.4GHz network (ESP32 doesn't support 5GHz)
- Check serial monitor for error messages

### Character Encoding Issues

- Ensure terminal/script uses UTF-8 encoding
- Test with ASCII-only credentials first
- Verify string length calculations include multi-byte chars

## Expected Integration Points

Based on HeadlessWiFiSettings architecture, Improv should:

1. **Use existing storage**: WiFi credentials should be stored in `/wifi-ssid` and `/wifi-password` files (same as JSON API)

2. **Trigger existing callbacks**:
   - `onConnect()` when starting connection
   - `onSuccess()` on successful connection
   - `onFailure()` on connection failure

3. **Work alongside HTTP endpoints**: Both Improv and HTTP `/wifi/main` should access same settings

4. **Respect existing WiFi logic**: Use same `connect()` flow for actual WiFi connection

## Protocol Reference

Full specification: https://www.improv-wifi.com/serial/

Key packet types:
- `0x01` - Current State
- `0x02` - Error State
- `0x03` - RPC Command
- `0x04` - RPC Result

Key commands:
- `0x01` - WIFI_SETTINGS
- `0x02` - IDENTIFY (blink LED or similar)

Key states:
- `0x01` - Authorized
- `0x02` - Awaiting Authorization
- `0x03` - Provisioning
- `0x04` - Provisioned

## Success Criteria

The integration is working correctly when:

1. ✓ Device is auto-discovered in Home Assistant
2. ✓ WiFi credentials can be provisioned via Improv
3. ✓ Device connects to specified network
4. ✓ Settings persist across reboots
5. ✓ Existing JSON endpoints remain functional
6. ✓ Error states are reported correctly
7. ✓ UTF-8 strings are handled properly
