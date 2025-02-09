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

#define Sprintf(f, ...) ({ char* s; asprintf(&s, f, __VA_ARGS__); String r = s; free(s); r; })

namespace { // Helpers
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

    String pwgen() {
        const char *passchars = "ABCEFGHJKLMNPRSTUXYZabcdefhkmnorstvxz23456789-#@?!";
        String password = "";
        for (int i = 0; i < 16; i++) {
            password.concat(passchars[random(strlen(passchars))]);
        }
        return password;
    }

    String json_encode(const String &raw) {
        String r;
        for (unsigned int i = 0; i < raw.length(); i++) {
            char c = raw.charAt(i);
            switch (c) {
                case '\"': r += "\\\""; break;
                case '\\': r += "\\\\"; break;
                case '\b': r += "\\b"; break;
                case '\f': r += "\\f"; break;
                case '\n': r += "\\n"; break;
                case '\r': r += "\\r"; break;
                case '\t': r += "\\t"; break;
                default:
                    if (c < ' ' || c > '~') {
                        r += Sprintf("\\u%04x", c);
                    } else {
                        r += c;
                    }
            }
        }
        return r;
    }

    struct HeadlessWiFiSettingsParameter {
        String name;
        String label;
        String value;
        String init;
        long min = LONG_MIN;
        long max = LONG_MAX;

        String filename() {
            String fn = "/";
            fn += name;
            return fn;
        }

        bool store() { return (name && name.length()) ? spurt(filename(), value) : true; }

        void fill() { if (name && name.length()) value = slurp(filename()); }

        virtual void set(const String &) = 0;

        virtual String json() = 0;
    };

    struct HeadlessWiFiSettingsDropdown : HeadlessWiFiSettingsParameter {
        virtual void set(const String &v) { value = v; }

        std::vector<String> options;

        String json() {
            if (value == "") return "";
            String j = F("\"{name}\":\"{value}\"");
            j.replace("{name}", json_encode(name));
            j.replace("{value}", json_encode(value));
            return j;
        }
    };

    struct HeadlessWiFiSettingsString : HeadlessWiFiSettingsParameter {
        virtual void set(const String &v) { value = v; }

        String json() {
            if (value == "") return "";
            String j = F("\"{name}\":\"{value}\"");
            j.replace("{name}", json_encode(name));
            j.replace("{value}", json_encode(value));
            return j;
        }
    };

    struct HeadlessWiFiSettingsPassword : HeadlessWiFiSettingsParameter {
        virtual void set(const String &v) {
            String trimmed = v;
            trimmed.trim();
            if (trimmed.length()) value = trimmed;
        }

        String json() {
            return "";
        }
    };

    struct HeadlessWiFiSettingsInt : HeadlessWiFiSettingsParameter {
        virtual void set(const String &v) { value = v; }

        String json() {
            if (value == "") return "";
            String j = F("\"{name}\":\"{value}\"");
            j.replace("{name}", json_encode(name));
            j.replace("{value}", String(value.toInt()));
            return j;
        }
    };

    struct HeadlessWiFiSettingsFloat : HeadlessWiFiSettingsParameter {
        virtual void set(const String &v) { value = v; }

        String json() {
            if (value == "") return "";
            String j = F("\"{name}\":{value}");
            j.replace("{name}", json_encode(name));
            j.replace("{value}", String(value.toFloat()));
            return j;
        }
    };

    struct HeadlessWiFiSettingsBool : HeadlessWiFiSettingsParameter {
        virtual void set(const String &v) { value = v.length() ? "1" : "0"; }

        String json() {
            if (value == "") return "";
            String j = F("\"{name}\":{value}");
            j.replace("{name}", json_encode(name));
            j.replace("{value}", value.toInt() ? "true" : "false");
            return j;
        }
    };

    bool extra = false;

    struct std::vector<HeadlessWiFiSettingsParameter *> primary;
    struct std::vector<HeadlessWiFiSettingsParameter *> extras;

    std::vector<HeadlessWiFiSettingsParameter *> *params() {
        return extra ? &extras : &primary;
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

void HeadlessWiFiSettingsClass::markExtra() {
    extra = true;
}

void HeadlessWiFiSettingsClass::httpSetup(bool wifi) {
    begin();

    if (onHttpSetup) onHttpSetup(&http);

    http.on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        response->print("{");
        bool needsComma = false;
        for (auto &p : primary) {
            auto s = p->json();
            if (s == "") continue;
            if (needsComma) response->print(",");
            response->print(s);
            needsComma = true;
        }
        response->print("}");
        request->send(response);
    });

