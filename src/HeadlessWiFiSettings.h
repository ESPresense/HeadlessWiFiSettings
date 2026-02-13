#ifndef HeadlessWiFiSettings_h
#define HeadlessWiFiSettings_h

#include <Arduino.h>
#include <functional>

#include <ESPAsyncWebServer.h>

#if defined(__has_include)
#  if __has_include(<ImprovWiFiLibrary.h>)
#    include <ImprovWiFiLibrary.h>
#    define HEADLESS_WIFI_SETTINGS_HAS_IMPROV 1
#  else
#    define HEADLESS_WIFI_SETTINGS_HAS_IMPROV 0
#  endif
#else
#  define HEADLESS_WIFI_SETTINGS_HAS_IMPROV 0
#endif

class HeadlessWiFiSettingsClass {
    public:
        typedef std::function<void(void)> TCallback;
        typedef std::function<int(void)> TCallbackReturnsInt;
        typedef std::function<void(String&)> TCallbackString;

        HeadlessWiFiSettingsClass();
        void markExtra();
        void markEndpoint(const String& name);
        void begin();
        bool connect(bool portal = true, int wait_seconds = 60);
        void portal();
        void httpSetup(bool softAP = false);
        void beginSerialImprov(const String& firmwareName,
                               const String& firmwareVersion,
                               const String& deviceName = "",
                               Stream* serial = nullptr,
                               const String& deviceUrl = "");
        void serialImprovLoop();
        String string(const String &name, const String &init = "", const String &label = "");
        String string(const String& name, unsigned int max_length, const String& init = "", const String& label = "");
        String string(const String& name, unsigned int min_length, unsigned int max_length, const String& init = "", const String& label = "");
        String pstring(const String& name, const String& init = "", const String& label = "");
        long dropdown(const String& name, std::vector<String> options, long init = 0, const String& label = "");
        long integer(const String& name, long init = 0, const String& label = "");
        long integer(const String& name, long min, long max, long init = 0, const String& label = "");
        float floating(const String &name, float init = 0, const String &label = "");
        float floating(const String &name, long min, long max, float init = 0, const String &label = "");
        bool checkbox(const String& name, bool init = false, const String& label = "");

        String hostname;
        String password;
        bool secure;

        std::function<void(AsyncWebServer*)> onHttpSetup;
        TCallback onConnect;
        TCallbackReturnsInt onWaitLoop;
        TCallback onSuccess;
        TCallback onFailure;
        TCallback onPortal;
        TCallback onPortalView;
        TCallbackString onUserAgent;
        TCallback onConfigSaved;
        TCallback onRestart;
        TCallbackReturnsInt onPortalWaitLoop;
        TCallback onImprovIdentify;
    private:
        AsyncWebServer http;
        bool begun = false;
        bool httpBegun = false;
#if HEADLESS_WIFI_SETTINGS_HAS_IMPROV
        ImprovWiFi* improv = nullptr;
        Stream* improvSerial = nullptr;
        bool handleImprovCredentials(const char* ssid, const char* password);
        void handleImprovIdentify();
        static bool improvConnectTrampoline(const char* ssid, const char* password);
#if defined(IMPROV_WIFI_LIBRARY_HAS_IDENTIFY_CALLBACK)
        static void improvIdentifyTrampoline();
#endif
#endif
};

extern HeadlessWiFiSettingsClass HeadlessWiFiSettings;

#endif
