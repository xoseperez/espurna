#include <unity.h>
#include <unity_extra.hpp>

#include <Arduino.h>

#include <espurna/mqtt_common.ipp>

namespace espurna {
namespace mqtt {
namespace {

namespace test {

// ref. https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718106 
// '4.7 Topic Names and Topic Filters' for some valid and invalid topic examples

#define TEST_VALID_TOPIC(X)\
    TEST_ASSERT(is_valid_topic(X))

void test_valid_topic() {
    TEST_VALID_TOPIC("a");
    TEST_VALID_TOPIC("bcd/");
    TEST_VALID_TOPIC("/bcd");
    TEST_VALID_TOPIC("/");
    TEST_VALID_TOPIC("////a");
    TEST_VALID_TOPIC("//");
    TEST_VALID_TOPIC("/finance");
    TEST_VALID_TOPIC("sport/tennis/player1");
    TEST_VALID_TOPIC("sport/tennis/player1/ranking");
    TEST_VALID_TOPIC("sport/tennis/player1/score/wimbledon");
    TEST_VALID_TOPIC("sport/tennis/player1/score/wimbledon");
}

#define TEST_INVALID_TOPIC(X)\
    TEST_ASSERT_FALSE(is_valid_topic(X))

void test_invalid_topic() {
    TEST_INVALID_TOPIC("");
    TEST_INVALID_TOPIC("+");
    TEST_INVALID_TOPIC("+/+");
    TEST_INVALID_TOPIC("foo+/+");
    TEST_INVALID_TOPIC("#");
    TEST_INVALID_TOPIC("+/");
    TEST_INVALID_TOPIC("+/");
    TEST_INVALID_TOPIC("/+");
    TEST_INVALID_TOPIC("//+");
    TEST_INVALID_TOPIC("#//+");
}

#define TEST_VALID_TOPIC_FILTER(X)\
    TEST_ASSERT(is_valid_topic_filter(X))

void test_valid_topic_filter() {
    TEST_VALID_TOPIC_FILTER("#");
    TEST_VALID_TOPIC_FILTER("+");
    TEST_VALID_TOPIC_FILTER("+/");
    TEST_VALID_TOPIC_FILTER("+/+");
    TEST_VALID_TOPIC_FILTER("+/foo");
    TEST_VALID_TOPIC_FILTER("+/tennis/#");
    TEST_VALID_TOPIC_FILTER("/");
    TEST_VALID_TOPIC_FILTER("/+");
    TEST_VALID_TOPIC_FILTER("/+///a");
    TEST_VALID_TOPIC_FILTER("//");
    TEST_VALID_TOPIC_FILTER("////+/b/c/+/#");
    TEST_VALID_TOPIC_FILTER("/finance");
    TEST_VALID_TOPIC_FILTER("foo");
    TEST_VALID_TOPIC_FILTER("sport/+/player1");
    TEST_VALID_TOPIC_FILTER("sport/tennis/#");
    TEST_VALID_TOPIC_FILTER("sport/tennis/player1");
    TEST_VALID_TOPIC_FILTER("sport/tennis/player1/ranking");
    TEST_VALID_TOPIC_FILTER("sport/tennis/player1/score/wimbledon");
    TEST_VALID_TOPIC_FILTER("sport/tennis/player1/score/wimbledon");
}

#define TEST_INVALID_TOPIC_FILTER(X)\
    TEST_ASSERT_FALSE(is_valid_topic_filter(X))

void test_invalid_topic_filter() {
    TEST_INVALID_TOPIC_FILTER("");
    TEST_INVALID_TOPIC_FILTER("##");
    TEST_INVALID_TOPIC_FILTER("##sport+a");
    TEST_INVALID_TOPIC_FILTER("++");
    TEST_INVALID_TOPIC_FILTER("+/#//#/+");
    TEST_INVALID_TOPIC_FILTER("#/abcd/efg/+/#");
    TEST_INVALID_TOPIC_FILTER("+sport+a");
    TEST_INVALID_TOPIC_FILTER("a##sport+a");
    TEST_INVALID_TOPIC_FILTER("a#sport+a");
    TEST_INVALID_TOPIC_FILTER("a+sport+a");
    TEST_INVALID_TOPIC_FILTER("as##port+a");
    TEST_INVALID_TOPIC_FILTER("sport#y");
    TEST_INVALID_TOPIC_FILTER("sport+a");
    TEST_INVALID_TOPIC_FILTER("sport/tennis#");
    TEST_INVALID_TOPIC_FILTER("sport/tennis/#/ranking");
    TEST_INVALID_TOPIC_FILTER("##/sport/tennis+");
    TEST_INVALID_TOPIC_FILTER("+##/sport/tennis+");
}

// Mix of a topic filter and a topic itself. Supposed to only contain a single wildcard.
// Wildcard itself can only be multi-level one - #.
//
// Special case of 'foo/#' matching, it would only work for topics starting with 'foo/'
// (unlike the MQTT spec behaviour, where it would match 'foo', 'foo/' and 'foo/bar')

#define TEST_VALID_ROOT_TOPIC(X)\
    TEST_ASSERT(is_valid_root_topic(X))

void test_valid_root_topic() {
    TEST_VALID_ROOT_TOPIC("#");
    TEST_VALID_ROOT_TOPIC("#/tennis/player");
    TEST_VALID_ROOT_TOPIC("tennis/#");
    TEST_VALID_ROOT_TOPIC("////a/b/c/d/#");
    TEST_VALID_ROOT_TOPIC("/finance/#/account");
    TEST_VALID_ROOT_TOPIC("sport/tennis/#");
    TEST_VALID_ROOT_TOPIC("sport/tennis/#/ranking");
}

#define TEST_INVALID_ROOT_TOPIC(X)\
    TEST_ASSERT_FALSE(is_valid_root_topic(X))

void test_invalid_root_topic() {
    TEST_INVALID_ROOT_TOPIC("");
    TEST_INVALID_ROOT_TOPIC("##");
    TEST_INVALID_ROOT_TOPIC("##sport+a");
    TEST_INVALID_ROOT_TOPIC("++");
    TEST_INVALID_ROOT_TOPIC("+/#//#/+");
    TEST_INVALID_ROOT_TOPIC("#/abcd/efg/+/#");
    TEST_INVALID_ROOT_TOPIC("+sport+a");
    TEST_INVALID_ROOT_TOPIC("a##sport+a");
    TEST_INVALID_ROOT_TOPIC("a#sport+a");
    TEST_INVALID_ROOT_TOPIC("a+sport+a");
    TEST_INVALID_ROOT_TOPIC("as##port+a");
    TEST_INVALID_ROOT_TOPIC("sport#y");
    TEST_INVALID_ROOT_TOPIC("sport+a");
    TEST_INVALID_ROOT_TOPIC("sport/tennis#");
    TEST_INVALID_ROOT_TOPIC("#sport/tennis+");
    TEST_INVALID_ROOT_TOPIC("##/sport/tennis+");
    TEST_INVALID_ROOT_TOPIC("+##/sport/tennis+");
}

#define TEST_VALID_SUFFIX(X)\
    TEST_ASSERT(is_valid_suffix(X))

void test_valid_suffix() {
    TEST_VALID_SUFFIX("");
    TEST_VALID_SUFFIX("/set");
    TEST_VALID_SUFFIX("/get");
    TEST_VALID_SUFFIX("/get/pub");
    TEST_VALID_SUFFIX("/get/pub/sub");
}

#define TEST_INVALID_SUFFIX(X)\
    TEST_ASSERT_FALSE(is_valid_suffix(X))

void test_invalid_suffix() {
    TEST_INVALID_SUFFIX("/");
    TEST_INVALID_SUFFIX("/pub/");
    TEST_INVALID_SUFFIX("sub/");
    TEST_INVALID_SUFFIX("//");
    TEST_INVALID_SUFFIX("+/set");
    TEST_INVALID_SUFFIX("get/+");
    TEST_INVALID_SUFFIX("/pub/get/#");
    TEST_INVALID_SUFFIX("/pub/+/sub");
}

#define __PREPARE_WILDCARD(LEN, RESULT, EXPECTED, FILTER, TOPIC)\
    const auto LEN = __builtin_strlen(EXPECTED);\
    const auto RESULT = match_wildcard((FILTER), (TOPIC), filter_wildcard(FILTER))

#define TEST_VALID_MATCH_WILDCARD(EXPECTED, FILTER, TOPIC)\
    ([]() {\
        __PREPARE_WILDCARD(len, result, (EXPECTED), (FILTER), (TOPIC));\
        TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, result.data(), "No pattern match");\
        if (len == 0) {\
            TEST_ASSERT_EQUAL(0, result.length());\
        } else {\
            TEST_ASSERT_EQUAL_STRING_VIEW((EXPECTED), result);\
        }\
    })()

void test_valid_match_wildcard() {
     TEST_VALID_MATCH_WILDCARD("", "#/set", "/set");
     TEST_VALID_MATCH_WILDCARD("", "sport/#", "sport/");
     TEST_VALID_MATCH_WILDCARD("", "sport/#/", "sport//");
     TEST_VALID_MATCH_WILDCARD("", "sport/+", "sport/");
     TEST_VALID_MATCH_WILDCARD("/", "sport/#", "sport//");
     TEST_VALID_MATCH_WILDCARD("action", "device/#/set", "device/action/set");
     TEST_VALID_MATCH_WILDCARD("relay", "device/+/set", "device/relay/set");
     TEST_VALID_MATCH_WILDCARD("relay/0", "device/#/set", "device/relay/0/set");
     TEST_VALID_MATCH_WILDCARD("sport", "#", "sport");
     TEST_VALID_MATCH_WILDCARD("sport", "+", "sport");
     TEST_VALID_MATCH_WILDCARD("sport/tennis", "#", "sport/tennis");
     TEST_VALID_MATCH_WILDCARD("value", "#/set", "value/set");
     TEST_VALID_MATCH_WILDCARD("value", "some/kind/of/#/set", "some/kind/of/value/set");
}

#define TEST_INVALID_MATCH_WILDCARD(FILTER, TOPIC)\
    ([]() {\
        __PREPARE_WILDCARD(len, result, "", FILTER, TOPIC);\
        TEST_ASSERT_EQUAL(0, len);\
        TEST_ASSERT_EQUAL(0, result.length());\
        TEST_ASSERT_EQUAL(nullptr, result.data());\
    })()

#define TEST_INVALID_MATCH_WILDCARD_RAW(FILTER, TOPIC, WILDCARD)\
    ([]() {\
        const auto result = match_wildcard((FILTER), (TOPIC), (WILDCARD));\
        TEST_ASSERT_EQUAL(0, result.length());\
        TEST_ASSERT_EQUAL(nullptr, result.data());\
    })()

void test_invalid_match_wildcard() {
     TEST_INVALID_MATCH_WILDCARD("", "");
     TEST_INVALID_MATCH_WILDCARD_RAW("", "", '+');
     TEST_INVALID_MATCH_WILDCARD_RAW("", "", '#');

     TEST_INVALID_MATCH_WILDCARD("", "foo");
     TEST_INVALID_MATCH_WILDCARD_RAW("", "foo", '+');
     TEST_INVALID_MATCH_WILDCARD_RAW("", "foo", '#');

     TEST_INVALID_MATCH_WILDCARD("bar", "");
     TEST_INVALID_MATCH_WILDCARD("#", "");
     TEST_INVALID_MATCH_WILDCARD("+", "");

     TEST_INVALID_MATCH_WILDCARD("/device/#/set", "device/relay/0/set");
     TEST_INVALID_MATCH_WILDCARD("device/+/set", "device/relay/0/set");
}

} // namespace test

} // namespace
} // namespace mqtt
} // namespace espurna

int main(int, char**) {
    UNITY_BEGIN();

    using namespace espurna::mqtt::test;

    RUN_TEST(test_valid_topic);
    RUN_TEST(test_invalid_topic);

    RUN_TEST(test_valid_topic_filter);
    RUN_TEST(test_invalid_topic_filter);

    RUN_TEST(test_valid_root_topic);
    RUN_TEST(test_invalid_root_topic);

    RUN_TEST(test_valid_suffix);
    RUN_TEST(test_invalid_suffix);

    RUN_TEST(test_valid_match_wildcard);
    RUN_TEST(test_invalid_match_wildcard);

    return UNITY_END();
}