    http.on("/settings", HTTP_POST, [this](AsyncWebServerRequest *request) {
        bool ok = true;

        for (auto &p : primary) {
            p->set(request->arg(p->name));
            if (!p->store()) ok = false;
        }

        if (ok) {
            request->send(200);
            if (onConfigSaved) onConfigSaved();
        } else {
            request->send(500, "text/plain", "Error writing to flash filesystem");
        }
    });

    http.on("/extras", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        response->print("{");
        bool needsComma = false;
        for (auto &p : extras) {
            auto s = p->json();
            if (s == "") continue;
            if (needsComma) response->print(",");
            response->print(s);
            needsComma = true;
        }
        response->print("}");
        request->send(response);
    });

    http.on("/extras", HTTP_POST, [this](AsyncWebServerRequest *request) {
        bool ok = true;

        for (auto &p : extras) {
            p->set(request->arg(p->name));
            if (!p->store()) ok = false;
        }

        if (ok) {
            request->send(200);
            if (onConfigSaved) onConfigSaved();
        } else {
            request->send(500, "text/plain", "Error writing to flash filesystem");
        }
    });

    http.onNotFound([this](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "404");
    });

    http.begin();
}

void HeadlessWiFiSettingsClass::portal() {
    begin();

#ifdef ESP32
    WiFi.disconnect(true, true);
#else
    WiFi.disconnect(true);
#endif
    WiFi.mode(WIFI_AP);

    Serial.println(F("Starting access point for configuration portal."));
    if (secure && password.length()) {
        Serial.printf("SSID: '%s', Password: '%s'\n", hostname.c_str(), password.c_str());
        if (!WiFi.softAP(hostname.c_str(), password.c_str()))
            Serial.println("Failed to start access point!");
    } else {
        Serial.printf("SSID: '%s'\n", hostname.c_str());
        if (!WiFi.softAP(hostname.c_str()))
            Serial.println("Failed to start access point!");
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
        if (onPortalWaitLoop && (millis() - starttime) > desired) {
            desired = onPortalWaitLoop();
            starttime = millis();
        }
        esp_task_wdt_reset();
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

    String ssid = slurp("/wifi-ssid");
    String pw = slurp("/wifi-password");
    if (ssid.length() == 0) {
        Serial.println(F("First contact!\n"));
        this->portal();
    }

    Serial.print(F("Connecting to WiFi SSID '"));
    Serial.print(ssid);
    Serial.print(F("'"));
    if (onConnect) onConnect();

    WiFi.setHostname(hostname.c_str());
    auto status = WiFi.begin(ssid.c_str(), pw.c_str());

    unsigned long const wait_ms = wait_seconds * 1000UL;
    unsigned long starttime = millis();
    unsigned long lastbegin = starttime;
    while (status != WL_CONNECTED) {
        if (millis() - lastbegin > 60000) {
            lastbegin = millis();
            Serial.print("*");
            WiFi.disconnect(true, true);
            status = WiFi.begin(ssid.c_str(), pw.c_str());
        } else {
            Serial.print(".");
            status = WiFi.status();
        }
        delay(onWaitLoop ? onWaitLoop() : 100);
        if (wait_seconds >= 0 && millis() - starttime > wait_ms)
            break;
    }

    if (status != WL_CONNECTED) {
        Serial.printf(" failed (status=%d).\n", status);
        if (onFailure) onFailure();
        if (portal) this->portal();
        return false;
    }

    Serial.println(WiFi.localIP().toString());
    if (onSuccess) onSuccess();
    return true;
}

void HeadlessWiFiSettingsClass::begin() {
    if (begun) return;
    begun = true;

#ifdef PORTAL_PASSWORD
    if (!secure) {
        secure = checkbox(
            F("HeadlessWiFiSettings-secure"),
            false,
            "Secure Portal"
        );
    }

    if (!password.length()) {
        password = string(
            F("HeadlessWiFiSettings-password"),
            8, 63,
            "",
            "Portal Password"
        );
        if (password == "") {
            password = pwgen();
            params()->back()->set(password);
            params()->back()->store();
        }
    }
#endif

    if (hostname.endsWith("-")) hostname += ESPMAC;
}

HeadlessWiFiSettingsClass::HeadlessWiFiSettingsClass() : http(80) {
#ifdef ESP32
    hostname = F("esp32-");
#else
    hostname = F("esp8266-");
#endif
}

HeadlessWiFiSettingsClass HeadlessWiFiSettings;