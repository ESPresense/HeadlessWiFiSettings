#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>

void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);  // On first run, will format after failing to mount

    // The ESP will create an access point with a random password
    // Connect to it and use these endpoints to configure:
    // GET /settings - Get current WiFi settings
    // POST /settings - Update WiFi settings (send ssid and password as form data)
    HeadlessWiFiSettings.connect();

    // At this point, WiFi should be connected
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    // Your code here
    delay(1000);
}
