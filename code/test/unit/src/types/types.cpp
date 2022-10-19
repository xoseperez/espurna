#include <unity.h>
#include <Arduino.h>

#include <espurna/types.h>

namespace espurna {
namespace test {
namespace {

void test_view() {
    StringView view{"123456789"};
    TEST_ASSERT_EQUAL(9, view.length());
    TEST_ASSERT(view.c_str() != nullptr);

    const char expected[] = "123456789";
    TEST_ASSERT_EQUAL(__builtin_strlen(expected), view.length());
    TEST_ASSERT_EQUAL_CHAR_ARRAY(
        expected, view.begin(), view.length());
}

void test_view_convert() {
    const String origin("12345");
    StringView view(origin);

    TEST_ASSERT(origin.begin() == view.begin());
    TEST_ASSERT(origin.end() == view.end());
    TEST_ASSERT_EQUAL(origin.length(), view.length());

    auto copy = view.toString();
    TEST_ASSERT(view.equals(copy));

    TEST_ASSERT(origin.begin() != copy.begin());
    TEST_ASSERT_EQUAL(origin.length(), copy.length());
    TEST_ASSERT_EQUAL_CHAR_ARRAY(
        origin.begin(), copy.begin(), copy.length());


    StringView copy_view(copy);
    TEST_ASSERT_EQUAL(view.length(), copy_view.length());
    TEST_ASSERT(view.equals(copy_view));
}

void test_callback_empty() {
    Callback callback;
    TEST_ASSERT(callback.isEmpty());
}

void test_callback_lambda() {
    static int once = 0;
    Callback callback([]() {
        ++once;
    });

    TEST_ASSERT(callback.isSimple());

    callback();
    TEST_ASSERT_EQUAL(1, once);
}

int external { 0 };

void increment_external() {
    ++external;
    ++external;
}

void test_callback_simple() {
    TEST_ASSERT_EQUAL(0, external);

    Callback callback(increment_external);
    TEST_ASSERT(callback.isSimple());

    callback();
    TEST_ASSERT_EQUAL(2, external);
}

void test_callback_capture() {
    int value = 0;

    std::vector<Callback> callbacks;
    callbacks.resize(3, [&]() {
        ++value;
    });

    TEST_ASSERT_EQUAL(3, callbacks.size());
    TEST_ASSERT(callbacks[0].isWrapped());
    TEST_ASSERT(callbacks[1].isWrapped());
    TEST_ASSERT(callbacks[2].isWrapped());

    for (const auto& callback : callbacks) {
        callback();
    }
    TEST_ASSERT_EQUAL(3, value);
}

void test_callback_capture_copy() {
    int value = 0;
    Callback original([&]() {
        ++value;
        ++value;
        ++value;
        ++value;
    });

    TEST_ASSERT(original.isWrapped());

    auto copy = original;
    TEST_ASSERT(original.isWrapped());
    TEST_ASSERT(copy.isWrapped());

    original();
    TEST_ASSERT_EQUAL(4, value);

    copy();
    TEST_ASSERT_EQUAL(8, value);
}

void test_callback_capture_move() {
    int value = 0;
    Callback original([&]() {
        ++value;
        ++value;
        ++value;
        ++value;
    });

    TEST_ASSERT(original.isWrapped());

    Callback moved(std::move(original));
    TEST_ASSERT(original.isEmpty());
    TEST_ASSERT(moved.isWrapped());

    original();
    TEST_ASSERT_EQUAL(0, value);

    moved();
    TEST_ASSERT_EQUAL(4, value);
}

void test_callback_assign() {
    int value = 0;
    Callback original([&]() {
        value += 10;
    });

    TEST_ASSERT(original.isWrapped());

    Callback copy;
    TEST_ASSERT(copy.isEmpty());

    copy = original;
    TEST_ASSERT(copy.isWrapped());

    original();
    TEST_ASSERT_EQUAL(10, value);

    copy();
    TEST_ASSERT_EQUAL(20, value);

    Callback moved;
    TEST_ASSERT(moved.isEmpty());

    moved = std::move(original);
    TEST_ASSERT(original.isEmpty());
    TEST_ASSERT(moved.isWrapped());

    original();
    TEST_ASSERT_EQUAL(20, value);

    copy();
    TEST_ASSERT_EQUAL(30, value);

    moved();
    TEST_ASSERT_EQUAL(40, value);
}

void test_callback_swap() {
    int output = 0;

    Callback good([&]() {
        output = 5;
    });

    Callback bad([&]() {
        output = 10;
    });

    good.swap(bad);
    TEST_ASSERT_EQUAL(0, output);

    bad();
    TEST_ASSERT_EQUAL(5, output);

    good();
    TEST_ASSERT_EQUAL(10, output);
}

} // namespace
} // namespace test
} // namespace espurna

int main(int, char**) {
    UNITY_BEGIN();

    using namespace espurna::test;
	RUN_TEST(test_view);
	RUN_TEST(test_view_convert);
	RUN_TEST(test_callback_empty);
	RUN_TEST(test_callback_simple);
	RUN_TEST(test_callback_lambda);
	RUN_TEST(test_callback_capture);
	RUN_TEST(test_callback_capture_copy);
	RUN_TEST(test_callback_capture_move);
    RUN_TEST(test_callback_assign);
	RUN_TEST(test_callback_swap);

    return UNITY_END();
}
