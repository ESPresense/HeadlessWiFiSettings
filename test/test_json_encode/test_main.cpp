#include <Arduino.h>
#include <unity.h>
#include <vector>

#include "HeadlessWiFiSettingsInternals.h"
#include "json_utils.h"

void test_utf8_preserved() {
    String raw = "O’Reilly"; // O’Reilly with curly apostrophe
    TEST_ASSERT_EQUAL_STRING("O’Reilly", json_encode(raw).c_str());
}

void test_control_escaped() {
    String raw = "line\nfeed";
    TEST_ASSERT_EQUAL_STRING("line\\nfeed", json_encode(raw).c_str());
}

void test_json_string_field_escapes_name_and_value() {
    String actual = HeadlessWiFiSettingsInternals::jsonString("host\"name", "line\nfeed");
    TEST_ASSERT_EQUAL_STRING("\"host\\\"name\":\"line\\nfeed\"", actual.c_str());
}

void test_json_string_field_omits_empty_values() {
    String actual = HeadlessWiFiSettingsInternals::jsonString("name", "");
    TEST_ASSERT_EQUAL_STRING("", actual.c_str());
}

void test_json_numeric_field_keeps_value_unquoted() {
    String actual = HeadlessWiFiSettingsInternals::jsonNumeric("enabled", "true");
    TEST_ASSERT_EQUAL_STRING("\"enabled\":true", actual.c_str());
}

void test_json_numeric_field_escapes_name_only() {
    String actual = HeadlessWiFiSettingsInternals::jsonNumeric("port\nnumber", "443");
    TEST_ASSERT_EQUAL_STRING("\"port\\nnumber\":443", actual.c_str());
}

void test_json_numeric_field_omits_empty_values() {
    String actual = HeadlessWiFiSettingsInternals::jsonNumeric("port", "");
    TEST_ASSERT_EQUAL_STRING("", actual.c_str());
}

void test_password_json_masks_non_empty_value_and_omits_default() {
    TEST_ASSERT_EQUAL_STRING("\"wifi-password\":\"***###***\"", HeadlessWiFiSettingsInternals::jsonPasswordValue("wifi-password", "secret").c_str());
    TEST_ASSERT_EQUAL_STRING("", HeadlessWiFiSettingsInternals::jsonPasswordValue("wifi-password", "").c_str());
    TEST_ASSERT_EQUAL_STRING("", HeadlessWiFiSettingsInternals::jsonPasswordDefault("wifi-password", "secret").c_str());
}

void test_integer_json_outputs_number_and_omits_empty() {
    TEST_ASSERT_EQUAL_STRING("\"server_port\":443", HeadlessWiFiSettingsInternals::jsonInt("server_port", "443").c_str());
    TEST_ASSERT_EQUAL_STRING("\"server_port\":0", HeadlessWiFiSettingsInternals::jsonInt("server_port", "not-a-number").c_str());
    TEST_ASSERT_EQUAL_STRING("", HeadlessWiFiSettingsInternals::jsonInt("server_port", "").c_str());
}

void test_float_json_outputs_number_and_omits_empty() {
    TEST_ASSERT_EQUAL_STRING("\"interval\":2.500000", HeadlessWiFiSettingsInternals::jsonFloat("interval", "2.5").c_str());
    TEST_ASSERT_EQUAL_STRING("\"interval\":0.000000", HeadlessWiFiSettingsInternals::jsonFloat("interval", "not-a-number").c_str());
    TEST_ASSERT_EQUAL_STRING("", HeadlessWiFiSettingsInternals::jsonFloat("interval", "").c_str());
}

void test_bool_json_outputs_booleans_and_omits_empty() {
    TEST_ASSERT_EQUAL_STRING("\"enabled\":true", HeadlessWiFiSettingsInternals::jsonBool("enabled", "1").c_str());
    TEST_ASSERT_EQUAL_STRING("\"enabled\":false", HeadlessWiFiSettingsInternals::jsonBool("enabled", "0").c_str());
    TEST_ASSERT_EQUAL_STRING("", HeadlessWiFiSettingsInternals::jsonBool("enabled", "").c_str());
}

void test_wifi_endpoint_aliases_to_main() {
    TEST_ASSERT_EQUAL_STRING("main", HeadlessWiFiSettingsInternals::wifiEndpointName("/wifi").c_str());
    TEST_ASSERT_EQUAL_STRING("main", HeadlessWiFiSettingsInternals::wifiEndpointName("/wifi/").c_str());
    TEST_ASSERT_EQUAL_STRING("main", HeadlessWiFiSettingsInternals::wifiEndpointName("/wifi/main").c_str());
}

void test_wifi_endpoint_extracts_custom_name() {
    TEST_ASSERT_EQUAL_STRING("mqtt", HeadlessWiFiSettingsInternals::wifiEndpointName("/wifi/mqtt").c_str());
    TEST_ASSERT_EQUAL_STRING("extras", HeadlessWiFiSettingsInternals::wifiEndpointName("/wifi/extras").c_str());
}

void test_wifi_options_param_name_extracts_suffix() {
    TEST_ASSERT_EQUAL_STRING("log_level", HeadlessWiFiSettingsInternals::wifiOptionsParamName("/wifi/options/log_level").c_str());
}

void test_find_endpoint_returns_index_or_not_found() {
    std::vector<String> names = {"main", "mqtt", "extras"};
    TEST_ASSERT_EQUAL_INT(0, HeadlessWiFiSettingsInternals::findEndpoint("main", names));
    TEST_ASSERT_EQUAL_INT(1, HeadlessWiFiSettingsInternals::findEndpoint("mqtt", names));
    TEST_ASSERT_EQUAL_INT(-1, HeadlessWiFiSettingsInternals::findEndpoint("missing", names));
}

void test_resolve_wifi_endpoint_applies_alias_before_lookup() {
    std::vector<String> names = {"main", "mqtt", "extras"};
    TEST_ASSERT_EQUAL_INT(0, HeadlessWiFiSettingsInternals::resolveWifiEndpoint("/wifi", names));
    TEST_ASSERT_EQUAL_INT(0, HeadlessWiFiSettingsInternals::resolveWifiEndpoint("/wifi/main", names));
    TEST_ASSERT_EQUAL_INT(1, HeadlessWiFiSettingsInternals::resolveWifiEndpoint("/wifi/mqtt", names));
    TEST_ASSERT_EQUAL_INT(-1, HeadlessWiFiSettingsInternals::resolveWifiEndpoint("/wifi/missing", names));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_utf8_preserved);
    RUN_TEST(test_control_escaped);
    RUN_TEST(test_json_string_field_escapes_name_and_value);
    RUN_TEST(test_json_string_field_omits_empty_values);
    RUN_TEST(test_json_numeric_field_keeps_value_unquoted);
    RUN_TEST(test_json_numeric_field_escapes_name_only);
    RUN_TEST(test_json_numeric_field_omits_empty_values);
    RUN_TEST(test_password_json_masks_non_empty_value_and_omits_default);
    RUN_TEST(test_integer_json_outputs_number_and_omits_empty);
    RUN_TEST(test_float_json_outputs_number_and_omits_empty);
    RUN_TEST(test_bool_json_outputs_booleans_and_omits_empty);
    RUN_TEST(test_wifi_endpoint_aliases_to_main);
    RUN_TEST(test_wifi_endpoint_extracts_custom_name);
    RUN_TEST(test_wifi_options_param_name_extracts_suffix);
    RUN_TEST(test_find_endpoint_returns_index_or_not_found);
    RUN_TEST(test_resolve_wifi_endpoint_applies_alias_before_lookup);
    return UNITY_END();
}
