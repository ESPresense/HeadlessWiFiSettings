# WiFi + SerialImprov Integration Test Plan

This test plan verifies that SerialImprov integrates correctly with the existing HeadlessWiFiSettings WiFi management.

## Integration Summary

The SerialImprov integration works by:
1. **Shared storage**: Both Improv and JSON endpoints use `/wifi-ssid` and `/wifi-password` files
2. **Improv callback**: Writes credentials to files, triggers `onConfigSaved()`, calls `connect(false)`
3. **Reuses connect()**: Same WiFi connection logic for all configuration methods

## Key Integration Points to Verify

### ✅ Storage Integration
- [ ] Credentials provisioned via Improv are saved to `/wifi-ssid` and `/wifi-password`
- [ ] Credentials saved via JSON POST to `/wifi/main` work with Improv
- [ ] Credentials persist across reboots regardless of provisioning method

### ✅ WiFi Connection Flow
- [ ] `connect(false)` is called after Improv provisioning
- [ ] Connection succeeds with valid credentials
- [ ] `onSuccess()` callback fires on successful connection
- [ ] `onFailure()` callback fires on connection failure
- [ ] `onConfigSaved()` callback fires when Improv saves credentials

### ✅ Coexistence with JSON Endpoints
- [ ] Can provision via Improv, then query settings via JSON GET
- [ ] Can provision via JSON POST, device still responds to Improv commands
- [ ] Custom parameters work alongside Improv
- [ ] HTTP server can be started after Improv provisioning

## Test Scenarios

### Scenario 1: Fresh Device (No WiFi Configured)

**Setup**: Factory reset device, no WiFi credentials stored

**Steps**:
1. Upload sketch with `beginSerialImprov()` and `connect()`
2. Device boots with no `/wifi-ssid` file
3. What happens?

**Expected Behavior**:
- Line 649-651 in connect(): If no SSID, calls `portal()`
- **⚠️ ISSUE**: Device goes into portal mode, never returns
- `serialImprovLoop()` in loop() never gets called!

**Test**:
- [ ] Verify device behavior with no WiFi configured
- [ ] Can device still be provisioned via Improv in this state?

**Recommended Fix**: Example should call `beginSerialImprov()` BEFORE `connect()`, and maybe use `connect(false)` to skip portal.

---

### Scenario 2: Improv Provisioning (Happy Path)

**Setup**: Fresh device or device with invalid WiFi

**Steps**:
1. Flash the SerialImprov example
2. Connect via Home Assistant or Python script
3. Send valid WiFi credentials via Improv
4. Observe connection process

**Expected Behavior**:
- Improv callback receives credentials (src/HeadlessWiFiSettings.cpp:352)
- Credentials saved to `/wifi-ssid` and `/wifi-password`
- `onConfigSaved()` fires
- `connect(false)` is called
- Device connects to WiFi
- `onSuccess()` fires
- Returns true

**Test**:
- [ ] Serial monitor shows: "Connecting to WiFi SSID 'xxx'"
- [ ] Device connects successfully
- [ ] Device shows IP address
- [ ] Files exist: `ls /wifi-ssid /wifi-password` (via SPIFFS)

---

### Scenario 3: Improv Provisioning (Wrong Password)

**Setup**: Fresh device

**Steps**:
1. Flash the SerialImprov example
2. Connect via Improv
3. Send credentials with WRONG password
4. Observe behavior

**Expected Behavior**:
- Credentials saved
- `connect(false)` called
- Connection fails (wrong password)
- `onFailure()` fires (line 682)
- **Portal does NOT start** because `connect(false)` (line 683)
- Returns false
- Loop continues calling `serialImprovLoop()`
- User can try again via Improv

**Test**:
- [ ] Device does NOT start portal
- [ ] Can re-provision via Improv with correct credentials
- [ ] Second attempt succeeds

**⚠️ ISSUE**: User gets no HTTP access, stuck with serial-only access until correct credentials provided.

---

### Scenario 4: Already Connected Device

**Setup**: Device with valid WiFi already configured

**Steps**:
1. Device has `/wifi-ssid` and `/wifi-password` files
2. Flash SerialImprov example
3. Device boots

