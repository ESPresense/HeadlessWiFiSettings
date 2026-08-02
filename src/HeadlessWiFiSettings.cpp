#include "HeadlessWiFiSettings.h"

#define ESPFS SPIFFS
#define ESPMAC (Sprintf("%06" PRIx32, ((uint32_t)(ESP.getEfuseMac() >> 24))))

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <esp_wifi.h>
#include <limits.h>

#include <vector>
#include "json_utils.h"

#if HEADLESS_WIFI_SETTINGS_HAS_IMPROV
namespace {
    // Chip family advertised to Improv provisioning tools.
    const char* improvChipName() {
#if defined(CONFIG_IDF_TARGET_ESP32C3)
        return "ESP32-C3";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
        return "ESP32-S2";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
        return "ESP32-S3";
#elif defined(ARDUINO_ARCH_ESP8266)
        return "ESP8266";
#else
        return "ESP32";
#endif
    }
}
#endif

#define Sprintf(f, ...) ({ char* s; asprintf(&s, f, __VA_ARGS__); String r = s; free(s); r; })

namespace { // Helpers
static constexpr char ERROR_FLASH[] = "Error writing to flash filesystem";
static constexpr char ENDPOINT_NOT_FOUND[] = "Endpoint not found";
static constexpr char ERROR_AP_START[] = "Failed to start access point!";
static constexpr char WIFI_PATH[] = "/wifi/";
static constexpr char JSON_NAME_VALUE[] = "\"{name}\":\"{value}\"";
static constexpr char JSON_NAME_NUM[] = "\"{name}\":{value}";
static constexpr char CONTENT_JSON[] = "application/json; charset=utf-8";
static constexpr char CONTENT_TEXT[] = "text/plain";

void ensureMainEndpoint();

    String slurp(const String &fn) {
        File f = ESPFS.open(fn, "r");
        String r = f.readString();
        f.close();
        return r;
    }

    bool spurt(const String &fn, const String &content) {
        if (content.isEmpty())
            return ESPFS.exists(fn) ? ESPFS.remove(fn) : true;
        File f = ESPFS.open(fn, "w");
        if (!f) return false;
        auto w = f.print(content);
        f.close();
        return w == content.length();
    }

    // Helper to format JSON string values
    String jsonString(const String &name, const String &value) {
        if (value == "") return "";
        String j = F(JSON_NAME_VALUE);
        j.replace("{name}", json_encode(name));
        j.replace("{value}", json_encode(value));
        return j;
    }

    // Helper to format JSON numeric values
    String jsonNumeric(const String &name, const String &value) {
        if (value == "") return "";
        String j = F(JSON_NAME_NUM);
        j.replace("{name}", json_encode(name));
        j.replace("{value}", value);
        return j;
    }

    enum class ParamType {
        Dropdown,
        String,
        Password,
        Int,
        Float,
        Bool
    };

    struct HeadlessWiFiSettingsParameter {
        String name;
        String label;
        String value;
        String init;
        long min = LONG_MIN;
        long max = LONG_MAX;
        ParamType type;

        String filename() {
            String fn = "/";
            fn += name;
            return fn;
        }

        bool store() { return (name && name.length()) ? spurt(filename(), value) : true; }

        void fill() { if (name && name.length()) value = slurp(filename()); }

        virtual void set(const String &) = 0;

        virtual String jsonValue() = 0;
        virtual String jsonDefault() = 0;

        ParamType getType() const { return type; }
    };

    struct HeadlessWiFiSettingsDropdown : HeadlessWiFiSettingsParameter {
        HeadlessWiFiSettingsDropdown() { type = ParamType::Dropdown; }
        virtual void set(const String &v) { value = v; }

        std::vector<String> options;

        String jsonValue() { return jsonString(name, value); }
        String jsonDefault() { return jsonString(name, init); }
    };

    struct HeadlessWiFiSettingsString : HeadlessWiFiSettingsParameter {
        HeadlessWiFiSettingsString() { type = ParamType::String; }
        virtual void set(const String &v) { value = v; }

        String jsonValue() { return jsonString(name, value); }
        String jsonDefault() { return jsonString(name, init); }
    };

    static const char* const MASKED_PASSWORD = "***###***";

    struct HeadlessWiFiSettingsPassword : HeadlessWiFiSettingsParameter {
        HeadlessWiFiSettingsPassword() { type = ParamType::Password; }
        virtual void set(const String &v) {
            if (v == MASKED_PASSWORD) return;
            value = v;
        }

