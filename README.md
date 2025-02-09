# Headless WiFi configuration manager for the ESP32 platform in the Arduino framework

## Description

This is a headless version of the WiFi configuration manager for ESP32 programs written in the Arduino framework. It provides JSON endpoints for configuration without a web UI. The library allows you to configure WiFi settings and custom parameters through HTTP endpoints.

The configuration is stored in files in the flash filesystem of the ESP. Debug output is written to `Serial`.

Only automatic IP address assignment (DHCP) is supported.

## Examples

### Minimal usage

```C++
#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>

void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);  // On first run, will format after failing to mount

    HeadlessWiFiSettings.connect();
}

void loop() {
    ...
}
```

### Callbacks and custom variables

```C++
void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);  // On first run, will format after failing to mount

    // Note that these examples call functions that you probably don't have.
    HeadlessWiFiSettings.onSuccess  = []() { green(); };
    HeadlessWiFiSettings.onFailure  = []() { red(); };
    HeadlessWiFiSettings.onWaitLoop = []() { blue(); return 30; };  // delay 30 ms
    HeadlessWiFiSettings.onPortalWaitLoop = []() { blink(); };

    String host = HeadlessWiFiSettings.string("server_host", "default.example.org");
    int    port = HeadlessWiFiSettings.integer("server_port", 0, 65535, 443);

    HeadlessWiFiSettings.connect(true, 30);
}
```

## JSON Endpoints

The library provides two main endpoints for configuration:

### /settings

This endpoint handles the primary configuration parameters.

- GET: Returns a JSON object containing all primary parameters
- POST: Updates primary parameters. Send parameters as form data.

Example response:
```json
{
    "server_host": "example.org",
    "server_port": "443"
}
```

### /extras

This endpoint handles additional parameters marked with `markExtra()`.

- GET: Returns a JSON object containing all extra parameters
- POST: Updates extra parameters. Send parameters as form data.

Example response:
```json
{
    "custom_param": "value",
    "another_param": "123"
}
```

## Installing

Automated installation:
* [Instructions for Arduino Library Manager](https://www.arduino.cc/en/guide/libraries)
* [Instructions for PlatformIO Library Manager](https://platformio.org/lib/show/7251/esp32-HeadlessWiFiSettings/installation)

Getting the source for manual installation:
* `git clone https://github.com/ESPresense/HeadlessWiFiSettings`
* [.zip and .tar.gz files](https://github.com/ESPresense/HeadlessWiFiSettings/releases)

## Reference

This library uses a singleton instance (object), `HeadlessWiFiSettings`, and is not
designed to be inherited from (subclassed), or to have multiple instances.

### Functions

#### HeadlessWiFiSettings.connect([...])

```C++
bool connect(bool portal = true, int wait_seconds = 60);
```

If no WiFi network is configured yet, starts the configuration portal.
In other cases, it will attempt to connect to the network in station (WiFi
client) mode, and wait until either a connection is established, or
`wait_seconds` has elapsed. Returns `true` if connection succeeded.

By default, a failed connection (no connection established within the timeout)
will cause the configuration portal to be started. Given `portal = false`, it
will instead return `false`.

To wait forever until WiFi is connected, use `wait_seconds = -1`. In this case,
the value of `portal` is ignored.

#### HeadlessWiFiSettings.portal()

```C++
void portal();
```

Disconnects any active WiFi and turns the ESP into an access point that serves the configuration endpoints.

This function never ends. A restart is required to resume normal operation.

#### HeadlessWiFiSettings.integer(...)
#### HeadlessWiFiSettings.string(...)
#### HeadlessWiFiSettings.checkbox(...)

```C++
int integer(String name, [long min, long max,] int init = 0, String label = name);
String string(String name, [[unsigned int min_length,] unsigned int max_length,] String init = "", String label = name);
bool checkbox(String name, bool init = false, String label = name);
```

Configures a custom configurable option and returns the current value. When no
value (or an empty string) is configured, the value given as `init` is returned.

These functions should be called *before* calling `.connect()` or `.portal()`.

The `name` is used as the filename in the SPIFFS, and as a parameter name in the JSON endpoints.

Some restrictions for the values can be given. For integers, a range can be specified by supplying both `min` and `max`. For strings, a maximum length can be specified as `max_length`. A minimum string length can be set with `min_length`, effectively making the field mandatory: it can no longer be left empty to get the `init` value.

### Variables

Note: because of the way this library is designed, any assignment to the
member variables should be done *before* calling any of the functions.

#### HeadlessWiFiSettings.hostname

```C++
String
```

Name to use as the hostname and SSID for the access point. By default, this is
set to "esp32-" or "esp8266-", depending on the platform.

If it ends in a `-` character, a unique 6 digit device identifier is added automatically.

#### HeadlessWiFiSettings.password

```C++
String
```

This variable is used to protect the configuration portal's softAP. When no
password is explicitly assigned before the first custom configuration parameter
is defined, a password will be automatically generated.

#### HeadlessWiFiSettings.secure

```C++
bool
```

By setting this to `true`, before any custom configuration parameter is defined,
secure mode will be forced, instead of the default behavior.

## History

This was forked from https://github.com/Juerd/ESP-WiFiSettings when it was converted to use AsyncWebServer instead of WebServer. This version removes the web UI in favor of JSON endpoints.
