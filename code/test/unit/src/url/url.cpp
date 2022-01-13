#include <Arduino.h>
#include <unity.h>

#include "libs/URL.h"

void test_parse() {
    URL url("http://api.thingspeak.com/update");
    TEST_ASSERT_EQUAL_STRING("api.thingspeak.com", url.host.c_str());
    TEST_ASSERT_EQUAL_STRING("/update", url.path.c_str());
    TEST_ASSERT_EQUAL(80, url.port);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_parse);
    return UNITY_END();
}