**Expected Behavior**:
- `connect()` reads existing credentials (line 647-648)
- Connects successfully
- `onSuccess()` fires
- `serialImprovLoop()` runs in loop()
- Device can be re-provisioned via Improv if needed

**Test**:
- [ ] Device connects with existing credentials
- [ ] Can still receive Improv commands
- [ ] Sending new credentials via Improv re-provisions device
- [ ] Old credentials overwritten

---

### Scenario 5: Improv + JSON Endpoints

**Setup**: Modified example that calls `httpSetup()` after connect

```cpp
void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);

    // Custom parameters
    String mqtt = HeadlessWiFiSettings.string("mqtt_server", "mqtt.local");

    HeadlessWiFiSettings.beginSerialImprov("MyDevice", "1.0");
    bool connected = HeadlessWiFiSettings.connect(false, 30);

    if (connected) {
        HeadlessWiFiSettings.httpSetup();
        Serial.println("HTTP endpoints available");
    }
}

void loop() {
    HeadlessWiFiSettings.serialImprovLoop();
}
```

**Expected Behavior**:
- Device provisioned via Improv
- HTTP server starts
- JSON endpoints accessible at device IP

**Test**:
- [ ] Provision via Improv
- [ ] GET http://[device-ip]/wifi/main shows parameters
- [ ] POST to /wifi/main updates files
- [ ] Settings updated via JSON are reflected in next connection
- [ ] Improv can still update WiFi credentials

---

### Scenario 6: Custom Parameters + Improv

**Setup**: Example with custom parameters

**Steps**:
1. Define custom parameters BEFORE `beginSerialImprov()`
2. Provision via Improv
3. Check parameter persistence

**Expected Behavior**:
- Custom parameters stored in their own files (e.g., `/mqtt_server`)
- WiFi credentials in `/wifi-ssid` and `/wifi-password`
- All files persist independently

**Test**:
- [ ] Custom parameters defined before Improv init work
- [ ] Custom parameters accessible via JSON endpoints (if httpSetup called)
- [ ] Improv only affects WiFi credentials, not custom params
- [ ] Custom params persist across Improv re-provisioning

---

### Scenario 7: Callbacks Integration

**Setup**: Example with all callbacks defined

```cpp
HeadlessWiFiSettings.onConnect = []() { Serial.println("Connecting..."); };
HeadlessWiFiSettings.onSuccess = []() { Serial.println("Connected!"); };
HeadlessWiFiSettings.onFailure = []() { Serial.println("Failed!"); };
HeadlessWiFiSettings.onConfigSaved = []() { Serial.println("Config saved!"); };
```

**Expected Behavior**:
- Improv provisioning triggers callbacks in this order:
  1. `onConfigSaved()` - when Improv saves to files
  2. `onConnect()` - when connect() starts
  3. `onSuccess()` OR `onFailure()` - based on result

**Test**:
- [ ] All callbacks fire in correct order
- [ ] Callbacks fire whether provisioned via Improv or JSON
- [ ] Callbacks receive correct context

---

### Scenario 8: Re-provisioning

**Setup**: Device already provisioned and connected

**Steps**:
1. Device connected to WiFi A
2. Send credentials for WiFi B via Improv
3. Observe behavior

**Expected Behavior**:
- New credentials overwrite old ones
- `connect(false)` disconnects from WiFi A
- Attempts to connect to WiFi B
- On success, device on new network

**Test**:
- [ ] Device disconnects from old network
- [ ] Device connects to new network
- [ ] Old credentials fully replaced
- [ ] Process works multiple times

---

## Testing Without Hardware (Code Review)

If you don't have ESP32 hardware, you can still verify:

### Code Integration Checks

- [x] `#include <ImprovWiFi.h>` added to header
- [x] `ImprovWiFi* improv = nullptr;` member variable
- [x] Proper memory management (deletes old instance before creating new)
- [x] Uses same files as JSON endpoints (`/wifi-ssid`, `/wifi-password`)
- [x] Calls existing `connect()` method
- [x] Uses existing callback mechanism

### Potential Issues Found

#### ⚠️ Issue 1: Portal Mode Conflict
**File**: src/HeadlessWiFiSettings.cpp:649-651

