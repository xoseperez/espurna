#include <Arduino.h>
#include <Stream.h>
#include <unity.h>

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

#include <type_traits>
#include <queue>

#include "libs/TypeChecks.h"
#include "tuya_types.h"
#include "tuya_util.h"
#include "tuya_transport.h"
#include "tuya_protocol.h"
#include "tuya_dataframe.h"

using namespace Tuya;

static bool datatype_same(const DataFrame& frame, const Type expect_type) {
    const auto type = dataType(frame);
    return expect_type == type;
}

void test_states() {

    States<bool> states(8);

    // Will not update anything without explicit push
    states.update(1, false);
    states.update(1, true);
    states.update(2, true);
    states.update(2, false);

    TEST_ASSERT_EQUAL_MESSAGE(8, states.capacity(),
            "Capacity has changed");
    TEST_ASSERT_EQUAL_MESSAGE(0, states.size(),
            "Size should not change when updating non-existant id");

    // Push something at specific ID
    states.pushOrUpdate(2, true);
    TEST_ASSERT_MESSAGE(states.changed(),
            "Should change after explicit push");
    states.pushOrUpdate(2, false);
    TEST_ASSERT_MESSAGE(states.changed(),
            "Should change after explicit update");
    TEST_ASSERT_EQUAL_MESSAGE(1, states.size(),
            "Size should not change when updating existing id");
    states.pushOrUpdate(3, true);
    TEST_ASSERT_MESSAGE(states.changed(),
            "Should change after explicit push");

    // Do not trigger "changed" state when value remains the same
    states.pushOrUpdate(2, false);
    TEST_ASSERT_MESSAGE(!states.changed(),
            "Should not change after not changing any values");

    // Still shouldn't trigger "changed" without explicit push
    states.update(4, false);
    TEST_ASSERT_MESSAGE(!states.changed(),
            "Should not change after updating non-existant id");
    TEST_ASSERT_EQUAL_MESSAGE(2, states.size(),
            "Size should remain the same after updating non-existant id");

}

void test_static_dataframe_bool() {

    DataFrame frame(Command::SetDP, DataProtocol<bool>(0x02, false).serialize());

    TEST_ASSERT_EQUAL_MESSAGE(0, frame.version,
            "Version should stay 0 unless explicitly set");
    TEST_ASSERT_MESSAGE(frame.commandEquals(Command::SetDP),
            "commandEquals should return true with the same arg as in the constructor");
    TEST_ASSERT_MESSAGE(datatype_same(frame, Type::BOOL),
            "DataProtocol<bool> should translate to Type::BOOL");

}

void test_static_dataframe_int() {

    DataFrame frame(Command::ReportDP, DataProtocol<uint32_t>(0x03, 255).serialize());
    TEST_ASSERT_EQUAL_MESSAGE(0, frame.version,
            "Version should stay 0 unless explicitly set");
    TEST_ASSERT_MESSAGE(frame.commandEquals(Command::ReportDP),
            "commandEquals should return true with the same arg as in the constructor");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(std::distance(frame.cbegin(), frame.cend()), frame.length,
            "Data is expected to be stored in a contigious memory and be equal in length to the ::length attribute");
    TEST_ASSERT_EQUAL_MESSAGE(0, frame[5],
            "Only last byte should be set");
    TEST_ASSERT_EQUAL_MESSAGE(255, frame[7],
            "Only last byte should be set");

}

void test_static_dataframe_heartbeat() {

    DataFrame frame(Command::Heartbeat);
    TEST_ASSERT_EQUAL_MESSAGE(0, frame.length,
            "Frame with Command::Heartbeat should not have any data attached to it");
    TEST_ASSERT_EQUAL_MESSAGE(0, std::distance(frame.cbegin(), frame.cend()),
            "Frame with Command::SetDP should not have any data attached to it");
    //test_hexdump("static", static_frame.serialize());

}

void test_dataframe_const() {

    const DataFrame frame(Command::SetDP);
    TEST_ASSERT_EQUAL_MESSAGE(0, frame.length,
            "Frame with Command::SetDP should not have any data attached to it");
    TEST_ASSERT_EQUAL_MESSAGE(0, std::distance(frame.cbegin(), frame.cend()),
            "Frame with Command::SetDP should not have any data attached to it");

}


