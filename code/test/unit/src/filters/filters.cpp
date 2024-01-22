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

    TEST_ASSERT_EQUAL(1, filter.capacity());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    filter.resize(123);

    TEST_ASSERT_EQUAL(1, filter.capacity());
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

    TEST_ASSERT_EQUAL(1, filter.capacity());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    filter.resize(567);

    TEST_ASSERT_EQUAL(1, filter.capacity());
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
    TEST_ASSERT_EQUAL(0, filter.capacity());

    const double samples[] {1., 2., 2., 3., 4., 7., 9.};
    filter.resize(std::size(samples));

    TEST_ASSERT_EQUAL(std::size(samples), filter.capacity());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    for (const auto& sample : samples) {
        filter.update(sample);
    }

    TEST_ASSERT_EQUAL_DOUBLE(3.6, filter.value());
}

void test_moving_average() {
    // TODO
}

void test_sum() {
    auto filter = SumFilter();

    TEST_ASSERT_EQUAL(1, filter.capacity());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, filter.value());

    const double samples[] {20., 20.1, 13., 10., 5., 14., 29., 32.};
    filter.resize(std::size(samples));

    TEST_ASSERT_EQUAL(1, filter.capacity());
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