        String jsonValue() { return value.length() ? jsonString(name, MASKED_PASSWORD) : ""; }
        String jsonDefault() { return ""; }
    };

    struct HeadlessWiFiSettingsInt : HeadlessWiFiSettingsParameter {
        HeadlessWiFiSettingsInt() { type = ParamType::Int; }
        virtual void set(const String &v) { value = v; }

        String jsonValue() { return jsonNumeric(name, value.length() ? String(value.toInt()) : ""); }
        String jsonDefault() { return jsonNumeric(name, init.length() ? String(init.toInt()) : ""); }
    };

    struct HeadlessWiFiSettingsFloat : HeadlessWiFiSettingsParameter {
        HeadlessWiFiSettingsFloat() { type = ParamType::Float; }
        virtual void set(const String &v) { value = v; }

        String jsonValue() { return jsonNumeric(name, value.length() ? String(value.toFloat()) : ""); }
        String jsonDefault() { return jsonNumeric(name, init.length() ? String(init.toFloat()) : ""); }
    };

    struct HeadlessWiFiSettingsBool : HeadlessWiFiSettingsParameter {
        HeadlessWiFiSettingsBool() { type = ParamType::Bool; }
        virtual void set(const String &v) { value = v.length() ? "1" : "0"; }

        String jsonValue() { return jsonNumeric(name, value.length() ? (value.toInt() ? "true" : "false") : ""); }
        String jsonDefault() { return jsonNumeric(name, init.length() ? (init.toInt() ? "true" : "false") : ""); }
    };

    // Parallel vectors for endpoint names and parameters
    std::vector<String> endpointNames;
    std::vector<std::vector<HeadlessWiFiSettingsParameter *>> endpointParams;
    uint8_t currentEndpointIndex = 0;

    void ensureMainEndpoint() {
        if (endpointNames.empty()) {
            endpointNames.push_back("main");
            endpointParams.push_back({});
        }
    }

    std::vector<HeadlessWiFiSettingsParameter *> *params() {
        ensureMainEndpoint();
        return &endpointParams[currentEndpointIndex];
    }

    // Find or create endpoint
    uint8_t findOrCreateEndpoint(const String& name) {
        ensureMainEndpoint();
        // Look for existing endpoint
        for (size_t i = 0; i < endpointNames.size(); i++) {
            if (endpointNames[i] == name) {
                return i;
            }
        }
        // Create new endpoint
        endpointNames.push_back(name);
        endpointParams.push_back({});
        return endpointNames.size() - 1;
    }

    // Find existing endpoint (returns -1 if not found)
    int findEndpoint(const String& name) {
        ensureMainEndpoint();
        for (size_t i = 0; i < endpointNames.size(); i++) {
            if (endpointNames[i] == name) {
                return i;
            }
        }
        return -1;
    }
} // namespace

String HeadlessWiFiSettingsClass::pstring(const String &name, const String &init, const String &label) {
    begin();
    auto *x = new HeadlessWiFiSettingsPassword();
    x->name = name;
    x->label = label.length() ? label : name;
    x->init = init;
    x->fill();

    params()->push_back(x);
    return x->value.length() ? x->value : x->init;
}

String HeadlessWiFiSettingsClass::string(const String &name, const String &init, const String &label) {
    begin();
    auto *x = new HeadlessWiFiSettingsString();
    x->name = name;
    x->label = label.length() ? label : name;
    x->init = init;
    x->fill();

    params()->push_back(x);
    return x->value.length() ? x->value : x->init;
}

String HeadlessWiFiSettingsClass::string(const String &name, unsigned int max_length, const String &init, const String &label) {
    String rv = string(name, init, label);
    params()->back()->max = max_length;
    return rv;
}

String HeadlessWiFiSettingsClass::string(const String &name, unsigned int min_length, unsigned int max_length, const String &init, const String &label) {
    String rv = string(name, init, label);
    params()->back()->min = min_length;
    params()->back()->max = max_length;
    return rv;
}

long HeadlessWiFiSettingsClass::dropdown(const String &name, std::vector<String> options, long init, const String &label) {
    begin();
    auto *x = new HeadlessWiFiSettingsDropdown();
    x->name = name;
    x->label = label.length() ? label : name;
    x->init = init;
    x->options = options;
    x->fill();

    params()->push_back(x);
    return (x->value.length() ? x->value : x->init).toInt();
}

