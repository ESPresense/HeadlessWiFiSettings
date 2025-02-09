#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>

void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);  // On first run, will format after failing to mount

    // Set a fixed hostname so we know where to connect
    HeadlessWiFiSettings.hostname = "esp-test";

    // Define some test settings
    HeadlessWiFiSettings.string("test_string", "default value");
    HeadlessWiFiSettings.integer("test_int", 0, 100, 42);
    HeadlessWiFiSettings.checkbox("test_bool", true);

    // Define some extra test settings
    HeadlessWiFiSettings.markExtra();
    HeadlessWiFiSettings.floating("test_float", 0, 10, 3.14);
    HeadlessWiFiSettings.string("test_extra", "extra value");

    // Connect to WiFi or start portal
    HeadlessWiFiSettings.connect();

    Serial.println("Test server running!");
    Serial.print("Connect to WiFi network: ");
    Serial.println(HeadlessWiFiSettings.hostname);
    if (HeadlessWiFiSettings.password.length()) {
        Serial.print("Password: ");
        Serial.println(HeadlessWiFiSettings.password);
    }
    Serial.print("Then visit: http://");
    Serial.println(WiFi.softAPIP());
}

void loop() {
    delay(1000);
}