#include <unity.h>
#include <Arduino.h>
#include <StreamString.h>
#include <ArduinoJson.h>

#include <espurna/utils.h>

namespace espurna {
namespace test {
namespace {

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
    RUN_TEST(test_parse_unsigned_result);
    RUN_TEST(test_parse_unsigned_value);
    RUN_TEST(test_parse_unsigned_overflow);
    RUN_TEST(test_parse_unsigned_prefix);
    return UNITY_END();
}