```cpp
if (ssid.length() == 0) {
    Serial.println(F("First contact!\n"));
    this->portal();  // Never returns!
}
```

**Problem**: If no WiFi configured, `connect()` calls `portal()` which runs forever. The example calls `connect()` in setup(), so `loop()` never runs, and `serialImprovLoop()` never gets called.

**Impact**: Can't provision via Improv on fresh device if example code calls `connect()` in setup.

**Recommendation**: Example should:
- Call `connect(false)` to skip portal, OR
- Handle the case differently, OR
- Document that Improv provisioning must happen BEFORE first `connect()` call

#### ⚠️ Issue 2: No Portal Fallback on Improv Failure
**File**: src/HeadlessWiFiSettings.cpp:352-356

```cpp
improv->setWiFiCallback([this](const char* ssid, const char* password) {
    spurt("/wifi-ssid", ssid);
    spurt("/wifi-password", password);
    if (onConfigSaved) onConfigSaved();
    connect(false);  // <-- false = no portal on failure
});
```

**Problem**: If user sends wrong credentials via Improv, connection fails but portal doesn't start (because `connect(false)`). Device has no HTTP interface, only Improv.

**Impact**: User must re-provision via Improv; can't fall back to JSON endpoints.

**This might be intentional**: Keeps device in "Improv mode" until successful.

**Recommendation**: Document this behavior, or provide callback for user to handle.

#### ✅ Good: Storage Integration
Files match perfectly:
- Improv writes: `/wifi-ssid`, `/wifi-password` (line 353-354)
- connect() reads: `/wifi-ssid`, `/wifi-password` (line 647-648)

#### ✅ Good: Callback Integration
- Uses existing `onConfigSaved()` callback
- Reuses `connect()` which triggers `onConnect`, `onSuccess`, `onFailure`

#### ✅ Good: Memory Management
```cpp
if (improv) {
    delete improv;
    improv = nullptr;
}
```

---

## Quick Hardware Test

If you have an ESP32, run this minimal test:

### Test 1: Improv Provisioning

```cpp
#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>

void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);

    HeadlessWiFiSettings.onConfigSaved = []() {
        Serial.println("✓ Config saved via Improv");
    };

    HeadlessWiFiSettings.onSuccess = []() {
        Serial.println("✓ WiFi connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    };

    HeadlessWiFiSettings.beginSerialImprov("TestDevice", "1.0");

    // Don't call connect() yet - let user provision via Improv first
    Serial.println("Ready for Improv provisioning...");
}

void loop() {
    HeadlessWiFiSettings.serialImprovLoop();
    delay(10);
}
```

**Test Steps**:
1. Flash this sketch
2. Run: `python3 test_improv.py /dev/ttyUSB0 "YourSSID" "YourPassword"`
3. Watch serial monitor

**Expected Output**:
```
Ready for Improv provisioning...
✓ Config saved via Improv
Connecting to WiFi SSID 'YourSSID'....
✓ WiFi connected!
IP: 192.168.1.XXX
```

**Success Criteria**:
- [ ] Device receives Improv command
- [ ] Config saved callback fires
- [ ] Device connects to WiFi
- [ ] IP address obtained

---

## Summary

### What Works ✅
- Shared storage between Improv and JSON endpoints
- Reuses existing WiFi connection logic
- Proper memory management
- Callback integration
- Dependency correctly added

### Concerns ⚠️
1. **Portal mode blocks Improv**: Fresh device calls portal() which never returns
2. **No fallback on failure**: `connect(false)` means no HTTP access if Improv creds fail
3. **Minimal example**: Doesn't show real-world usage with HTTP endpoints + custom params

### Recommendations
1. Update example to show proper integration with HTTP endpoints
2. Document portal mode behavior with fresh devices
3. Show how to handle connection failures
4. Add example with custom parameters + Improv

### Testing Verdict

**Can approve PR if**:
- Code compiles ✅
- Basic Improv → WiFi flow works ✅ (needs hardware verification)
- Documentation updated ✅ (README has example)

**Should request changes if**:
- Example code improved to show HTTP + Improv together
- Portal mode conflict documented or fixed
- Failure handling example provided
