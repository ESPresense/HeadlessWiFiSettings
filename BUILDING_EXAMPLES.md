# Building and Testing Examples

## The Problem

When you try to build from the library root directory, you get linker errors:
```
undefined reference to `setup()'
undefined reference to `loop()'
```

This happens because the root `platformio.ini` is for building the **library**, not example sketches.

## Solution: Build Examples Directly

### Method 1: PlatformIO (Recommended)

Each example now has its own `platformio.ini` file.

**Build ImprovWithHTTP example:**
```bash
cd examples/ImprovWithHTTP
pio run                    # Build
pio run -t upload          # Upload to ESP32
pio run -t upload -t monitor  # Upload and monitor serial
```

**Build SerialImprov example:**
```bash
cd examples/SerialImprov
pio run                    # Build
pio run -t upload          # Upload to ESP32
pio run -t upload -t monitor  # Upload and monitor serial
```

### Method 2: Arduino CLI

If you prefer Arduino CLI:

**Install the library locally:**
```bash
# From repo root
cd ..
ln -s HeadlessWiFiSettings ~/Documents/Arduino/libraries/
```

**Compile example:**
```bash
arduino-cli compile --fqbn esp32:esp32:esp32 examples/ImprovWithHTTP/ImprovWithHTTP.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 examples/ImprovWithHTTP/ImprovWithHTTP.ino
```

### Method 3: Arduino IDE

1. Open Arduino IDE
2. File → Open → Navigate to `examples/ImprovWithHTTP/ImprovWithHTTP.ino`
3. Tools → Board → ESP32 → ESP32 Dev Module
4. Sketch → Upload

---

## Which Example to Test?

### For Integration Testing: `ImprovWithHTTP`

This is the **comprehensive example** that shows:
- ✅ SerialImprov initialization
- ✅ HTTP JSON endpoints
- ✅ Custom parameters
- ✅ Proper callback handling
- ✅ Edge case handling

**Recommended for testing the PR integration.**

```bash
cd examples/ImprovWithHTTP
pio run -t upload -t monitor
```

### For Minimal Testing: `SerialImprov`

This is the **minimal example** from the PR:
- ✅ Basic Improv usage
- ❌ No HTTP endpoints
- ❌ No custom parameters
- ❌ No error handling

**Good for quick Improv-only test.**

```bash
cd examples/SerialImprov
pio run -t upload -t monitor
```

---

## Complete Testing Workflow

### 1. Build and Flash

```bash
cd examples/ImprovWithHTTP
pio run -t upload -t monitor
```

### 2. Provision via Improv

In another terminal:

```bash
# Back to repo root
cd ../..

# Provision via Python script
python3 test_improv.py /dev/ttyUSB0 "YourWiFiSSID" "YourPassword"
```

### 3. Run Integration Test

```bash
# Automated test (provisions + verifies HTTP)
./test_integration.sh /dev/ttyUSB0 "YourWiFiSSID" "YourPassword"
```

### 4. Manual HTTP Testing

Once device connects (watch serial monitor for IP):

```bash
# Get configuration
curl http://192.168.1.XXX/wifi/main

# Scan networks
curl http://192.168.1.XXX/wifi/scan

# Update config
curl -X POST -d "mqtt_server=test.local" http://192.168.1.XXX/wifi/main
```

---

## Troubleshooting

### Issue: `undefined reference to setup()`

**Problem**: You're in the library root directory.

**Solution**: Change to an example directory:
```bash
cd examples/ImprovWithHTTP
pio run
```

### Issue: `ImprovWiFi.h: No such file or directory`

**Problem**: Missing dependency.

**Solution**: PlatformIO should auto-install. If not:
```bash
pio lib install "judge2005/ImprovWiFi"
```

Or add to `platformio.ini`:
```ini
lib_deps =
    judge2005/ImprovWiFi
```

### Issue: Example won't compile in Arduino IDE

**Problem**: Library not installed.

**Solution**:
1. Sketch → Include Library → Add .ZIP Library
2. Select the HeadlessWiFiSettings folder
3. Or symlink to Arduino libraries folder:
   ```bash
   ln -s /path/to/HeadlessWiFiSettings ~/Documents/Arduino/libraries/
   ```

### Issue: Can't find serial port

**macOS**:
```bash
ls /dev/cu.* | grep -i usb
# Usually: /dev/cu.usbserial-* or /dev/cu.SLAB_USBtoUART
```

**Linux**:
```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

**Fix permissions** (Linux):
```bash
sudo usermod -a -G dialout $USER
# Then log out and back in
```

---

## Expected Output

When you flash `ImprovWithHTTP` and monitor serial:

```
=== SerialImprov + HTTP Integration Example ===

Initializing SerialImprov...
Checking for existing WiFi configuration...
→ Attempting WiFi connection...... ✓ Connected!
IP Address: 192.168.1.100

HTTP JSON endpoints available:
  GET/POST http://192.168.1.100/wifi/main
  GET      http://192.168.1.100/wifi/scan

=== Device Ready ===
Mode: WiFi Connected + SerialImprov Active

Current Configuration:
  MQTT Server: mqtt.example.com
  MQTT Port: 1883
  LED Enabled: Yes

✓ WiFi OK (-42 dBm)
```

If no WiFi configured:

```
=== SerialImprov + HTTP Integration Example ===

Initializing SerialImprov...
Checking for existing WiFi configuration...
→ Attempting WiFi connection... ✗ Connection failed!
Device can be re-provisioned via SerialImprov

=== Device Ready (No WiFi) ===
Mode: SerialImprov Only
Waiting for provisioning via:
  1. Home Assistant (USB auto-discovery)
  2. Web browser: https://www.improv-wifi.com/
  3. Python script: test_improv.py

✗ WiFi disconnected - awaiting Improv provisioning
```

---

## Quick Reference

| Task | Command |
|------|---------|
| Build example | `cd examples/ImprovWithHTTP && pio run` |
| Flash to ESP32 | `pio run -t upload` |
| Monitor serial | `pio run -t monitor` |
| Flash + monitor | `pio run -t upload -t monitor` |
| Provision via Improv | `python3 test_improv.py /dev/ttyUSB0 SSID PASS` |
| Integration test | `./test_integration.sh /dev/ttyUSB0 SSID PASS` |
| Check device IP | Watch serial monitor for "IP Address:" |
| Test HTTP | `curl http://[device-ip]/wifi/main` |

---

## For Your Situation

Based on your error, you need to:

```bash
# 1. Navigate to an example directory
cd examples/ImprovWithHTTP

# 2. Build the example
pio run

# 3. Flash to your ESP32
pio run -t upload -t monitor

# 4. In another terminal, provision it
cd ../..
python3 test_improv.py /dev/cu.SLAB_USBtoUART "YourWiFi" "YourPassword"
```

The build should succeed now because the example has its own `platformio.ini` that references the parent library correctly.
