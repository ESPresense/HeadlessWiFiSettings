#include <SPIFFS.h>
#include <HeadlessWiFiSettings.h>
#include <ESPAsyncWebServer.h>

static const char PROVISION_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>ESP Provisioning</title>
  <style>
    body { font-family: system-ui, -apple-system, sans-serif; margin: 2rem; background: #f5f5f5; }
    form { background: #fff; padding: 1.5rem; border-radius: 8px; max-width: 420px; box-shadow: 0 2px 8px rgba(0,0,0,.08); }
    label { display: block; margin-bottom: .5rem; font-weight: 600; }
    input { width: 100%; padding: .5rem; margin-bottom: 1rem; border: 1px solid #ccc; border-radius: 4px; font-size: 1rem; }
    button { width: 100%; padding: .6rem; font-size: 1rem; border: none; border-radius: 4px; background: #007aff; color: #fff; cursor: pointer; }
    button:disabled { opacity: .6; cursor: not-allowed; }
    .status { margin-top: 1rem; font-size: .95rem; }
  </style>
</head>
<body>
  <form id="wifi-form">
    <h2>Configure Wi-Fi</h2>
    <label for="ssid">SSID</label>
    <input id="ssid" name="wifi-ssid" placeholder="Network name" required>
    <label for="pass">Password</label>
    <input id="pass" name="wifi-password" type="password" placeholder="Network password">
    <button type="submit">Save</button>
    <div class="status" id="status"></div>
  </form>
  <script>
    const statusEl = document.getElementById('status');
    async function loadCurrent() {
      try {
        const res = await fetch('/wifi/main');
        if (!res.ok) return;
        const data = await res.json();
        if (data['wifi-ssid']) document.getElementById('ssid').value = data['wifi-ssid'];
      } catch (e) {}
    }
    loadCurrent();
    document.getElementById('wifi-form').addEventListener('submit', async (ev) => {
      ev.preventDefault();
      const form = ev.currentTarget;
      const fd = new FormData(form);
      const body = new URLSearchParams();
      for (const [k, v] of fd.entries()) body.append(k, v);
      statusEl.textContent = 'Saving...';
      form.querySelector('button').disabled = true;
      try {
        const res = await fetch('/wifi', {
          method: 'POST',
          body
        });
        if (res.ok) {
          statusEl.textContent = 'Saved! Restarting device...';
          await fetch('/restart', { method: 'POST' }).catch(() => {});
        } else {
          statusEl.textContent = `Server responded ${res.status} ${res.statusText}`;
          form.querySelector('button').disabled = false;
        }
      } catch (err) {
        statusEl.textContent = 'Error sending credentials.';
        form.querySelector('button').disabled = false;
      }
    });
  </script>
</body>
</html>
)HTML";

void setup() {
    Serial.begin(115200);
    SPIFFS.begin(true);
    HeadlessWiFiSettings.string("wifi-ssid");
    HeadlessWiFiSettings.pstring("wifi-password");
    HeadlessWiFiSettings.onHttpSetup = [](AsyncWebServer* server) {
        server->on("/", HTTP_ANY, [](AsyncWebServerRequest* request) {
            request->redirect("/provision");
        });
        server->on("/provision", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "text/html; charset=utf-8", PROVISION_PAGE);
        });
        server->on("/restart", HTTP_POST, [](AsyncWebServerRequest* request) {
            request->send(200, "text/plain", "Restarting...");
            delay(100);
            ESP.restart();
        });
    };
    HeadlessWiFiSettings.startImprovSerial("HeadlessWiFiSettings", "1.0");
    HeadlessWiFiSettings.connect();
}

void loop() {
    HeadlessWiFiSettings.loop();
}
