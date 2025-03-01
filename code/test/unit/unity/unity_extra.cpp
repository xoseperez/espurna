#include <unity.h>

#include <espurna/types.h>
#include <espurna/libs/Delimiter.h>

namespace {

String printable_string(espurna::StringView view) {
    String out;
    out.reserve(view.length());

    for (auto c : view) {
        if (isprint(c)) {
            out += c;
        } else {
            out += "\\x";
            out += String(int(c), 16);
        }
    }

    return out;
}

String expected_actual(espurna::StringView expected, espurna::StringView actual) {
    String out;

    out += "'";
    out += expected;

    out += "' ";
    out += "(" + String(expected.length(), 10) + ") vs. ";

    out += "'";
    out += printable_string(actual);
    out += "' ";
    out += "(" + String(actual.length(), 10) + ")";

    return out;
}

} // namespace

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