long HeadlessWiFiSettingsClass::integer(const String &name, long init, const String &label) {
    begin();
    auto *x = new HeadlessWiFiSettingsInt();
    x->name = name;
    x->label = label.length() ? label : name;
    x->init = init;
    x->fill();

    params()->push_back(x);
    return (x->value.length() ? x->value : x->init).toInt();
}

long HeadlessWiFiSettingsClass::integer(const String &name, long min, long max, long init, const String &label) {
    long rv = integer(name, init, label);
    params()->back()->min = min;
    params()->back()->max = max;
    return rv;
}

float HeadlessWiFiSettingsClass::floating(const String &name, float init, const String &label) {
    begin();
    auto *x = new HeadlessWiFiSettingsFloat();
    x->name = name;
    x->label = label.length() ? label : name;
    x->init = init;
    x->fill();

    params()->push_back(x);
    return (x->value.length() ? x->value : x->init).toFloat();
}

float HeadlessWiFiSettingsClass::floating(const String &name, long min, long max, float init, const String &label) {
    float rv = floating(name, init, label);
    params()->back()->min = min;
    params()->back()->max = max;
    return rv;
}

bool HeadlessWiFiSettingsClass::checkbox(const String &name, bool init, const String &label) {
    begin();
    auto *x = new HeadlessWiFiSettingsBool();
    x->name = name;
    x->label = label.length() ? label : name;
    x->init = String((int)init);
    x->fill();

    if (!x->value.length()) x->value = x->init;

    params()->push_back(x);
    return x->value.toInt();
}

void HeadlessWiFiSettingsClass::markEndpoint(const String& name) {
    currentEndpointIndex = findOrCreateEndpoint(name);
}

void HeadlessWiFiSettingsClass::markExtra() {
    currentEndpointIndex = findOrCreateEndpoint("extras");
}

void HeadlessWiFiSettingsClass::beginSerialImprov(const String& firmwareName, const String& firmwareVersion, const String& deviceName) {
#if HEADLESS_WIFI_SETTINGS_HAS_IMPROV
    begin();
    if (improv) {
        delete improv;
        improv = nullptr;
    }
    improvFirmware = firmwareName;
    improvVersion = firmwareVersion;
    improvChip = improvChipName();
    improvName = deviceName.length() ? deviceName : hostname;
    // NOTE: ImprovWiFi keeps the pointers, so these must be long-lived members.
    improv = new ImprovWiFi(improvFirmware.c_str(), improvVersion.c_str(), improvChip.c_str(), improvName.c_str());
    // No info/debug callbacks: they would print onto the same UART the Improv
    // protocol uses and corrupt the byte stream.
    improv->setWiFiCallback([this](const char* ssid, const char* password) {
        if (!(spurt("/wifi-ssid", ssid) && spurt("/wifi-password", password))) {
            if (onFailure) onFailure();
            return;  // ImprovWiFi::loop() reports the 10s timeout as an error
        }
        if (onConfigSaved) onConfigSaved();
        // Non-blocking: kick off the connection and return immediately. ImprovWiFi::loop()
        // polls WiFi.status() and sends PROVISIONED (with the device URL) once we connect.
        // Blocking here would starve the task watchdog and stall the Improv handler;
        // restarting would drop the serial session before the client gets its reply.
        if (WiFi.getMode() & WIFI_STA) WiFi.disconnect(true, true);
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(hostname.c_str());
        WiFi.begin(ssid, password);
    });
#else
    (void)firmwareName; (void)firmwareVersion; (void)deviceName;
#endif
}

void HeadlessWiFiSettingsClass::serialImprovLoop() {
#if HEADLESS_WIFI_SETTINGS_HAS_IMPROV
    if (improv) improv->loop();
#endif
}

