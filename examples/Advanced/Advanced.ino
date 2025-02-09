#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>

// Callback function to blink LED while waiting for WiFi
int waitCallback() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    return 100; // Delay 100ms between blinks
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    SPIFFS.begin(true);  // On first run, will format after failing to mount

    // Set callbacks
    HeadlessWiFiSettings.onWaitLoop = waitCallback;
    HeadlessWiFiSettings.onSuccess = []() { digitalWrite(LED_BUILTIN, HIGH); };
    HeadlessWiFiSettings.onFailure = []() { digitalWrite(LED_BUILTIN, LOW); };

    // Define some primary settings (available at /settings endpoint)
    String mqtt_host = HeadlessWiFiSettings.string("mqtt_host", "mqtt.example.org");
    int mqtt_port = HeadlessWiFiSettings.integer("mqtt_port", 0, 65535, 1883);
    String mqtt_user = HeadlessWiFiSettings.string("mqtt_user", "");
    String mqtt_pass = HeadlessWiFiSettings.pstring("mqtt_pass", "");

    // Mark the following settings as extras (available at /extras endpoint)
    HeadlessWiFiSettings.markExtra();

    // Define some extra settings
    bool debug_mode = HeadlessWiFiSettings.checkbox("debug", false);
    float update_interval = HeadlessWiFiSettings.floating("update_interval", 0, 3600, 60.0);
    String device_name = HeadlessWiFiSettings.string("device_name", "esp-device");

    // Connect to WiFi or start portal if not configured
    if (HeadlessWiFiSettings.connect()) {
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // Print the configuration
        Serial.println("\nCurrent configuration:");
        Serial.println("Primary settings (/settings):");
        Serial.printf("MQTT Host: %s\n", mqtt_host.c_str());
        Serial.printf("MQTT Port: %d\n", mqtt_port);
        Serial.printf("MQTT User: %s\n", mqtt_user.c_str());
        Serial.println("MQTT Pass: ********");

        Serial.println("\nExtra settings (/extras):");
        Serial.printf("Debug Mode: %s\n", debug_mode ? "true" : "false");
        Serial.printf("Update Interval: %.1f seconds\n", update_interval);
        Serial.printf("Device Name: %s\n", device_name.c_str());
    }
}

void loop() {
    // Your code here
    delay(1000);
}

/* Example curl commands to interact with the endpoints:

Get current settings:
curl http://YOUR_ESP_IP/settings

Update settings:
curl -X POST -d "mqtt_host=test.mosquitto.org" -d "mqtt_port=1883" http://YOUR_ESP_IP/settings

Get extra settings:
curl http://YOUR_ESP_IP/extras

Update extra settings:
curl -X POST -d "debug=1" -d "update_interval=30.5" http://YOUR_ESP_IP/extras

*/
