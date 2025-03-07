#include <unity.h>

#include <Arduino.h>
#include <StreamString.h>
#include <ArduinoJson.h>

#define DEVICE "NODEMCU"
#define MANUFACTURER "LOLIN"

#include <espurna/libs/StreamEcho.h>
#include <espurna/utils.h>

#include <espurna/sensor_emon.ipp>

// TODO is ..._SUPPORT wrapping necessary inside of sensor includes?
// TODO ..._PORT should not be used in the class itself?

// TODO ignore -Wunused-value that comes up here from interrupts() / noInterrupts() usage
#undef xt_rsil
#define xt_rsil(X)

#define SENSOR_SUPPORT 1
#define CSE7766_SUPPORT 1
#define A02YYU_SUPPORT 1
#define DHT_SUPPORT 1

#include <espurna/config/sensors.h>
#include <espurna/sensors/CSE7766Sensor.h>
#include <espurna/sensors/A02YYUSensor.h>
#include <espurna/sensors/DHTSensor.h>

#include <memory>
#include <vector>

namespace espurna {
namespace test {
namespace {

void test_cse7766_data() {
    constexpr size_t PacketSize = 24;

    constexpr uint8_t data[] {
        // some invalid data at the start (e.g. uart ram buffer contents at boot)
        0x00, 0x12, 0x21,
        // invalid calibration state (may happen, but probably should not)
        0xAA, 0x5A, 0xFF, 0xFF, 0xFF, 0xA1, 0xA2, 0xA3, 0xFF, 0xFF, 0xFF, 0xB1, 0xB2, 0xB3, 0xFF, 0xFF, 0xFF, 0xC1, 0xC2, 0xC3, 0xD1, 0xD2, 0xD3, 0xAF,
        // actual payload, with load
        0x55, 0x5A, 0x02, 0xE9, 0x50, 0x00, 0x03, 0x31, 0x00, 0x3E, 0x9E, 0x00, 0x0D, 0x30, 0x4F, 0x44, 0xF8, 0x00, 0x12, 0x65, 0xF1, 0x81, 0x76, 0x72,
        // repeated, but broken
        0x55, 0x5A, 0x02, 0xE9, 0x50, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0x0D, 0x30, 0xFE, 0xFE, 0xF8, 0x00, 0x12, 0x65, 0xF1, 0x81, 0x76, 0x72,
        // some invalid data in the middle
        0xDE, 0xF0, 0x0D,
        // another payload, without load
        0xF2, 0x5A, 0x02, 0xE9, 0x50, 0x00, 0x03, 0x2B, 0x00, 0x3E, 0x9E, 0x02, 0xD7, 0x7C, 0x4F, 0x44, 0xF8, 0xCF, 0xA5, 0x5D, 0xE1, 0xB3, 0x2A, 0xB4,
        // repeated, but with an error
        0xF5, 0x5A, 0x02, 0xE9, 0x50, 0x00, 0x03, 0x2B, 0x00, 0x3E, 0x9E, 0x02, 0xD7, 0x7C, 0x4F, 0x44, 0xF8, 0xCF, 0xA5, 0x5D, 0xE1, 0xB3, 0x2A, 0xB4,
        // something remaining in the buffer
        0xFF, 0xFE,
    };

    StreamEcho port;
    port.write(&data[0], std::size(data));

    auto ptr = std::make_unique<CSE7766Sensor>();
    ptr->setPort(&port);
    ptr->begin();

    TEST_ASSERT_EQUAL(SENSOR_ERROR_OK, ptr->error());

    size_t remaining = std::size(data);

    std::vector<double> values;
    values.reserve(ptr->count());

#define TEST_VALUES(DATA)\
    ([&]() {\
\
        values.clear();\
        for (unsigned char index = 0; index < ptr->count(); ++index) {\
            if (ptr->type(index) == MAGNITUDE_ENERGY) {\
                continue;\
            }\
            values.push_back(\
                roundTo(ptr->value(index), 3));\
        }\
\
        TEST_ASSERT_EQUAL(values.size(), std::size(DATA));\
        TEST_ASSERT_EQUAL_DOUBLE_ARRAY((DATA), values.data(), std::size(DATA));\
    })()

    // ignore initial data and stumble on the calibration error first

    ptr->tick();
    TEST_ASSERT_EQUAL(SENSOR_ERROR_CALIBRATION, ptr->error());

    remaining -= 3;
    remaining -= PacketSize;

    TEST_ASSERT_EQUAL(remaining, port.available());

    // consume initial payload

    const double with_load[] {
        4.748,
        233.537,
        1103.207,
        112.099,
        1108.887,
        99.488,
    };

    ptr->tick();
    TEST_ASSERT_EQUAL(SENSOR_ERROR_OK, ptr->error());

    remaining -= PacketSize;

    TEST_ASSERT_EQUAL(remaining, port.available());
    TEST_VALUES(with_load);

    // consume invalid payload

    ptr->tick();
    TEST_ASSERT_EQUAL(SENSOR_ERROR_CRC, ptr->error());

    remaining -= PacketSize;
    TEST_ASSERT_EQUAL(remaining, port.available());

    // skip invalid data and consume the second valid payload

    const double without_load[] {
        0,
        235.265,
        0.,
        0.,
        0.,
        100.,
    };

    ptr->tick();
    TEST_ASSERT_EQUAL(SENSOR_ERROR_OK, ptr->error());

    remaining -= 3;
    remaining -= PacketSize;

    TEST_ASSERT_EQUAL(remaining, port.available());

    TEST_VALUES(without_load);

#undef TEST_VALUES

    // should consume the remaining payload and the rest of the buffer

    ptr->tick();
    TEST_ASSERT_EQUAL(SENSOR_ERROR_VALUE, ptr->error());

    remaining -= PacketSize;

    TEST_ASSERT_EQUAL(remaining, port.available());

    ptr->tick();
    TEST_ASSERT_EQUAL(SENSOR_ERROR_OK, ptr->error());

    remaining -= 2;

    TEST_ASSERT_EQUAL(0, remaining);
    TEST_ASSERT_EQUAL(0, port.available());
}

void test_a02yyu_data() {
    // baseline test for port reader
    StreamEcho port;

    auto ptr = std::make_unique<A02YYUSensor>();
    ptr->setPort(&port);

    TEST_ASSERT_EQUAL(SENSOR_ERROR_OK, ptr->error());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, ptr->value(0));

