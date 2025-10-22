#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>

void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);
    HeadlessWiFiSettings.beginSerialImprov("HeadlessWiFiSettings", "1.0");
    HeadlessWiFiSettings.connect();
}

void loop() {
    HeadlessWiFiSettings.serialImprovLoop();
}

