#include <unity.h>

#include <espurna/types.h>
#include <espurna/libs/Delimiter.h>

void test_assert_equal_string_view(espurna::StringView expected, espurna::StringView actual, unsigned int line, const char* msg) {
    UNITY_TEST_ASSERT_EQUAL_INT(expected.length(), actual.length(), line, msg);
    if (msg != nullptr) {
        UNITY_TEST_ASSERT_EQUAL_CHAR_ARRAY(
            expected.data(), actual.data(), actual.length(), line, msg);
    } else {
        String out;

        out += "'";
        out += expected;

        out += "' vs. '";
        out += actual;
        out += "'";

        UNITY_TEST_ASSERT_EQUAL_CHAR_ARRAY(
            expected.data(), actual.data(), actual.length(), line, out.c_str());
    }
}
