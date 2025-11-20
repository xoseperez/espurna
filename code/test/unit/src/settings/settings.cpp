#include <unity.h>
#include <Arduino.h>

#include <espurna/settings_convert.h>

namespace espurna {
namespace settings {
namespace internal {

using CustomSeconds = std::chrono::duration<float, std::ratio<1>>;

template <>
CustomSeconds convert(const String& value) {
    return espurna::duration::unchecked_parse<CustomSeconds>(value);
}

using CustomMinutes = std::chrono::duration<float, std::ratio<60>>;

template <>
CustomMinutes convert(const String& value) {
    return espurna::duration::unchecked_parse<CustomMinutes>(value);
}

using CustomHours = std::chrono::duration<float, std::ratio<3600>>;

template <>
CustomHours convert(const String& value) {
    return espurna::duration::unchecked_parse<CustomHours>(value);
}

} // namespace internal

namespace test {
namespace {

void test_convert_bool() {
    TEST_ASSERT(!internal::convert<bool>(""));
    TEST_ASSERT(!internal::convert<bool>("-"));
    TEST_ASSERT(!internal::convert<bool>("+"));
    TEST_ASSERT(!internal::convert<bool>("\xfe"));

    TEST_ASSERT(internal::convert<bool>("1"));
    TEST_ASSERT(internal::convert<bool>("on"));
    TEST_ASSERT(internal::convert<bool>("true"));
    TEST_ASSERT(internal::convert<bool>("y"));
    TEST_ASSERT(internal::convert<bool>("yes"));

    TEST_ASSERT(!internal::convert<bool>("invalid"));
    TEST_ASSERT(!internal::convert<bool>("0"));
    TEST_ASSERT(!internal::convert<bool>("false"));
    TEST_ASSERT(!internal::convert<bool>("n"));
    TEST_ASSERT(!internal::convert<bool>("no"));
    TEST_ASSERT(!internal::convert<bool>("off"));
}

#define TEST_CONVERT_ASSERT(COMPARISON, TYPE, EXPECTED, TEXT)\
    ([]() {\
        const auto result = internal::convert<TYPE>(TEXT);\
        static_assert(std::is_same<decltype(result), TYPE const>::value, "");\
        COMPARISON(EXPECTED, result);\
    })()

void test_convert_int() {
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_INT8, int8_t, 0, "invalid text");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_INT8, int8_t, 0, ".5");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_INT8, int8_t, 56, "56");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_INT8, int8_t, 33, "+33");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_INT8, int8_t, -72, "-72");
}

void test_convert_uint() {
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT8, uint8_t, 0, "invalid");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT8, uint8_t, 0, "-0");

    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT8, uint8_t, 10, "10");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT8, uint8_t, (54321 % 256), "54321");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT8, uint8_t, 5, "5.3");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT8, uint8_t, 5, "5.");

    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT16, uint16_t, 0, "-51");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT16, uint16_t, 0, "+61212");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT16, uint16_t, 0, "+121");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT16, uint16_t, 0, "-1121");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT16, uint16_t, 12121, "12121");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT16, uint16_t, 12, "12.345");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT16, uint16_t, 0, "0.345");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT16, uint16_t, 0, ".345");

    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT32, uint32_t, 0, "-1235");
    TEST_CONVERT_ASSERT(TEST_ASSERT_EQUAL_UINT32, uint32_t, 1234554321, "1234554321");
}

#define UNITY_TEST_ASSERT_EQUAL_CHRONO(LHS, RHS, LINE, MESSAGE)\
    ([](auto lhs, auto rhs, int line, const char* message) {\
        using common_type = std::common_type_t<decltype(lhs), decltype(rhs)>;\
        UNITY_TEST_ASSERT_EQUAL_INT(common_type(lhs).count(), common_type(rhs).count(), line, message);\
    })(LHS, RHS, LINE, MESSAGE);

#define TEST_ASSERT_EQUAL_CHRONO(LHS, RHS)\
    UNITY_TEST_ASSERT_EQUAL_CHRONO(LHS, RHS, __builtin_LINE(), "Durations should be equal")

template <typename Convert, typename Expected>
void test_convert(Expected expected, StringView spec, int line = __builtin_LINE()) {
    String message = "\"";
    message += spec;
    message += "\" ";
    message += "unchecked conversion failed";

    // n.b. conversion type must be able to actually hold parsed value, 0 may be a logical error
    UNITY_TEST_ASSERT_EQUAL_CHRONO(expected, internal::convert<Convert>(spec.toString()), line, message.c_str());
}

