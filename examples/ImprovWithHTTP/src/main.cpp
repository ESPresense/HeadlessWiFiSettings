/*
 * SerialImprov + HTTP Endpoints Integration Example
 *
 * This example demonstrates how to use BOTH SerialImprov and HTTP JSON endpoints
 * together, showing proper integration and handling of edge cases.
 *
 * Features:
 * - SerialImprov for USB provisioning
 * - HTTP JSON endpoints for configuration
 * - Custom parameters
 * - Proper callback handling
 * - Graceful handling of connection failures
 */

#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>

// Custom configuration
String mqttServer;
int mqttPort;
bool enableLED;

void setup() {
    Serial.begin(115200);
    delay(1000);  // Give serial time to initialize

    Serial.println("\n\n=== SerialImprov + HTTP Integration Example ===\n");

    if (!SPIFFS.begin(true)) {
        Serial.println("ERROR: SPIFFS initialization failed!");
        return;
    }

    // Set hostname (will append unique ID if ends with -)
    HeadlessWiFiSettings.hostname = "esp32-improv-";

    // Setup callbacks for visibility
    HeadlessWiFiSettings.onConfigSaved = []() {
        Serial.println("✓ Configuration saved (WiFi credentials updated)");
    };

    HeadlessWiFiSettings.onConnect = []() {
        Serial.print("→ Attempting WiFi connection");
    };

    HeadlessWiFiSettings.onSuccess = []() {
        Serial.println(" ✓ Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.println("\nHTTP JSON endpoints available:");
        Serial.println("  GET/POST http://" + WiFi.localIP().toString() + "/wifi/main");
        Serial.println("  GET      http://" + WiFi.localIP().toString() + "/wifi/scan");
    };

    HeadlessWiFiSettings.onFailure = []() {
        Serial.println(" ✗ Connection failed!");
        Serial.println("Device can be re-provisioned via SerialImprov");
    };

    // Define custom parameters BEFORE beginSerialImprov
    mqttServer = HeadlessWiFiSettings.string("mqtt_server", "mqtt.example.com", "MQTT Server");
    mqttPort = HeadlessWiFiSettings.integer("mqtt_port", 1, 65535, 1883, "MQTT Port");
    enableLED = HeadlessWiFiSettings.checkbox("enable_led", true, "Enable LED");

    // Initialize SerialImprov
    Serial.println("Initializing SerialImprov...");
    HeadlessWiFiSettings.beginSerialImprov(
        "HeadlessWiFiSettings",  // Firmware name
        "1.0",                   // Version
        ""                       // Device name (empty = use hostname)
    );

    // Try to connect to WiFi
    // Use connect(false, 30) to:
    // - NOT start portal on failure (keeps device in Improv mode)
    // - Wait 30 seconds for connection
    Serial.println("Checking for existing WiFi configuration...");
    bool connected = HeadlessWiFiSettings.connect(false, 30);

    if (connected) {
        // WiFi connected - start HTTP server for JSON endpoints
        HeadlessWiFiSettings.httpSetup();

        Serial.println("\n=== Device Ready ===");
        Serial.println("Mode: WiFi Connected + SerialImprov Active");
        Serial.println("\nCurrent Configuration:");
        Serial.print("  MQTT Server: ");
        Serial.println(mqttServer);
        Serial.print("  MQTT Port: ");
        Serial.println(mqttPort);
        Serial.print("  LED Enabled: ");
        Serial.println(enableLED ? "Yes" : "No");
    } else {
        Serial.println("\n=== Device Ready (No WiFi) ===");
        Serial.println("Mode: SerialImprov Only");
        Serial.println("Waiting for provisioning via:");
        Serial.println("  1. Home Assistant (USB auto-discovery)");
        Serial.println("  2. Web browser: https://www.improv-wifi.com/");
        Serial.println("  3. Python script: test_improv.py");
    }

    Serial.println();
}

void loop() {
    // IMPORTANT: Always call serialImprovLoop() in loop
    // This allows re-provisioning at any time
    HeadlessWiFiSettings.serialImprovLoop();

    // Your application code here
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 10000) {  // Every 10 seconds
        lastCheck = millis();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("✓ WiFi OK (");
            Serial.print(WiFi.RSSI());
            Serial.println(" dBm)");
        } else {
            Serial.println("✗ WiFi disconnected - awaiting Improv provisioning");
        }
    }

    delay(10);
}
