#include <Arduino.h>
#include <unity.h>

#include <espurna/datetime.h>

void test_format_local_tz() {
    const struct {
        int offset_minutes;
        const char* expected;
    } cases[] {
        { 0, "2026-01-09T12:00:00Z" },
        { -300, "2026-01-09T07:00:00-05:00" },
        { -210, "2026-01-09T08:30:00-03:30" },
        { -30, "2026-01-09T11:30:00-00:30" },
        { -540, "2026-01-09T03:00:00-09:00" },
        { 540, "2026-01-09T21:00:00+09:00" },
        { 9, "2026-01-09T12:09:00+00:09" },
        { -9, "2026-01-09T11:51:00-00:09" },
        { 345, "2026-01-09T17:45:00+05:45" },
        { 600, "2026-01-09T22:00:00+10:00" },
        { 840, "2026-01-10T02:00:00+14:00" },
        { -720, "2026-01-09T00:00:00-12:00" }
    };

    for (const auto& entry : cases) {
        espurna::datetime::Context context{};
        context.timestamp = 1767960000;
        const time_t local = context.timestamp + entry.offset_minutes * 60;
        gmtime_r(&context.timestamp, &context.utc);
        gmtime_r(&local, &context.local);

        const auto result = espurna::datetime::format_local_tz(context);
        TEST_ASSERT_EQUAL_STRING(entry.expected, result.c_str());
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_format_local_tz);
    return UNITY_END();
}
