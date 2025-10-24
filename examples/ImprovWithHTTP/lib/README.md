# Local Libraries

## ImprovWiFiLibrary

This folder contains a local copy of the Improv WiFi Library source code.

### Why Local?

The Improv WiFi Library declares `depends=WiFi` in its `library.properties` file. This causes PlatformIO's dependency resolver to install the **Arduino WiFi shield library** instead of using ESP32's built-in WiFi library. These are incompatible:

- Arduino WiFi library: For WiFi shields (different API)
- ESP32 WiFi library: Built into ESP32 Arduino framework

By including the library source directly in the `lib/` folder, we bypass PlatformIO's dependency resolution and avoid this conflict.

### Source

Copied from: https://github.com/jnthas/Improv-WiFi-Library
Version: 0.0.3
License: See https://github.com/jnthas/Improv-WiFi-Library/blob/main/LICENSE
