/*
 * Example sketch demonstrating HeadlessWiFiSettings with SerialImprov
 *
 * This example shows how to use Home Assistant's SerialImprov protocol
 * for WiFi provisioning alongside the existing JSON HTTP endpoints.
 *
 * Features:
 * - WiFi provisioning via USB (SerialImprov protocol)
 * - Compatible with Home Assistant auto-discovery
 * - Existing JSON endpoints still available
 * - LED feedback for identify command
 *
 * Hardware:
 * - ESP32 board
 * - Optional: LED on GPIO 2 (built-in on most boards)
 *
 * Testing:
 * 1. Upload this sketch to ESP32
 * 2. Connect ESP32 to computer running Home Assistant via USB
 * 3. In Home Assistant, go to Settings → Devices & Services
 * 4. Device should appear for provisioning with "Improv via Serial"
 * 5. Enter WiFi credentials through Home Assistant UI
 *
 * Alternatively, use the web browser:
 * 1. Visit https://www.improv-wifi.com/
 * 2. Click "Connect to device" and select ESP32 serial port
 * 3. Follow provisioning wizard
 */

#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>

#define LED_PIN 2  // Built-in LED on most ESP32 boards

// Custom configuration parameters
String mqttServer;
int mqttPort;
String deviceName;

void blinkLED(int times = 3, int delayMs = 100) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialize filesystem
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed!");
        return;
    }

    Serial.println("\n\n=== HeadlessWiFiSettings with SerialImprov Example ===\n");

    // Configure WiFi Settings
    HeadlessWiFiSettings.hostname = "esp32-improv-";  // Will append unique ID

    // Callbacks for visual feedback
    HeadlessWiFiSettings.onSuccess = []() {
        Serial.println("✓ Connected to WiFi successfully!");
        blinkLED(5, 100);  // Fast blinks on success
    };

    HeadlessWiFiSettings.onFailure = []() {
        Serial.println("✗ Failed to connect to WiFi");
        blinkLED(3, 500);  // Slow blinks on failure
    };

    HeadlessWiFiSettings.onConnect = []() {
        Serial.println("Attempting WiFi connection...");
        digitalWrite(LED_PIN, HIGH);  // LED on while connecting
    };

    HeadlessWiFiSettings.onPortalWaitLoop = []() {
        // Blink slowly while in portal mode
        static bool ledState = false;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        return 500;  // 500ms delay
    };

    // This callback is called when SerialImprov IDENTIFY command is received
    // Useful for helping users identify which physical device they're configuring
    HeadlessWiFiSettings.onImprovIdentify = []() {
        Serial.println("SerialImprov: IDENTIFY command received");
        blinkLED(10, 50);  // Rapid blinking for identification
    };

    // Define custom configuration parameters
    // These will be available via both SerialImprov and JSON endpoints
    mqttServer = HeadlessWiFiSettings.string("mqtt_server", "mqtt.example.com", "MQTT Server");
    mqttPort = HeadlessWiFiSettings.integer("mqtt_port", 1, 65535, 1883, "MQTT Port");
    deviceName = HeadlessWiFiSettings.string("device_name", "My ESP32 Device", "Device Name");

    // Attempt to connect to WiFi
    // This will:
    // 1. Check if WiFi credentials exist
    // 2. If not, start portal mode (accessible via JSON endpoints)
    // 3. If yes, connect to WiFi
    // 4. On failure, start portal mode
    bool connected = HeadlessWiFiSettings.connect(
        true,   // Start portal on failure
        30      // Wait 30 seconds for connection
    );

    if (connected) {
        Serial.println("\n=== WiFi Connected ===");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Hostname: ");
        Serial.println(HeadlessWiFiSettings.hostname);

        Serial.println("\n=== Configuration ===");
        Serial.print("MQTT Server: ");
        Serial.println(mqttServer);
        Serial.print("MQTT Port: ");
        Serial.println(mqttPort);
        Serial.print("Device Name: ");
        Serial.println(deviceName);

        Serial.println("\n=== JSON Endpoints Available ===");
        Serial.println("GET/POST http://" + WiFi.localIP().toString() + "/wifi/main");
        Serial.println("GET http://" + WiFi.localIP().toString() + "/wifi/scan");

        // Setup HTTP server for JSON endpoints
        HeadlessWiFiSettings.httpSetup();

        digitalWrite(LED_PIN, LOW);
    }

    Serial.println("\n=== SerialImprov Active ===");
    Serial.println("Device can be provisioned via:");
    Serial.println("1. Home Assistant (auto-discovery via USB)");
    Serial.println("2. Web browser at https://www.improv-wifi.com/");
    Serial.println("3. Any SerialImprov-compatible client");
    Serial.println();
}

void loop() {
    // Your application code here

    // The SerialImprov protocol handler runs in the background
    // and will process incoming serial commands automatically

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 10000) {
        lastPrint = millis();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.print(".");
        } else {
            Serial.println("WiFi disconnected!");
        }
    }

    delay(10);
}