    // lhs part stores intermediate result, but does not error out
    constexpr uint8_t one[] {0xff, 0x07};
    port.write(&one[0], std::size(one));

    ptr->tick();

    TEST_ASSERT_EQUAL(SENSOR_ERROR_OK, ptr->error());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, ptr->value(0));

    port.write(&one[0], std::size(one));

    ptr->tick();

    TEST_ASSERT_EQUAL(SENSOR_ERROR_OK, ptr->error());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, ptr->value(0));

    // then rhs part finishes up and yields the value
    constexpr uint8_t two[] {0xa1, 0xa7};
    port.write(&two[0], std::size(two));

    ptr->tick();

    TEST_ASSERT_EQUAL(SENSOR_ERROR_OK, ptr->error());
    TEST_ASSERT_EQUAL_DOUBLE(1.953, ptr->value(0));

    // some sampled data to verify possible outputs
    // tnx @toniSg for the captures
    static constexpr std::array<uint8_t, 4> samples[] = {
        { 0xFF, 0x2, 0xBF, 0xC0 }, // 703
        { 0xFF, 0x2, 0xB6, 0xB7 }, // 694
        { 0xFF, 0x2, 0xB7, 0xB8 }, // 695
        { 0xFF, 0x2, 0xC3, 0xC4 }, // 707
        { 0xFF, 0x2, 0xB2, 0xB3 }, // 690
        { 0xFF, 0x2, 0xC4, 0xC5 }, // 708
        { 0xFF, 0x2, 0xB2, 0xB3 }, // 690
        { 0xFF, 0x2, 0xBF, 0xC0 }, // 703
        { 0xFF, 0x2, 0xC8, 0xC9 }, // 712
        { 0xFF, 0x2, 0xC4, 0xC5 }, // 708
        { 0xFF, 0x2, 0xB2, 0xB3 }, // 690
        { 0xFF, 0x2, 0xB3, 0xB4 }, // 691
        { 0xFF, 0x2, 0xB7, 0xB8 }, // 695
        { 0xFF, 0x2, 0xBF, 0xC0 }, // 703
        { 0xFF, 0x2, 0xB7, 0xB8 }, // 695
        { 0xFF, 0x2, 0xB2, 0xB3 }, // 690
        { 0xFF, 0x2, 0xF1, 0xF2 }, // 753
        { 0xFF, 0x2, 0xFD, 0xFE }, // 765
        { 0xFF, 0x2, 0xF9, 0xFA }, // 761
        { 0xFF, 0x2, 0xF9, 0xFA }, // 761
        { 0xFF, 0x2, 0xCB, 0xCC }, // 715
        { 0xFF, 0x2, 0x13, 0x14 }, // 531
        { 0xFF, 0x1, 0x30, 0x30 }, // 304
        { 0xFF, 0x0, 0xE5, 0xE4 }, // 229
        { 0xFF, 0x1, 0x02, 0x02 }, // 258
        { 0xFF, 0x1, 0x03, 0x03 }, // 259
        { 0xFF, 0x1, 0x04, 0x04 }, // 260
        { 0xFF, 0x1, 0x06, 0x06 }, // 262
        { 0xFF, 0x1, 0x04, 0x04 }, // 260
        { 0xFF, 0x1, 0x04, 0x04 }, // 260
        { 0xFF, 0x1, 0x05, 0x05 }, // 261
        { 0xFF, 0x1, 0x07, 0x07 }, // 263
        { 0xFF, 0x1, 0x0B, 0x0B }, // 267
        { 0xFF, 0x1, 0x07, 0x07 }, // 263
        { 0xFF, 0x1, 0x04, 0x04 }, // 260
        { 0xFF, 0x1, 0x04, 0x04 }, // 260
        { 0xFF, 0x1, 0x05, 0x05 }, // 261
        { 0xFF, 0x1, 0x06, 0x06 }, // 262
        { 0xFF, 0x1, 0x05, 0x05 }, // 261
        { 0xFF, 0x1, 0x0A, 0x0A }, // 266
        { 0xFF, 0x1, 0x06, 0x06 }, // 262
        { 0xFF, 0x1, 0x0A, 0x0A }, // 266
        { 0xFF, 0x1, 0x06, 0x06 }, // 262
        { 0xFF, 0x1, 0x07, 0x07 }, // 263
        { 0xFF, 0x1, 0x07, 0x07 }, // 263
        { 0xFF, 0x1, 0x07, 0x07 }, // 263
        { 0xFF, 0x1, 0x0C, 0x0C }, // 268
        { 0xFF, 0x1, 0x0B, 0x0B }, // 267
        { 0xFF, 0x1, 0x10, 0x10 }, // 272
        { 0xFF, 0x1, 0x15, 0x15 }, // 277
        { 0xFF, 0x1, 0x10, 0x10 }, // 272
        { 0xFF, 0x1, 0x12, 0x12 }, // 274
        { 0xFF, 0x1, 0x12, 0x12 }, // 274
        { 0xFF, 0x1, 0x12, 0x12 }, // 274
        { 0xFF, 0x1, 0x12, 0x12 }, // 274
        { 0xFF, 0x1, 0x12, 0x12 }, // 274
        { 0xFF, 0x1, 0x12, 0x12 }, // 274
        { 0xFF, 0x1, 0x12, 0x12 }, // 274
        { 0xFF, 0x1, 0x19, 0x19 }, // 281
        { 0xFF, 0x2, 0x10, 0x11 }, // 528
        { 0xFF, 0x2, 0xC0, 0xC1 }, // 704
        { 0xFF, 0x2, 0xB7, 0xB8 }, // 695
        { 0xFF, 0x2, 0x51, 0x52 }, // 593
        { 0xFF, 0x2, 0x4C, 0x4D }, // 588
        { 0xFF, 0x2, 0xC4, 0xC5 }, // 708
        { 0xFF, 0x2, 0x33, 0x34 }, // 563
        { 0xFF, 0x2, 0xB7, 0xB8 }, // 695
        { 0xFF, 0x2, 0x51, 0x52 }, // 593
        { 0xFF, 0x2, 0xB7, 0xB8 }, // 695
        { 0xFF, 0x2, 0x51, 0x52 }, // 593
        { 0xFF, 0x2, 0x50, 0x51 }, // 592
        { 0xFF, 0x2, 0x43, 0x44 }, // 579
        { 0xFF, 0x2, 0x4C, 0x4D }, // 588
        { 0xFF, 0x2, 0xB7, 0xB8 }  // 695
    };