void test_convert_duration() {
    test_convert<duration::Microseconds>(
        duration::Microseconds(150000), "150000");
    test_convert<duration::Microseconds>(
        duration::Microseconds(300000), "300ms");

    test_convert<internal::CustomSeconds>(
        duration::Milliseconds(1500), "1.5");
    test_convert<duration::Milliseconds>(
        duration::Milliseconds(500), "500");
    test_convert<duration::Seconds>(
        duration::Milliseconds(3000), "3000ms");

    test_convert<duration::Seconds>(
        duration::Seconds(5), "5");
    test_convert<duration::Seconds>(
        duration::Seconds(44), "44s");

    test_convert<duration::Seconds>(
        duration::Seconds(60), "1m");
    test_convert<duration::Minutes>(
        duration::Minutes(1), "60s");
    test_convert<duration::Minutes>(
        duration::Minutes(60), "60m");
    test_convert<duration::Hours>(
        duration::Hours(2), "120m");
    test_convert<duration::Hours>(
        duration::Hours(3), "10800s");

    test_convert<duration::Seconds>(
        duration::Seconds(90), "1m30s");
    test_convert<duration::Seconds>(
        duration::Milliseconds(1000), "1");

    test_convert<internal::CustomMinutes>(
        duration::Seconds(30), "0.5");
    test_convert<internal::CustomHours>(
        duration::Minutes(15), "0.25");

    test_convert<duration::Minutes>(
        duration::Minutes(3), "3.5");
    test_convert<duration::Minutes>(
        duration::Minutes(1), "65s");
    test_convert<duration::Hours>(
        duration::Hours(2), "121m");
    test_convert<duration::Hours>(
        duration::Hours(0), "59m");
    test_convert<duration::Minutes>(
        duration::Seconds(0), "5s");
}

template <typename Ratio>
void test_parse_fail(StringView spec, Ratio ratio, int line = __builtin_LINE()) {
    String message = "\"";
    message += spec;
    message += "\" ";
    message += "should trigger parsing failure";

    UNITY_TEST_ASSERT(!duration::parse(spec, ratio).ok, line, message.c_str());
}

void test_parse_fail(StringView spec, int line = __builtin_LINE()) {
    test_parse_fail(spec, std::milli{}, line);
}

template <typename Expected, typename Ratio>
void test_parse(Expected expected, Ratio ratio, StringView spec, int line = __builtin_LINE()) {
    String base = "\"";
    base += spec;
    base += "\" ";

    const auto result = duration::parse(spec, ratio);

    auto message = base + "cannot be parsed";
    UNITY_TEST_ASSERT(result.ok, line, message.c_str());

    message = base + "parsed duration does not match the expected one";
    UNITY_TEST_ASSERT_EQUAL_CHRONO(expected, duration::to_chrono<Expected>(result.value),
        line, message.c_str());
}

template <typename Expected>
void test_parse(Expected expected, StringView spec, int line = __builtin_LINE()) {
    test_parse(expected, std::milli{}, spec, line);
}

void test_parse_duration() {
    test_parse(duration::Microseconds(11), std::micro{}, "11");
    test_parse(duration::Milliseconds(6), "6");
    test_parse(duration::Seconds(15), std::ratio<1>{}, "15");
    test_parse(duration::Minutes(21), std::ratio<60>{}, "21");
    test_parse(duration::Hours(46), std::ratio<3600>{}, "46");
}

void test_parse_duration_spec() {
    test_parse(duration::Microseconds(150000), "150000μs"); // \xce\xbc
    test_parse(duration::Microseconds(1234), "1234us");
    test_parse(duration::Milliseconds(100), "99ms1000us");
    test_parse(duration::Milliseconds(6789), "6s789ms");
    test_parse(duration::Seconds(1), "1000ms");
    test_parse(duration::Seconds(552), "552s");
    test_parse(duration::Seconds(112), "0h0m112s");
    test_parse(duration::Minutes(99), "99m");
    test_parse(duration::Minutes(5), "4m60s");
    test_parse(duration::Minutes(62) + duration::Seconds(59), "1h2m59s");
    test_parse(duration::Hours(2) + duration::Seconds(5), "2h5000ms");
    test_parse(duration::Hours(7), "7h");
    test_parse(duration::Hours(15) + duration::Seconds(30), "15h30s");
}

void test_parse_fail_duration_spec() {
    test_parse_fail("1000us100");
    test_parse_fail("100s50");
    test_parse_fail("ms100");
    test_parse_fail("s200");
    test_parse_fail("u300");
    test_parse_fail("123ui");
    test_parse_fail("123s456sm");
    test_parse_fail("456s\x01\\789uu");
    test_parse_fail("50ms6h");
    test_parse_fail("100us2m");
    test_parse_fail("5s2m");
    test_parse_fail("3m10h");
    test_parse_fail("3mm");
    test_parse_fail("9ho");
    test_parse_fail("999mu");
    test_parse_fail("11ns");
    test_parse_fail("10000y");
}

} // namespace
} // namespace test
} // namespace settings
} // namespace espurna

int main(int, char**) {
    using namespace espurna::settings::test;

    UNITY_BEGIN();

    RUN_TEST(test_convert_bool);
    RUN_TEST(test_convert_int);
    RUN_TEST(test_convert_uint);
    RUN_TEST(test_convert_duration);

    RUN_TEST(test_parse_duration);
    RUN_TEST(test_parse_duration_spec);
    RUN_TEST(test_parse_fail_duration_spec);

    return UNITY_END();
}
