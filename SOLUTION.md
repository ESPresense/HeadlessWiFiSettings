# Build Solution: WiFi Library Conflict

## Root Cause

The Improv WiFi Library (`jnthas/Improv-WiFi-Library`) declares `depends=WiFi` in its `library.properties`:

```
depends=WiFi
```

This causes PlatformIO to install the **Arduino WiFi shield library** package, which is incompatible with ESP32. ESP32 uses its own built-in WiFi library that's part of the Arduino-ESP32 framework.

### The Conflict

| Library | Purpose | API Signature |
|---------|---------|---------------|
| Arduino WiFi (shield) | External WiFi shields | `WiFi.begin(char* ssid, ...)` |
| ESP32 WiFi (built-in) | ESP32's integrated WiFi | `WiFi.begin(const char* ssid, ...)` |

The Improv library code uses `#include <WiFi.h>` expecting ESP32's built-in WiFi, but PlatformIO installs the Arduino shield library instead, causing compilation errors:

```cpp
error: invalid conversion from 'const char*' to 'char*'
error: 'WIFI_AUTH_OPEN' was not declared in this scope
```

## Solution

Include the Improv WiFi Library source **directly in the example's `lib/` folder** instead of using PlatformIO's library dependency resolution.

### Implementation

```
examples/
├── ImprovWithHTTP/
│   ├── lib/
│   │   └── ImprovWiFiLibrary/    ← Library source copied here
│   │       ├── ImprovTypes.h
│   │       ├── ImprovWiFiLibrary.h
│   │       └── ImprovWiFiLibrary.cpp
│   ├── ImprovWithHTTP.ino
│   └── platformio.ini
```

### platformio.ini

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps =
    file://../../                                       # HeadlessWiFiSettings
    https://github.com/me-no-dev/ESPAsyncWebServer.git
    https://github.com/me-no-dev/AsyncTCP.git
    # Note: ImprovWiFiLibrary is in lib/ folder
```

This way:
- ✅ Improv library compiles with ESP32's built-in WiFi
- ✅ No WiFi dependency resolution conflict
- ✅ DNSServer finds WiFiUdp.h correctly
- ✅ All libraries use the same WiFi API

## Testing

```bash
cd examples/ImprovWithHTTP
pio run -t clean
pio run
```

Should compile without errors.

## Why This Wasn't Obvious

The original PR specified:
```json
"name": "judge2005/ImprovWiFi"
```

This library doesn't exist. The correct library is `jnthas/Improv WiFi Library`, but it has the `depends=WiFi` issue that requires this workaround.

## Alternative Solutions Tried

1. ❌ `lib_ignore = WiFi` - Breaks DNSServer which needs WiFiUdp.h
2. ❌ Using registry name `jnthas/Improv WiFi Library` - Still pulls WiFi dependency
3. ❌ Using GitHub URL directly - PlatformIO still processes library.properties
4. ✅ **Local copy in lib/ folder** - Bypasses dependency resolution entirely

## For the PR Author

The PR should either:
1. Use this local copy approach in examples
2. OR fork the Improv library and remove `depends=WiFi` from library.properties
3. OR document that users must manually handle the WiFi dependency conflict

The integration code itself is correct - it's purely a dependency resolution issue.
