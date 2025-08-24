#include <Arduino.h>
#include <unity.h>
#include "json_utils.h"

void test_utf8_preserved() {
    String raw = "O’Reilly"; // O’Reilly with curly apostrophe
    TEST_ASSERT_EQUAL_STRING("O’Reilly", json_encode(raw).c_str());
}

void test_control_escaped() {
    String raw = "line\nfeed";
    TEST_ASSERT_EQUAL_STRING("line\\nfeed", json_encode(raw).c_str());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_utf8_preserved);
    RUN_TEST(test_control_escaped);
    return UNITY_END();
}

