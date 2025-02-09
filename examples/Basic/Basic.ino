#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>

void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);  // On first run, will format after failing to mount

    // Basic WiFi settings go to /wifi/main
    String ssid = HeadlessWiFiSettings.string("ssid", "");
    String password = HeadlessWiFiSettings.pstring("password", "");

    // Network settings go to /wifi/network
    HeadlessWiFiSettings.markEndpoint("network");
    String hostname = HeadlessWiFiSettings.string("hostname", "myesp");
    bool dhcp = HeadlessWiFiSettings.checkbox("dhcp", true);
    String ip = HeadlessWiFiSettings.string("ip", "192.168.1.100");

    // API settings go to /wifi/api
    HeadlessWiFiSettings.markEndpoint("api");
    String apiKey = HeadlessWiFiSettings.pstring("key", "");
    String apiEndpoint = HeadlessWiFiSettings.string("endpoint", "https://api.example.com");

    // Extra settings go to /wifi/extra
    HeadlessWiFiSettings.markExtra();
    int refreshInterval = HeadlessWiFiSettings.integer("refresh", 60);

    // The ESP will create an access point with a random password
    // Connect to it and use these endpoints to configure:
    //
    // WiFi Scan:
    // GET /wifi/scan - Get list of available networks with signal strengths
    //
    // Main endpoints:
    // GET /wifi/main - Get main WiFi settings
    // POST /wifi/main - Update WiFi settings
    //
    // Network endpoints:
    // GET /wifi/network - Get network settings
    // POST /wifi/network - Update network settings
    //
    // API endpoints:
    // GET /wifi/api - Get API settings
    // POST /wifi/api - Update API settings
    //
    // Extra endpoints:
    // GET /wifi/extra - Get extra settings
    // POST /wifi/extra - Update extra settings
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
