#ifndef HeadlessWiFiSettings_h
#define HeadlessWiFiSettings_h

#include <Arduino.h>
#include <functional>

#include <ESPAsyncWebServer.h>

// Serial Improv is optional: it is compiled in only when the ImprovWiFi library
// (ESPresense/ImprovWiFi) is available, so builds that don't want it are unaffected.
#if defined(__has_include)
#  if __has_include(<ImprovWiFi.h>)
#    include <ImprovWiFi.h>
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
    typedef std::function<void(String &)> TCallbackString;

    HeadlessWiFiSettingsClass();
    void markExtra();
    void markEndpoint(const String &name);
    void begin();
    bool connect(bool portal = true, int wait_seconds = 60);
    void portal();
    void httpSetup(bool softAP = false);
    void beginSerialImprov(const String &firmwareName,
                           const String &firmwareVersion,
                           const String &deviceName = "");
    void loop();
    String string(const String &name, const String &init = "", const String &label = "");
    String string(const String &name, unsigned int max_length, const String &init = "", const String &label = "");
    String string(const String &name, unsigned int min_length, unsigned int max_length, const String &init = "", const String &label = "");
    String pstring(const String &name, const String &init = "", const String &label = "");
    long dropdown(const String &name, std::vector<String> options, long init = 0, const String &label = "");
    long integer(const String &name, long init = 0, const String &label = "");
    long integer(const String &name, long min, long max, long init = 0, const String &label = "");
    float floating(const String &name, float init = 0, const String &label = "");
    float floating(const String &name, long min, long max, float init = 0, const String &label = "");
    bool checkbox(const String &name, bool init = false, const String &label = "");

    String hostname;
    String password;
    bool secure;

    std::function<void(AsyncWebServer *)> onHttpSetup;
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

  private:
    AsyncWebServer http;
    bool begun = false;
    bool httpBegun = false;
    // True while Serial Improv is active; used to keep debug prints off the
    // shared UART so they don't corrupt the Improv byte stream.
    bool improvActive() const {
#if HEADLESS_WIFI_SETTINGS_HAS_IMPROV
        return improv != nullptr;
#else
        return false;
#endif
    }
#if HEADLESS_WIFI_SETTINGS_HAS_IMPROV
    ImprovWiFi *improv = nullptr;
    String improvFirmware;
    String improvVersion;
    String improvChip;
    String improvName;
#endif
};

extern HeadlessWiFiSettingsClass HeadlessWiFiSettings;

#endif