    auto raw_distance = [](std::array<uint8_t, 2> pair) {
        double out = (pair[0] << 8) | pair[1];
        out /= 1000.0;

        return out;
    };

    for (auto& sample : samples) {
        port.write(sample.data(), sample.size());
        ptr->tick();

        TEST_ASSERT_EQUAL(SENSOR_ERROR_OK, ptr->error());
        TEST_ASSERT_EQUAL_DOUBLE(
            raw_distance({sample[1], sample[2]}), ptr->value(0));
    }
}

void test_dht_data() {
    TEST_ASSERT_EQUAL_FLOAT(43.f,
        dht_humidity(DHT_CHIP_DHT11, {0x2b, 0x0}));
    TEST_ASSERT_EQUAL_FLOAT(43.f,
        dht_humidity(DHT_CHIP_DHT11, {0x2b, 0xe}));

    TEST_ASSERT_EQUAL_FLOAT(23.f,
        dht_temperature(DHT_CHIP_DHT11, {0x17, 0x0}));
    TEST_ASSERT_EQUAL_FLOAT(23.f,
        dht_temperature(DHT_CHIP_DHT11, {0x17, 0xf}));

    TEST_ASSERT_EQUAL_FLOAT(56.8f,
        dht_humidity(DHT_CHIP_DHT12, {0x38, 0x8}));
    TEST_ASSERT_EQUAL_FLOAT(26.6f,
        dht_temperature(DHT_CHIP_DHT12, {0x1a, 0x6}));
    TEST_ASSERT_EQUAL_FLOAT(-26.6f,
        dht_temperature(DHT_CHIP_DHT12, {0x1a, 0x86}));

    TEST_ASSERT(dht_checksum({0x1, 0xc1, 0x0, 0x2, 0xc4}));
    TEST_ASSERT(dht_checksum({0x1, 0xc8, 0x80, 0x2, 0x4b}));

    TEST_ASSERT_EQUAL_FLOAT(44.9f,
        dht_humidity(DHT_CHIP_DHT22, {0x1, 0xc1}));
    TEST_ASSERT_EQUAL_FLOAT(45.6f,
        dht_humidity(DHT_CHIP_DHT22, {0x1, 0xc8}));
    TEST_ASSERT_EQUAL_FLOAT(0.2f,
        dht_temperature(DHT_CHIP_DHT22, {0x0, 0x2}));
    TEST_ASSERT_EQUAL_FLOAT(-0.2f,
        dht_temperature(DHT_CHIP_DHT22, {0x80, 0x2}));

    TEST_ASSERT_EQUAL_FLOAT(92.3f,
        dht_humidity(DHT_CHIP_DHT22, {0x3, 0x9b}));
    TEST_ASSERT_EQUAL_FLOAT(2.9f,
        dht_temperature(DHT_CHIP_DHT22, {0x0, 0x1d}));

    TEST_ASSERT(dht_checksum({0x2, 0x33, 0xff, 0xf7, 0x2b}));
    TEST_ASSERT(dht_checksum({0x2, 0x11, 0xff, 0xf1, 0x03}));
    TEST_ASSERT(dht_checksum({0x2, 0x10, 0xff, 0xf1, 0x02}));

    TEST_ASSERT_EQUAL_FLOAT(56.3f,
        dht_humidity(DHT_CHIP_DHT22, {0x2, 0x33}));
    TEST_ASSERT_EQUAL_FLOAT(-0.9f,
        dht_temperature(DHT_CHIP_DHT22, {0xff, 0xf7}));

    TEST_ASSERT_EQUAL_FLOAT(93.0f,
        dht_humidity(DHT_CHIP_DHT22, {0x3, 0xa2}));
    TEST_ASSERT_EQUAL_FLOAT(-4.8f,
        dht_temperature(DHT_CHIP_DHT22, {0xff, 0xd0}));
    TEST_ASSERT_EQUAL_FLOAT(-4.7f,
        dht_temperature(DHT_CHIP_DHT22, {0xff, 0xd1}));
    TEST_ASSERT_EQUAL_FLOAT(-4.6f,
        dht_temperature(DHT_CHIP_DHT22, {0xff, 0xd2}));

    TEST_ASSERT_EQUAL_FLOAT(88.9f,
        dht_humidity(DHT_CHIP_DHT22, {0x3, 0x79}));
    TEST_ASSERT_EQUAL_FLOAT(-2.2f,
        dht_temperature(DHT_CHIP_DHT22, {0xf, 0xea}));
}

} // namespace
} // namespace test
} // namespace espurna

int main(int, char**) {
    UNITY_BEGIN();
    using namespace espurna::test;
    RUN_TEST(test_cse7766_data);
    RUN_TEST(test_a02yyu_data);
    RUN_TEST(test_dht_data);
    return UNITY_END();
}
