#include <unity.h>
#include <Arduino.h>

#include <espurna/settings_convert.h>

namespace espurna {
namespace settings {
namespace internal {

// TODO: convert() overload for any std::chrono::duration<>

using CustomMinutes = std::chrono::duration<float, std::ratio<60>>;

template <>
CustomMinutes convert(const String& value) {
    return duration_convert::unchecked_parse<CustomMinutes>(value);
}

using CustomHours = std::chrono::duration<float, std::ratio<3600>>;

template <>
CustomHours convert(const String& value) {
    return duration_convert::unchecked_parse<CustomHours>(value);
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

#define TEST_ASSERT_EQUAL_CHRONO(LHS, RHS)\
    ([]() {\
        using common_type = std::common_type_t<decltype(LHS), decltype(RHS)>;\
        TEST_ASSERT_EQUAL(common_type(LHS).count(), common_type(RHS).count());\
    })();

void test_convert_duration() {
    TEST_ASSERT_EQUAL_CHRONO(duration::Seconds(5),
            internal::convert<duration::Seconds>("5"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Seconds(44),
            internal::convert<duration::Seconds>("44s"));

    TEST_ASSERT_EQUAL_CHRONO(duration::Seconds(60),
            internal::convert<duration::Seconds>("1m"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Minutes(1),
            internal::convert<duration::Minutes>("60s"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Hours(1),
            internal::convert<duration::Hours>("1h"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Minutes(60),
            internal::convert<duration::Minutes>("60m"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Hours(1),
            internal::convert<duration::Hours>("3600s"));

    TEST_ASSERT_EQUAL_CHRONO(duration::Seconds(90),
            internal::convert<duration::Seconds>("1m30s"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Milliseconds(1000),
            internal::convert<duration::Seconds>("1"));

    TEST_ASSERT_EQUAL_CHRONO(duration::Seconds(30),
            internal::convert<internal::CustomMinutes>("0.5"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Minutes(15),
            internal::convert<internal::CustomHours>("0.25"));

    TEST_ASSERT_EQUAL_CHRONO(duration::Minutes(3),
            internal::convert<duration::Minutes>("3.5"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Minutes(1),
            internal::convert<duration::Minutes>("65s"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Hours(2),
            internal::convert<duration::Hours>("121m"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Hours(0),
            internal::convert<duration::Hours>("59m"));
    TEST_ASSERT_EQUAL_CHRONO(duration::Seconds(0),
            internal::convert<duration::Minutes>("5s"));
}

void test_parse_duration() {
    using internal::duration_convert::parse;

    TEST_ASSERT(parse("6", std::milli{}).ok);
    TEST_ASSERT_EQUAL_CHRONO(duration::Milliseconds(6),
            parse("6", std::milli{}).value.microseconds);

    TEST_ASSERT(parse("11", std::micro{}).ok);
    TEST_ASSERT_EQUAL_CHRONO(duration::Microseconds(11),
            parse("11", std::micro{}).value.microseconds);

    TEST_ASSERT(parse("15", std::ratio<1>{}).ok);
    TEST_ASSERT_EQUAL_CHRONO(duration::Seconds(15),
            parse("15", std::ratio<1>{}).value.seconds);

    TEST_ASSERT(parse("21", std::ratio<60>{}).ok);
    TEST_ASSERT_EQUAL_CHRONO(duration::Minutes(21),
            parse("21", std::ratio<60>{}).value.seconds);

    TEST_ASSERT(parse("46", std::ratio<3600>{}).ok);
    TEST_ASSERT_EQUAL_CHRONO(duration::Hours(46),
            parse("46", std::ratio<3600>{}).value.seconds);
}

void test_parse_duration_spec() {
    using internal::duration_convert::parse;

    TEST_ASSERT(!parse("5s2m", std::milli{}).ok);
    TEST_ASSERT(!parse("3m10h", std::milli{}).ok);
    TEST_ASSERT(!parse("3mm", std::milli{}).ok);
    TEST_ASSERT(!parse("9hh", std::milli{}).ok);
    TEST_ASSERT(!parse("11ss", std::milli{}).ok);
    TEST_ASSERT(!parse("10000y", std::milli{}).ok);

    TEST_ASSERT(parse("7h", std::milli{}).ok);
    TEST_ASSERT_EQUAL_CHRONO(duration::Hours(7),
            parse("7h", std::milli{}).value.seconds);

    TEST_ASSERT(parse("15h30s", std::milli{}).ok);
    TEST_ASSERT_EQUAL_CHRONO(duration::Hours(15) + duration::Seconds(30),
            parse("15h30s", std::milli{}).value.seconds);

    TEST_ASSERT_EQUAL_CHRONO(duration::Seconds(112),
            parse("0h0m112s", std::milli{}).value.seconds);

    TEST_ASSERT_EQUAL_CHRONO(duration::Minutes(5),
            parse("5m", std::milli{}).value.seconds);
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

    return UNITY_END();
}