void test_dataframe_copy() {

    DataFrame frame(Command::Heartbeat);
    frame.version = 0x7f;

    DataFrame moved_frame(std::move(frame));
    TEST_ASSERT_EQUAL_MESSAGE(0x7f, moved_frame.version,
            "DataFrame should be movable object");

    TEST_ASSERT_MESSAGE(!std::is_copy_constructible<DataFrame>::value,
            "DataFrame should not be copyable");

}

void test_dataframe_raw_data() {

    {
        const std::vector<uint8_t> data = {0x55, 0xaa, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01};
        DataFrame frame(data.cbegin());
        TEST_ASSERT_MESSAGE(frame.commandEquals(Command::Heartbeat),
                "This message should be parsed as heartbeat");
        TEST_ASSERT_EQUAL_MESSAGE(0, frame.version,
                "This message should have version == 0");
        TEST_ASSERT_EQUAL_MESSAGE(1, frame.length,
                "Heartbeat message contains a single byte");
        TEST_ASSERT_EQUAL_MESSAGE(1, frame[0],
                "Heartbeat message contains a single 0x01");
    }

    {
        const std::vector<uint8_t> data = {0x55, 0xaa, 0x00, 0x07, 0x00, 0x05, 0x01, 0x01, 0x00, 0x01, 0x01, 0x0f};
        DataFrame frame(data.cbegin());
        TEST_ASSERT_MESSAGE(frame.commandEquals(Command::ReportDP),
                "This message should be parsed as data protocol");
        TEST_ASSERT_MESSAGE(datatype_same(frame, Type::BOOL),
                "This message should have boolean datatype attached to it");
        TEST_ASSERT_EQUAL_MESSAGE(5, frame.length,
                "Boolean DP contains 5 bytes");

        const DataProtocol<bool> dp(frame);
        TEST_ASSERT_EQUAL_MESSAGE(1, dp.id(), "This boolean DP id should be 1");
        TEST_ASSERT_MESSAGE(dp.value(), "This boolean DP value should be true");
    }

    //show_datatype(frame);
    //std::cout << "length=" << frame.length << std::endl;
    //test_hexdump("input", frame.serialize());

    //std::cout << "[" << millis() << "] -------------------bad dp frame----------------" << std::endl;
    //DataFrame bad_dp_frame(Command::ReportDP, DataProtocol<uint32_t>(0x03, 255).serialize());
    //show_datatype(bad_dp_frame);
    //std::cout << "length=" << bad_dp_frame.length << std::endl;
    //test_hexdump("input", bad_dp_frame.serialize());

}

class BufferedStream : public Stream {
    public:
        // Print interface
        size_t write(uint8_t c) {
            _buffer.push((int)c);
            return 1;
        }
        size_t write(const unsigned char* data, unsigned long size) {
            for (size_t n = 0; n < size; ++n) {
                _buffer.push(data[n]);
            }
            return size;
        }
        int availableForWrite() { return 1; }
        void flush() {
            while (!_buffer.empty()) {
                _buffer.pop();
            }
        }
        // Stream interface
        int available() {
            return _buffer.size();
        }
        int read() {
            if (!_buffer.size()) return -1;
            int c = _buffer.front();
            _buffer.pop();
            return c;
        }
        int peek() {
            if (!_buffer.size()) return -1;
            return _buffer.front();
        }
    private:
        std::queue<int> _buffer;
};

void test_transport() {
    const std::vector<uint8_t> data = {0x55, 0xaa, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01};
    BufferedStream stream;
    stream.write(data.data(), data.size());

    Transport transport(stream);
    TEST_ASSERT_MESSAGE(transport.available(), "Available data");
}

int main(int argc, char** argv) {

    UNITY_BEGIN();

    RUN_TEST(test_states);
    RUN_TEST(test_static_dataframe_bool);
    RUN_TEST(test_static_dataframe_int);
    RUN_TEST(test_static_dataframe_heartbeat);
    RUN_TEST(test_dataframe_const);
    RUN_TEST(test_dataframe_copy);
    RUN_TEST(test_dataframe_raw_data);
    RUN_TEST(test_transport);

    UNITY_END();

}