void HeadlessWiFiSettingsClass::httpSetup(bool wifi) {
    begin();

    static bool const configureWifi = wifi;
    static String ip = WiFi.softAPIP().toString();

    if (onHttpSetup) onHttpSetup(&http);

    auto redirect = [](AsyncWebServerRequest *request) {
        if (!configureWifi) return false;
        // iPhone doesn't deal well with redirects to http://hostname/ and
        // will wait 40 to 60 seconds before succesful retry. Works flawlessly
        // with http://ip/ though.
        if (request->host() == ip) return false;

        request->redirect("http://" + ip + "/");
        return true;
    };

    // Get dropdown options endpoint
    http.on("/wifi/options/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String path = request->url();
        Serial.print("GET ");
        Serial.println(path);

        String paramName = path.substring(14); // Remove "/wifi/options/"

        // Search all endpoints for the parameter
        HeadlessWiFiSettingsDropdown* dropdown = nullptr;
        for (auto& params : endpointParams) {
            for (auto& p : params) {
                if (p->name == paramName) {
                    if (p->getType() == ParamType::Dropdown) {
                        dropdown = static_cast<HeadlessWiFiSettingsDropdown*>(p);
                        break;
                    }
                }
            }
            if (dropdown) break;
        }

        if (!dropdown) {
            request->send(404, CONTENT_TEXT, "Dropdown not found");
            return;
        }

        AsyncResponseStream *response = request->beginResponseStream(CONTENT_JSON);
        response->print("[");
        bool needsComma = false;
        for (const auto& option : dropdown->options) {
            if (needsComma) response->print(",");
            response->printf("\"%s\"", json_encode(option).c_str());
            needsComma = true;
        }
        response->print("]");
        request->send(response);
    });

    http.on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        String path = request->url();
        Serial.print("GET ");
        Serial.println(path);

        int numNetworks = WiFi.scanNetworks();
        AsyncResponseStream *response = request->beginResponseStream(CONTENT_JSON);
        response->print("{\"networks\":{");

        bool needsComma = false;
        struct Network {
            String ssid;
            int rssi;
        };
        std::vector<Network> networks;

        // First pass: collect all networks with their RSSI values
        for (int i = 0; i < numNetworks; i++) {
            String ssid = WiFi.SSID(i);
            if (ssid.isEmpty()) continue;  // Skip hidden networks

            int rssi = WiFi.RSSI(i);
            bool found = false;

            // Update existing network if we've seen it before
            for (auto& network : networks) {
                if (network.ssid == ssid) {
                    if (rssi > network.rssi) {
                        network.rssi = rssi;  // Keep the highest RSSI value
                    }
                    found = true;
                    break;
                }
            }

            // Add new network if we haven't seen it
            if (!found) {
                networks.push_back({ssid, rssi});
            }
        }

        // Second pass: output the networks with their highest RSSI values
        for (const auto& network : networks) {
            if (needsComma) response->print(",");
            response->printf("\"%s\":%d", json_encode(network.ssid).c_str(), network.rssi);
            needsComma = true;
        }

        response->print("}}");
        request->send(response);
        WiFi.scanDelete();
    });

    // Handler for /wifi/{name} endpoints
    http.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String path = request->url();
        Serial.print("GET ");
        Serial.println(path);

        String endpointName = (path.length() <= 6) ? "main" : path.substring(6);
        int endpointIndex = findEndpoint(endpointName);

        if (endpointIndex < 0) {
            request->send(404, CONTENT_TEXT, ENDPOINT_NOT_FOUND);
            return;
        }

        AsyncResponseStream *response = request->beginResponseStream(CONTENT_JSON);
        response->print("{");

        // Output current values
        response->print("\"values\":{");
        bool needsComma = false;
        for (auto &p : endpointParams[endpointIndex]) {
            auto s = p->jsonValue();
            if (s == "") continue;
            if (needsComma) response->print(",");
            response->print(s);
            needsComma = true;
        }
        response->print("}");

        // Output defaults
        response->print(",\"defaults\":{");
        needsComma = false;
        for (auto &p : endpointParams[endpointIndex]) {
            auto s = p->jsonDefault();
            if (s == "") continue;
            if (needsComma) response->print(",");
            response->print(s);
            needsComma = true;
        }
        response->print("}}");
        request->send(response);
    });

    // Handler for /wifi/{name} POST endpoints
    http.on("/wifi", HTTP_POST, [this](AsyncWebServerRequest *request) {
        String path = request->url();
        Serial.print("POST ");
        Serial.println(path);

        String endpointName = (path.length() <= 6) ? "main" : path.substring(6);
        int endpointIndex = findEndpoint(endpointName);

        if (endpointIndex < 0) {
            request->send(404, CONTENT_TEXT, ENDPOINT_NOT_FOUND);
            return;
        }

        bool ok = true;
        for (auto &p : endpointParams[endpointIndex]) {
            p->set(request->arg(p->name));
            if (!p->store()) ok = false;
        }

        if (ok) {
            request->send(200);
            if (onConfigSaved) onConfigSaved();
        } else {
            Serial.println(ERROR_FLASH);
            request->send(500, CONTENT_TEXT, ERROR_FLASH);
        }
    });

    http.onNotFound([this, &redirect](AsyncWebServerRequest *request) {
        String path = request->url();
        Serial.print("GET ");
        Serial.println(path);
        if (redirect(request)) return;
        request->send(404, CONTENT_TEXT, "404");
    });

    http.begin();
}

