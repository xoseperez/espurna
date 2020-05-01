#include <unity.h>
#include <Arduino.h>

// Ensure build system works
// ref: https://github.com/bxparks/UnixHostDuino/pull/6

void test_linkage() {
    pinMode(0, INPUT);
    pinMode(0, OUTPUT);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
	RUN_TEST(test_linkage);
    UNITY_END();
}
