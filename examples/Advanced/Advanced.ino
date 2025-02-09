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

    // WiFi settings in main endpoint (/settings/main or legacy /settings)
    String wifi_ssid = HeadlessWiFiSettings.string("wifi_ssid", "");
    String wifi_pass = HeadlessWiFiSettings.pstring("wifi_pass", "");

    // MQTT settings in their own endpoint (/settings/mqtt)
    HeadlessWiFiSettings.markEndpoint("mqtt");
    String mqtt_host = HeadlessWiFiSettings.string("host", "mqtt.example.org");
    int mqtt_port = HeadlessWiFiSettings.integer("port", 0, 65535, 1883);
    String mqtt_user = HeadlessWiFiSettings.string("user", "");
    String mqtt_pass = HeadlessWiFiSettings.pstring("pass", "");

    // Network settings in their own endpoint (/settings/network)
    HeadlessWiFiSettings.markEndpoint("network");
    String hostname = HeadlessWiFiSettings.string("hostname", "esp-device");
    bool dhcp = HeadlessWiFiSettings.checkbox("dhcp", true);
    String static_ip = HeadlessWiFiSettings.string("ip", "192.168.1.100");
    String gateway = HeadlessWiFiSettings.string("gateway", "192.168.1.1");
    String netmask = HeadlessWiFiSettings.string("netmask", "255.255.255.0");

    // Debug settings using legacy extras endpoint (/settings/extra or legacy /extras)
    HeadlessWiFiSettings.markExtra();
    bool debug_mode = HeadlessWiFiSettings.checkbox("debug", false);
    float update_interval = HeadlessWiFiSettings.floating("update_interval", 0, 3600, 60.0);
    std::vector<String> log_levels = {"error", "warn", "info", "debug"};
    int log_level = HeadlessWiFiSettings.dropdown("log_level", log_levels, 2);

    // Connect to WiFi or start portal if not configured
    if (HeadlessWiFiSettings.connect()) {
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // Print the configuration
        Serial.println("\nCurrent configuration:");

        Serial.println("\nWiFi settings (/settings/main):");
        Serial.printf("SSID: %s\n", wifi_ssid.c_str());
        Serial.println("Password: ********");

        Serial.println("\nMQTT settings (/settings/mqtt):");
        Serial.printf("Host: %s\n", mqtt_host.c_str());
        Serial.printf("Port: %d\n", mqtt_port);
        Serial.printf("User: %s\n", mqtt_user.c_str());
        Serial.println("Password: ********");

        Serial.println("\nNetwork settings (/settings/network):");
        Serial.printf("Hostname: %s\n", hostname.c_str());
        Serial.printf("DHCP: %s\n", dhcp ? "true" : "false");
        if (!dhcp) {
            Serial.printf("Static IP: %s\n", static_ip.c_str());
            Serial.printf("Gateway: %s\n", gateway.c_str());
            Serial.printf("Netmask: %s\n", netmask.c_str());
        }

        Serial.println("\nDebug settings (/settings/extra):");
        Serial.printf("Debug Mode: %s\n", debug_mode ? "true" : "false");
        Serial.printf("Update Interval: %.1f seconds\n", update_interval);
        Serial.printf("Log Level: %s\n", log_levels[log_level].c_str());
    }
}

void loop() {
    // Your code here
    delay(1000);
}

/* Example curl commands to interact with the endpoints:

Get current WiFi settings (both work):
curl http://YOUR_ESP_IP/settings
curl http://YOUR_ESP_IP/settings/main

Update WiFi settings:
curl -X POST -d "wifi_ssid=MyNetwork" -d "wifi_pass=MyPassword" http://YOUR_ESP_IP/settings/main

Get MQTT settings:
curl http://YOUR_ESP_IP/settings/mqtt

Update MQTT settings:
curl -X POST -d "host=test.mosquitto.org" -d "port=1883" http://YOUR_ESP_IP/settings/mqtt

Get network settings:
curl http://YOUR_ESP_IP/settings/network

Update network settings:
curl -X POST -d "hostname=myesp" -d "dhcp=0" -d "ip=192.168.1.100" http://YOUR_ESP_IP/settings/network

Get debug settings (both work):
curl http://YOUR_ESP_IP/extras
curl http://YOUR_ESP_IP/settings/extra

Update debug settings:
curl -X POST -d "debug=1" -d "update_interval=30.5" -d "log_level=3" http://YOUR_ESP_IP/settings/extra

*/