void HeadlessWiFiSettingsClass::portal() {
    begin();

    // Just disconnect and set AP mode, no need to scan since we have /wifi/scan endpoint
    // Only disconnect if STA was started (avoids error on ESP32-C6)
    if (WiFi.getMode() & WIFI_STA) {
        WiFi.disconnect(true, true);
    }
    WiFi.mode(WIFI_AP);

    Serial.println(F("Starting access point for configuration portal."));
    if (secure && password.length()) {
        Serial.printf("SSID: '%s', Password: '%s'\n", hostname.c_str(), password.c_str());
        if (!WiFi.softAP(hostname.c_str(), password.c_str()))
            Serial.println(ERROR_AP_START);
    } else {
        Serial.printf("SSID: '%s'\n", hostname.c_str());
        if (!WiFi.softAP(hostname.c_str()))
            Serial.println(ERROR_AP_START);
    }
    delay(500);
    DNSServer dns;
    dns.setTTL(0);
    dns.start(53, "*", WiFi.softAPIP());

    if (onPortal) onPortal();
    String ip = WiFi.softAPIP().toString();
    Serial.printf("IP: %s\n", ip.c_str());

    httpSetup(true);

    unsigned long starttime = millis();
    int desired = 0;
    for (;;) {
        dns.processNextRequest();
        serialImprovLoop();  // service Improv so a device can be provisioned from the portal
        if (onPortalWaitLoop && (millis() - starttime) > desired) {
            desired = onPortalWaitLoop();
            starttime = millis();
        }
        // Guard WDT reset to avoid "task not found" spam on ESP32 core 3.x
        if (esp_task_wdt_status(NULL) == ESP_OK) {
            esp_task_wdt_reset();
        }
        delay(1);
    }
}

bool HeadlessWiFiSettingsClass::connect(bool portal, int wait_seconds) {
    begin();

    if (WiFi.getMode() != WIFI_OFF) {
        WiFi.mode(WIFI_OFF);
    }

    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);

    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);

    String const ssid = slurp("/wifi-ssid");
    String const pw = slurp("/wifi-password");
    if (ssid.length() == 0) {
        Serial.println(F("First contact!\n"));
        if (portal) {
            this->portal();
        }
        return false;
    }

    if (!improvActive()) {
        Serial.print(F("Connecting to WiFi SSID '"));
        Serial.print(ssid);
        Serial.print(F("'"));
    }
    if (onConnect) onConnect();

    WiFi.setHostname(hostname.c_str());
    auto status = WiFi.begin(ssid.c_str(), pw.c_str());

    unsigned long const wait_ms = wait_seconds * 1000UL;
    unsigned long starttime = millis();
    unsigned long lastbegin = starttime;
    while (status != WL_CONNECTED) {
        if (millis() - lastbegin > 60000) {
            lastbegin = millis();
            if (!improvActive()) Serial.print("*");
            WiFi.disconnect(true, true);
            status = WiFi.begin(ssid.c_str(), pw.c_str());
        } else {
            if (!improvActive()) Serial.print(".");
            status = WiFi.status();
        }
        serialImprovLoop();  // keep Improv responsive during the connection wait
        delay(onWaitLoop ? onWaitLoop() : 100);
        if (wait_seconds >= 0 && millis() - starttime > wait_ms)
            break;
    }

    if (status != WL_CONNECTED) {
        if (!improvActive()) Serial.printf(" failed (status=%d).\n", status);
        if (onFailure) onFailure();
        if (portal) this->portal();
        return false;
    }

    if (!improvActive()) Serial.println(WiFi.localIP().toString());
    if (onSuccess) onSuccess();
    return true;
}

void HeadlessWiFiSettingsClass::begin() {
    if (begun) return;
    begun = true;
    if (hostname.endsWith("-")) hostname += ESPMAC;
}

HeadlessWiFiSettingsClass::HeadlessWiFiSettingsClass() : http(80) {
    hostname = F("esp32-");
}

HeadlessWiFiSettingsClass HeadlessWiFiSettings;
