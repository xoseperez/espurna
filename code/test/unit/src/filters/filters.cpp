#include <unity.h>

#include <Arduino.h>
#include <StreamString.h>
#include <ArduinoJson.h>

#include <espurna/filters/LastFilter.h>
#include <espurna/filters/MaxFilter.h>
#include <espurna/filters/MedianFilter.h>
#include <espurna/filters/MovingAverageFilter.h>
#include <espurna/filters/SumFilter.h>

#include <algorithm>

namespace espurna {
namespace test {
namespace {

void test_last() {
    auto filter = LastFilter();

    TEST_ASSERT(filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    filter.resize(123);

    TEST_ASSERT(filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    filter.update(123.4);
    TEST_ASSERT_EQUAL_DOUBLE(123.4, filter.value());

    filter.update(456.7);
    TEST_ASSERT_EQUAL_DOUBLE(456.7, filter.value());

    filter.update(789.0);
    TEST_ASSERT_EQUAL_DOUBLE(789.0, filter.value());

    filter.reset();
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());
}

void test_max() {
    auto filter = MaxFilter();

    TEST_ASSERT(filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    filter.resize(567);

    TEST_ASSERT(filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    filter.update(5.0);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, filter.value());

    filter.update(10.0);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, filter.value());

    filter.update(15.0);
    TEST_ASSERT_EQUAL_DOUBLE(15.0, filter.value());

    filter.update(10.0);
    TEST_ASSERT_EQUAL_DOUBLE(15.0, filter.value());

    filter.update(-10.0);
    TEST_ASSERT_EQUAL_DOUBLE(15.0, filter.value());

    filter.update(-15.0);
    TEST_ASSERT_EQUAL_DOUBLE(15.0, filter.value());

    filter.update(0.0);
    TEST_ASSERT_EQUAL_DOUBLE(15.0, filter.value());

    filter.reset();
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());
}

void test_median() {
    auto filter = MedianFilter();
    TEST_ASSERT(!filter.status());

    const double one[] {4., 3., 5., 6., 2., 2., 3., 4., 7., 9.};
    filter.resize(std::size(one));

    TEST_ASSERT(filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    for (const auto& sample : one) {
        filter.update(sample);
    }

    TEST_ASSERT_EQUAL_DOUBLE(4.0, filter.value());

    const double two[] {6., 6.1, 6.2, 6.3, 6.4, 6.5, 2.5, 4.5, 2.6, 2.5, 2.4};
    filter.resize(std::size(two));

    TEST_ASSERT(filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(9, filter.value());

    for (const auto& sample : two) {
        filter.update(sample);
    }

    TEST_ASSERT_EQUAL_DOUBLE(4.97, filter.value());

    const double three[] {2.4, 2.4};
    filter.resize(std::size(three));

    TEST_ASSERT(filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(2.4, filter.value());

    for (const auto& sample : three) {
        filter.update(sample);
    }

    TEST_ASSERT_EQUAL_DOUBLE(2.4, filter.value());
}

void test_moving_average() {
    auto filter = MovingAverageFilter();

    TEST_ASSERT(!filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    const double samples[] {22., 22.3, 22.1, 22.1, 22.1, 22.0, 22.5, 22.1};
    filter.resize(std::size(samples));

    TEST_ASSERT(filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    for (const auto& sample : samples) {
        filter.update(sample);
    }

    TEST_ASSERT_EQUAL_DOUBLE(22.15, filter.value());
}

void test_sum() {
    auto filter = SumFilter();

    TEST_ASSERT(filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    const double samples[] {20., 20.1, 13., 10., 5., 14., 29., 32.};
    filter.resize(std::size(samples));

    TEST_ASSERT(filter.status());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    for (const auto& sample : samples) {
        filter.update(sample);
    }

    TEST_ASSERT_EQUAL_DOUBLE(143.1, filter.value());

    filter.reset();
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    filter.update(-15.0);
    filter.update(30.0);
    filter.update(-15.0);
    filter.update(1.0);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, filter.value());
}

} // namespace
} // namespace test
} // namespace espurna

int main(int, char**) {
    UNITY_BEGIN();
    using namespace espurna::test;
    RUN_TEST(test_last);
    RUN_TEST(test_max);
    RUN_TEST(test_median);
    RUN_TEST(test_moving_average);
    RUN_TEST(test_sum);
    return UNITY_END();
}

