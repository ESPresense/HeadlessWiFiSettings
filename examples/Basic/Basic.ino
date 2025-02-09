#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>

void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);  // On first run, will format after failing to mount

    // Basic WiFi settings go to /settings/main (or legacy /settings endpoint)
    String ssid = HeadlessWiFiSettings.string("ssid", "");
    String password = HeadlessWiFiSettings.pstring("password", "");

    // Network settings go to /settings/network
    HeadlessWiFiSettings.markEndpoint("network");
    String hostname = HeadlessWiFiSettings.string("hostname", "myesp");
    bool dhcp = HeadlessWiFiSettings.checkbox("dhcp", true);
    String ip = HeadlessWiFiSettings.string("ip", "192.168.1.100");

    // API settings go to /settings/api
    HeadlessWiFiSettings.markEndpoint("api");
    String apiKey = HeadlessWiFiSettings.pstring("key", "");
    String apiEndpoint = HeadlessWiFiSettings.string("endpoint", "https://api.example.com");

    // Legacy extra settings still work with markExtra()
    HeadlessWiFiSettings.markExtra();
    int refreshInterval = HeadlessWiFiSettings.integer("refresh", 60);

    // The ESP will create an access point with a random password
    // Connect to it and use these endpoints to configure:
    // GET /settings - Get current WiFi settings (legacy, same as /settings/main)
    // POST /settings - Update WiFi settings (legacy, same as /settings/main)
    // GET /extras - Get extra settings (legacy, same as /settings/extra)
    // POST /extras - Update extra settings (legacy, same as /settings/extra)
    // GET /settings/main - Get main WiFi settings
    // POST /settings/main - Update main WiFi settings
    // GET /settings/network - Get network settings
    // POST /settings/network - Update network settings
    // GET /settings/api - Get API settings
    // POST /settings/api - Update API settings
    // GET /settings/extra - Get extra settings
    // POST /settings/extra - Update extra settings
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
