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

### Serial Improv provisioning

HeadlessWiFiSettings can receive WiFi credentials over the [Improv Wi-Fi serial protocol](https://improv-wifi.com/).
Initialise the serial handler and call the loop handler from your main loop:

```C++
void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);
    HeadlessWiFiSettings.startImprovSerial("HeadlessWiFiSettings", "1.0");
    HeadlessWiFiSettings.connect();
}

void loop() {
    HeadlessWiFiSettings.loop();
}
```

To exercise the Serial Improv protocol over USB without a GUI, install `pyserial`
(`python3 -m pip install --user pyserial`) and run the helper script:

```bash
python3 examples/SerialImprov/test_improv.py /dev/cu.usbserial-54F70151721 MyWiFi MyPassword
```

Leave the SSID/password arguments off if you only want to send IDENTIFY/GET
STATE RPCs.

## WiFi Configuration

### Initial Setup

When the device first starts without any stored WiFi credentials, it will automatically enter portal mode. In portal mode:

1. The ESP32 creates an access point with the SSID specified in `HeadlessWiFiSettings.hostname` (default: `esp32-XXXXXX` where XXXXXX is a unique device ID)
2. A captive portal is started on port 80
3. DNS server redirects all requests to the portal's IP address

### Configuring WiFi via HTTP Endpoints

Connect to the ESP32's access point and use the endpoints below to configure WiFi:

```bash
# Scan for available networks
curl http://192.168.4.1/wifi/scan

# Get current WiFi settings
curl http://192.168.4.1/wifi/main

# Configure WiFi credentials
curl -X POST http://192.168.4.1/wifi \
  -d "wifi-ssid=YourNetworkName" \
  -d "wifi-password=YourPassword"
```

### Configuring WiFi via Serial Improv

The library supports the [Improv Wi-Fi serial protocol](https://improv-wifi.com/) for provisioning over USB serial:

```C++
HeadlessWiFiSettings.startImprovSerial("MyDevice", "1.0");
```

You can then use any Improv-compatible tool or the included Python script:

```bash
python3 examples/SerialImprov/test_improv.py /dev/ttyUSB0 MyWiFi MyPassword
```

### Multiple Endpoint Configuration

You can organize parameters into different endpoints using `markEndpoint()`:

```C++
// Default "main" endpoint
String host = HeadlessWiFiSettings.string("server_host", "example.org");
int port = HeadlessWiFiSettings.integer("server_port", 443);

// Switch to "mqtt" endpoint
HeadlessWiFiSettings.markEndpoint("mqtt");
String mqttHost = HeadlessWiFiSettings.string("mqtt_host", "mqtt.example.org");
int mqttPort = HeadlessWiFiSettings.integer("mqtt_port", 1883);

// Legacy "extras" endpoint (for backward compatibility)
HeadlessWiFiSettings.markExtra();
String otherParam = HeadlessWiFiSettings.string("other_param", "value");
```

## HTTP API Reference

All endpoints are served over HTTP on port 80. The API returns JSON responses and accepts form-encoded POST data.

### GET /wifi/scan

Scans for available WiFi networks and returns their SSIDs and signal strengths.

**Response:**
```json
{
  "networks": {
    "NetworkName1": -45,
    "NetworkName2": -67,
    "NetworkName3": -82
  }
}
```

Signal strength values are in dBm (negative numbers, closer to 0 is stronger). Duplicate SSIDs are merged with the strongest signal retained.

### GET /wifi or GET /wifi/main

Returns the current values and defaults for the main parameter endpoint.

**Response:**
```json
{
  "values": {
    "wifi-ssid": "CurrentNetwork",
    "wifi-password": "***###***",
    "server_host": "example.org",
    "server_port": 443
  },
  "defaults": {
    "server_host": "default.example.org",
    "server_port": 443
  }
}
```

Note: Password fields always return the masked value `***###***` for security.

### GET /wifi/{endpoint}

Returns values and defaults for a specific endpoint (e.g., `/wifi/mqtt`, `/wifi/extras`).

**Response format:** Same as `/wifi/main`

### POST /wifi or POST /wifi/main

Updates configuration parameters on the main endpoint.

**Request:** Send parameters as form-encoded data:
```
POST /wifi
Content-Type: application/x-www-form-urlencoded

wifi-ssid=MyNetwork&wifi-password=MyPassword&server_host=api.example.com&server_port=8080
```

**Response:**
- `200 OK` - Configuration saved successfully
- `500 Internal Server Error` - Error writing to flash filesystem
- `404 Not Found` - Endpoint doesn't exist

After a successful save, the `onConfigSaved` callback is triggered.

### POST /wifi/{endpoint}

Updates configuration parameters for a specific endpoint.

**Request format:** Same as POST /wifi/main

### GET /wifi/options/{parameter_name}

Returns available options for dropdown parameters.

**Example:** `GET /wifi/options/log_level`

**Response:**
```json
["debug", "info", "warning", "error"]
```

**Error Response:**
- `404 Not Found` - Parameter not found or not a dropdown type

## Complete Usage Example

Here's a comprehensive example demonstrating most features:

```C++
#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>
#include <ESPAsyncWebServer.h>

void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);

    // Configure hostname
    HeadlessWiFiSettings.hostname = "my-device-";  // Will append device ID

    // Set up callbacks
    HeadlessWiFiSettings.onConnect = []() {
        Serial.println("Connecting to WiFi...");
    };

    HeadlessWiFiSettings.onSuccess = []() {
        Serial.println("WiFi connected!");
        Serial.println(WiFi.localIP());
    };

    HeadlessWiFiSettings.onFailure = []() {
        Serial.println("WiFi connection failed");
    };

    HeadlessWiFiSettings.onConfigSaved = []() {
        Serial.println("Configuration saved to flash");
    };

    // Define main configuration parameters
    String serverHost = HeadlessWiFiSettings.string("server_host", "api.example.com", "Server Host");
    int serverPort = HeadlessWiFiSettings.integer("server_port", 1, 65535, 443, "Server Port");
    bool useTLS = HeadlessWiFiSettings.checkbox("use_tls", true, "Use TLS");

    // Define MQTT parameters in separate endpoint
    HeadlessWiFiSettings.markEndpoint("mqtt");
    String mqttBroker = HeadlessWiFiSettings.string("mqtt_broker", "mqtt.local", "MQTT Broker");
    int mqttPort = HeadlessWiFiSettings.integer("mqtt_port", 1883, "MQTT Port");
    String mqttUser = HeadlessWiFiSettings.string("mqtt_user", "", "MQTT Username");
    String mqttPass = HeadlessWiFiSettings.pstring("mqtt_pass", "", "MQTT Password");

    // Define advanced parameters
    HeadlessWiFiSettings.markExtra();
    std::vector<String> logLevels = {"debug", "info", "warning", "error"};
    int logLevel = HeadlessWiFiSettings.dropdown("log_level", logLevels, 1, "Log Level");
    float updateInterval = HeadlessWiFiSettings.floating("update_interval", 0.1, 60.0, 5.0, "Update Interval (s)");

    // Add custom HTTP endpoints
    HeadlessWiFiSettings.onHttpSetup = [](AsyncWebServer* server) {
        server->on("/status", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "application/json", "{\"status\":\"ok\",\"uptime\":" + String(millis()) + "}");
        });

        server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "text/html",
                "<h1>My Device</h1>"
                "<p>Configure via <a href='/wifi/main'>/wifi/main</a></p>"
                "<p>MQTT settings at <a href='/wifi/mqtt'>/wifi/mqtt</a></p>"
            );
        });
    };

    // Enable Serial Improv for USB provisioning
    HeadlessWiFiSettings.startImprovSerial("MyDevice", "1.0.0");

    // Connect to WiFi (30 second timeout, show portal on failure)
    HeadlessWiFiSettings.connect(true, 30);

    // Now use the configured values
    Serial.printf("Server: %s:%d (TLS: %s)\n",
        serverHost.c_str(), serverPort, useTLS ? "yes" : "no");
    Serial.printf("MQTT: %s:%d\n", mqttBroker.c_str(), mqttPort);
    Serial.printf("Log level: %s\n", logLevels[logLevel].c_str());
}

void loop() {
    // Process Serial Improv messages
    HeadlessWiFiSettings.loop();

    // Your application code here
    delay(100);
}
```

### Testing the API

Once the device is running, you can interact with it using curl or any HTTP client:

```bash
# Check status
curl http://my-device-123456.local/status

# Get main configuration
curl http://my-device-123456.local/wifi/main

# Update server settings
curl -X POST http://my-device-123456.local/wifi \
  -d "server_host=newserver.com" \
  -d "server_port=8443" \
  -d "use_tls=1"

# Get MQTT configuration
curl http://my-device-123456.local/wifi/mqtt

# Update MQTT credentials
curl -X POST http://my-device-123456.local/wifi/mqtt \
  -d "mqtt_user=myuser" \
  -d "mqtt_pass=mypassword"

# Get dropdown options
curl http://my-device-123456.local/wifi/options/log_level

# Scan for WiFi networks
curl http://my-device-123456.local/wifi/scan
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

#### HeadlessWiFiSettings.string(...)

```C++
String string(String name, String init = "", String label = name);
String string(String name, unsigned int max_length, String init = "", String label = name);
String string(String name, unsigned int min_length, unsigned int max_length, String init = "", String label = name);
```

Configures a custom string parameter and returns the current value. When no value is configured, the `init` value is returned.

#### HeadlessWiFiSettings.pstring(...)

```C++
String pstring(String name, String init = "", String label = name);
```

Like `string()`, but for password fields. Values are masked in JSON responses as `***###***`.

#### HeadlessWiFiSettings.integer(...)

```C++
long integer(String name, long init = 0, String label = name);
long integer(String name, long min, long max, long init = 0, String label = name);
```

Configures a custom integer parameter with optional min/max constraints.

#### HeadlessWiFiSettings.floating(...)

```C++
float floating(String name, float init = 0, String label = name);
float floating(String name, long min, long max, float init = 0, String label = name);
```

Configures a custom floating-point parameter with optional min/max constraints.

#### HeadlessWiFiSettings.checkbox(...)

```C++
bool checkbox(String name, bool init = false, String label = name);
```

Configures a boolean checkbox parameter.

#### HeadlessWiFiSettings.dropdown(...)

```C++
long dropdown(String name, std::vector<String> options, long init = 0, String label = name);
```

Configures a dropdown parameter with predefined options. Returns the index of the selected option. The available options can be retrieved via `/wifi/options/{name}`.

**Note:** All parameter configuration functions should be called *before* calling `.connect()` or `.portal()`.

The `name` is used as the filename in SPIFFS and as the parameter name in JSON endpoints.

#### HeadlessWiFiSettings.markEndpoint(...)

```C++
void markEndpoint(String name);
```

Switches to a named endpoint. All subsequent parameter definitions will be grouped under this endpoint and accessible via `/wifi/{name}`.

```C++
// Parameters added to "main" endpoint
String host = HeadlessWiFiSettings.string("host", "example.org");

// Switch to custom endpoint
HeadlessWiFiSettings.markEndpoint("mqtt");
String mqttHost = HeadlessWiFiSettings.string("mqtt_host", "broker.local");
```

#### HeadlessWiFiSettings.markExtra()

```C++
void markExtra();
```

Convenience function that switches to the "extras" endpoint. Equivalent to `markEndpoint("extras")`.

#### HeadlessWiFiSettings.startImprovSerial(...)

```C++
void startImprovSerial(String firmware, String version, String chip = "");
```

Enables the Improv Wi-Fi serial protocol for provisioning over USB. The firmware name, version, and optionally chip family are advertised to provisioning tools.

#### HeadlessWiFiSettings.loop()

```C++
void loop();
```

Processes Improv serial protocol messages. Must be called repeatedly in your main `loop()` function if using Serial Improv.

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

### Callbacks

Callbacks can be assigned to customize behavior at various stages. All callbacks are optional.

#### HeadlessWiFiSettings.onHttpSetup

```C++
std::function<void(AsyncWebServer*)> onHttpSetup;
```

Called during HTTP server setup, before the server starts. Allows you to add custom routes or modify server configuration.

```C++
HeadlessWiFiSettings.onHttpSetup = [](AsyncWebServer* server) {
    server->on("/custom", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "Custom endpoint");
    });
};
```

#### HeadlessWiFiSettings.onConnect

```C++
std::function<void(void)> onConnect;
```

Called when attempting to connect to WiFi, before the connection starts.

#### HeadlessWiFiSettings.onSuccess

```C++
std::function<void(void)> onSuccess;
```

Called when WiFi connection is successful.

#### HeadlessWiFiSettings.onFailure

```C++
std::function<void(void)> onFailure;
```

Called when WiFi connection fails after the timeout period.

#### HeadlessWiFiSettings.onWaitLoop

```C++
std::function<int(void)> onWaitLoop;
```

Called repeatedly while waiting for WiFi connection. Return the delay in milliseconds until the next call (default: 100ms).

```C++
HeadlessWiFiSettings.onWaitLoop = []() {
    // Blink LED or update display
    return 50;  // Call again in 50ms
};
```

#### HeadlessWiFiSettings.onPortal

```C++
std::function<void(void)> onPortal;
```

Called when the configuration portal starts.

#### HeadlessWiFiSettings.onPortalView

```C++
std::function<void(void)> onPortalView;
```

Called when someone views the portal page (currently unused in headless mode).

#### HeadlessWiFiSettings.onPortalWaitLoop

```C++
std::function<int(void)> onPortalWaitLoop;
```

Called repeatedly while the portal is running. Return the delay in milliseconds until the next call.

#### HeadlessWiFiSettings.onConfigSaved

```C++
std::function<void(void)> onConfigSaved;
```

Called after configuration parameters are successfully saved to flash storage.

#### HeadlessWiFiSettings.onRestart

```C++
std::function<void(void)> onRestart;
```

Called before the device restarts (e.g., after receiving credentials via Serial Improv).

#### HeadlessWiFiSettings.onUserAgent

```C++
std::function<void(String&)> onUserAgent;
```

Called with the User-Agent string from HTTP requests (currently unused in headless mode).

## History

This was forked from https://github.com/Juerd/ESP-WiFiSettings when it was converted to use AsyncWebServer instead of WebServer. This version removes the web UI in favor of JSON endpoints.
