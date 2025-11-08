#include <unity.h>
#include <unity_extra.hpp>

#include <Arduino.h>

#include <espurna/led_pattern.ipp>

namespace espurna {
namespace test {
namespace {

#define EXTRA_DETAIL(MSG)\
    detail = (MSG);\
    detail += " at ";\
    detail += __FUNCTION__;\
    detail += ":";\
    detail += line;\
    UNITY_SET_DETAIL(detail.c_str())

void test_object() {
    const led::Delay Delays[] {
        {
            .on = duration::Milliseconds(250),
            .off = duration::Milliseconds(750),
            .repeats = 0,
        },
        {
            .on = duration::Milliseconds(900),
            .off = duration::Milliseconds(100),
            .repeats = 0,
        },
        {
            .on = duration::Milliseconds(100),
            .off = duration::Milliseconds(4900),
            .repeats = 5,
        },
    };

    led::Pattern a;

    a.add(Delays[0]);
    TEST_ASSERT_EQUAL(1, a.size());

    const auto check = [&](const led::Pattern& pattern, int line = __builtin_LINE()) {
        static String detail;
        EXTRA_DETAIL("pattern delays");

        const auto& delays = pattern.delays();

        auto it = delays.begin();
        auto end = delays.end();

        for (size_t index = 0; index < delays.size() && it != end; ++index) {
            TEST_ASSERT_EQUAL(
                Delays[index].on.count(),
                (*it).on.count());
            TEST_ASSERT_EQUAL(
                Delays[index].off.count(),
                (*it).off.count());
            TEST_ASSERT_EQUAL(
                Delays[index].repeats,
                (*it).repeats);
            ++it;
        }

        UNITY_CLR_DETAILS();
    };

    check(a);

    led::Pattern b;
    TEST_ASSERT_EQUAL(0, b.size());

    b = a;
    TEST_ASSERT_EQUAL(1, a.size());
    TEST_ASSERT_EQUAL(1, b.size());

    check(a);
    check(b);

    b.add(Delays[1]);
    TEST_ASSERT_EQUAL(1, a.size());
    TEST_ASSERT_EQUAL(2, b.size());

    led::Pattern c(std::move(b));
    TEST_ASSERT_EQUAL(1, a.size());
    TEST_ASSERT_EQUAL(0, b.size());
    TEST_ASSERT_EQUAL(2, c.size());

    check(c);

    led::Pattern d(c);
    d.add(Delays[2]);

    TEST_ASSERT_EQUAL(1, a.size());
    TEST_ASSERT_EQUAL(0, b.size());
    TEST_ASSERT_EQUAL(2, c.size());
    TEST_ASSERT_EQUAL(3, d.size());

    check(d);
}

void test_serialize() {
    led::Pattern p;
    TEST_ASSERT_EQUAL_STRING("", p.toString().c_str());
    p.add(duration::Milliseconds(100), duration::Milliseconds(200), 30);
    TEST_ASSERT_EQUAL_STRING("100,200,30", p.toString().c_str());
    p.add(duration::Milliseconds(500), duration::Milliseconds(1234), 5);
    TEST_ASSERT_EQUAL_STRING("100,200,30 500,1234,5", p.toString().c_str());
    p.add(duration::Milliseconds(0), duration::Milliseconds(0), 0);
    TEST_ASSERT_EQUAL_STRING("100,200,30 500,1234,5 0,0", p.toString().c_str());
}

struct Runner {
    struct Expected {
        led::Delay delay;
        size_t iterations;
        bool ends;
    };

    bool status{ false };
    led::Pattern pattern;

    void run(std::initializer_list<Expected>, int line = __builtin_LINE());

    void reset(std::initializer_list<led::Delay> delays) {
        pattern = led::Pattern{};
        for (const auto& delay : delays) {
            pattern.add(delay);
        }
    }

    led::Sequence make_sequence() const {
        return pattern.make_sequence();
    }

    template <typename ...Args>
    void add(Args&& ...args) {
        pattern.add(std::forward<Args>(args)...);
    }
};

void Runner::run(std::initializer_list<Expected> list, int line) {
    static String detail;

    auto sequence = pattern.make_sequence();

    for (const auto& expected : list) {
        TEST_ASSERT_GREATER_OR_EQUAL(1, expected.iterations);
        for (size_t i = 0 ; i < expected.iterations; ++i) {
            EXTRA_DETAIL(String("iteration #") + i);

            const auto changed = sequence.run(
                [&]() {
                    return status;
                },
                [&](bool current_status, led::Duration duration) {
                    TEST_ASSERT_EQUAL(current_status, status);
                    if (current_status) {
                        TEST_ASSERT_EQUAL(expected.delay.on.count(), duration.count());
                    } else {
                        TEST_ASSERT_EQUAL(expected.delay.off.count(), duration.count());
                    }

                    return true;
                });

            if (!expected.ends) {
                TEST_ASSERT(changed);
            } else {
                if (i == (expected.iterations - 1)) {
                    TEST_ASSERT_FALSE(changed);
                } else {
                    TEST_ASSERT_TRUE(changed);
                }
            }

            status = !status;
        }
    }

    UNITY_CLR_DETAILS();
}

void test_repeats() {
    // infinite delay value that continiously repeat
    const auto infinite = led::Delay{
        .on = duration::Milliseconds(100),
        .off = duration::Milliseconds(4900),
        .repeats = 0,
    };

    Runner r;
    r.add(infinite);
    r.run({
        {infinite, 123, false},
    });

    // finite delay that should repeat exactly 10 times
    const auto one = led::Delay{
        .on = duration::Milliseconds(250),
        .off = duration::Milliseconds(1000),
        .repeats = 10,
    };

    r.reset({one});
    r.run({
        {one, one.repeats, true},
    });

    r.reset({one, infinite});
    r.run({
        {one, one.repeats, false},
        {infinite, 25, false},
    });

    // another finite delay
    const auto two = led::Delay{
        .on = duration::Milliseconds(100),
        .off = duration::Milliseconds(900),
        .repeats = 5,
    };

    r.reset({one, two});
    r.run({
        {one, one.repeats, false},
        {two, two.repeats, true},
    });

    // special case for repeating everything in the pattern preceding this '0,0' delay value
    const auto three = led::Delay{
        .on = duration::Milliseconds(0),
        .off = duration::Milliseconds(0),
        .repeats = 0,
    };

    r.reset({two, one, three});
    r.run({
        {two, two.repeats, false},
        {one, one.repeats, false},
        {two, two.repeats, false},
        {one, one.repeats, false},
        {two, two.repeats, false},
        {one, one.repeats, false},
        {two, two.repeats, false},
        {one, one.repeats, false},
        {two, two.repeats, false},
        {one, one.repeats, false},
    });
}

} // namespace
} // namespace test
} // namespace espurna

int main(int, char**) {
    UNITY_BEGIN();

    using namespace espurna::test;
    RUN_TEST(test_object);
    RUN_TEST(test_serialize);
    RUN_TEST(test_repeats);

    return UNITY_END();
}
