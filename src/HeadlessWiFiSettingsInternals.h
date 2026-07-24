#pragma once

#include <Arduino.h>
#include <vector>

#include "json_utils.h"

namespace HeadlessWiFiSettingsInternals {
static constexpr char ERROR_FLASH[] = "Error writing to flash filesystem";
static constexpr char ENDPOINT_NOT_FOUND[] = "Endpoint not found";
static constexpr char ERROR_AP_START[] = "Failed to start access point!";
static constexpr char CONTENT_JSON[] = "application/json; charset=utf-8";
static constexpr char CONTENT_TEXT[] = "text/plain";
static constexpr char MASKED_PASSWORD[] = "***###***";

inline String jsonString(const String &name, const String &value) {
    if (value == "") return "";
    String j = "\"{name}\":\"{value}\"";
    j.replace("{name}", json_encode(name));
    j.replace("{value}", json_encode(value));
    return j;
}

inline String jsonNumeric(const String &name, const String &value) {
    if (value == "") return "";
    String j = "\"{name}\":{value}";
    j.replace("{name}", json_encode(name));
    j.replace("{value}", value);
    return j;
}

inline String jsonPasswordValue(const String &name, const String &value) {
    return value.length() ? jsonString(name, MASKED_PASSWORD) : "";
}

inline String jsonPasswordDefault(const String &, const String &) {
    return "";
}

inline String jsonInt(const String &name, const String &value) {
    return jsonNumeric(name, value.length() ? String(value.toInt()) : "");
}

inline String jsonFloat(const String &name, const String &value) {
    return jsonNumeric(name, value.length() ? String(value.toFloat()) : "");
}

inline String jsonBool(const String &name, const String &value) {
    return jsonNumeric(name, value.length() ? (value.toInt() ? "true" : "false") : "");
}

inline String wifiEndpointName(const String &path) {
    return (path.length() <= 6) ? "main" : path.substring(6);
}

inline String wifiOptionsParamName(const String &path) {
    return path.substring(14);
}

inline int findEndpoint(const String &name, const std::vector<String> &endpointNames) {
    for (size_t i = 0; i < endpointNames.size(); i++) {
        if (endpointNames[i] == name) {
            return i;
        }
    }
    return -1;
}

inline int resolveWifiEndpoint(const String &path, const std::vector<String> &endpointNames) {
    return findEndpoint(wifiEndpointName(path), endpointNames);
}
} // namespace HeadlessWiFiSettingsInternals
