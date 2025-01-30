#include <unity.h>
#include <Arduino.h>
#include <StreamString.h>
#include <ArduinoJson.h>

#include <espurna/utils.h>

namespace espurna {
namespace test {
namespace {

void test_consume_available() {
    const char tmp[] {
        "aaaa"
        "bbbb"
        "cccc"
        "dddd"
    };

    StreamString stream;
    TEST_ASSERT(stream.hasPeekBufferAPI());

    stream.concat(&tmp[0], __builtin_strlen(tmp));

    const auto available = stream.available();
    TEST_ASSERT_EQUAL(__builtin_strlen(tmp), available);
    TEST_ASSERT_EQUAL(__builtin_strlen(tmp), consumeAvailable(stream));
}

void test_pretty_duration() {
    const duration::Seconds one =
        duration::Days{5}
      + duration::Hours{15}
      + duration::Minutes{30}
      + duration::Seconds{45};

    TEST_ASSERT_EQUAL_STRING("5d 15h 30m 45s",
            prettyDuration(one).c_str());

    const duration::Seconds two =
        duration::Days{5}
      + duration::Hours{15};

    TEST_ASSERT_EQUAL_STRING("5d 15h",
            prettyDuration(two).c_str());

    const auto three = duration::Seconds{0};
    TEST_ASSERT_EQUAL_STRING("0s",
            prettyDuration(three).c_str());
}

void test_parse_unsigned_result() {
    const auto result = parseUnsigned("");
    using Value = std::is_same<decltype(result.value), uint32_t>;
    TEST_ASSERT(static_cast<bool>(!result.ok));
    TEST_ASSERT(Value::value);
}

void test_parse_unsigned_value() {
#define TEST_RESULT(VALUE) ([] {\
    const auto result = parseUnsigned(#VALUE);\
    TEST_ASSERT(result.ok);\
    TEST_ASSERT_EQUAL(VALUE, result.value);})()

    TEST_RESULT(12345);
    TEST_RESULT(54321);
    TEST_RESULT(0b111);
    TEST_RESULT(0xfeaf);
}

void test_parse_unsigned_overflow() {
    const auto a = parseUnsigned("0b1111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
    TEST_ASSERT(!a.ok);

    const auto b = parseUnsigned("0o12345123451234512345123451234512345");
    TEST_ASSERT(!b.ok);

    const auto c = parseUnsigned("12345678901234567890");
    TEST_ASSERT(!c.ok);

    const auto d = parseUnsigned("0xfefefefefe");
    TEST_ASSERT(!d.ok);
}

void test_parse_unsigned_prefix() {
    const auto a = parseUnsigned("0b101010101", 2);
    TEST_ASSERT(!a.ok);

    const auto b = parseUnsigned("101010101", 2);
    TEST_ASSERT_EQUAL(0b101010101, b.value);
    TEST_ASSERT(b.ok);

    const auto c = parseUnsigned("0o123134");
    TEST_ASSERT_EQUAL(42588, c.value);
    TEST_ASSERT(c.ok);
}

} // namespace
} // namespace test
} // namespace espurna

int main(int, char**) {
    UNITY_BEGIN();
    using namespace espurna::test;
    RUN_TEST(test_consume_available);
    RUN_TEST(test_pretty_duration);
    RUN_TEST(test_parse_unsigned_result);
    RUN_TEST(test_parse_unsigned_value);
    RUN_TEST(test_parse_unsigned_overflow);
    RUN_TEST(test_parse_unsigned_prefix);
    return UNITY_END();
}

