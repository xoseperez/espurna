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

using namespace tuya;

template <typename T>
static bool datatype_same(const T& frame, const Type expect_type) {
    const auto type = dataType(frame);
    return expect_type == type;
}

void test_dpmap() {

    DpMap map;

    // id <-> dp
    map.add(1, 2);
    map.add(3, 4);
    map.add(5, 6);
    map.add(7, 8);

    TEST_ASSERT_EQUAL(4, map.size());

    map.add(7,10);
    map.add(5,5);

    // dpmap is a 'set' of values
    TEST_ASSERT_EQUAL(4, map.size());

#define TEST_FIND_DP_ID(EXPECTED_DP_ID, EXPECTED_LOCAL_ID) \
    {\
        auto* entry = map.find_dp(EXPECTED_DP_ID);\
        TEST_ASSERT(entry != nullptr);\
        TEST_ASSERT_EQUAL(EXPECTED_DP_ID, entry->dp_id);\
        TEST_ASSERT_EQUAL(EXPECTED_LOCAL_ID, entry->local_id);\
    }

    TEST_FIND_DP_ID(2, 1);
    TEST_FIND_DP_ID(4, 3);
    TEST_FIND_DP_ID(6, 5);
    TEST_FIND_DP_ID(8, 7);

#define TEST_FIND_LOCAL_ID(EXPECTED_LOCAL_ID, EXPECTED_DP_ID) \
    {\
        auto* entry = map.find_local(EXPECTED_LOCAL_ID);\
        TEST_ASSERT(entry != nullptr);\
        TEST_ASSERT_EQUAL(EXPECTED_DP_ID, entry->dp_id);\
        TEST_ASSERT_EQUAL(EXPECTED_LOCAL_ID, entry->local_id);\
    }

    TEST_FIND_LOCAL_ID(1, 2);
    TEST_FIND_LOCAL_ID(3, 4);
    TEST_FIND_LOCAL_ID(5, 6);
    TEST_FIND_LOCAL_ID(7, 8);

#undef TEST_FIND_LOCAL_ID
#undef TEST_FIND_DP_ID
}

void test_static_dataframe_bool() {

    DataFrame frame(Command::SetDP, DataProtocol<bool>(0x02, false).serialize());

    TEST_ASSERT_EQUAL_MESSAGE(0, frame.version(),
            "Version should stay 0 unless explicitly set");
    TEST_ASSERT_MESSAGE((frame.command() == Command::SetDP),
            "commandEquals should return true with the same arg as in the constructor");
    TEST_ASSERT_MESSAGE(datatype_same(frame, Type::BOOL),
            "DataProtocol<bool> should translate to Type::BOOL");

}

void test_static_dataframe_int() {

    DataFrame frame(Command::ReportDP, DataProtocol<uint32_t>(0x03, 255).serialize());
    TEST_ASSERT_EQUAL_MESSAGE(0, frame.version(),
            "Version should stay 0 unless explicitly set");
    TEST_ASSERT_MESSAGE((frame.command() == Command::ReportDP),
            "commandEquals should return true with the same arg as in the constructor");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(std::distance(frame.cbegin(), frame.cend()), frame.length(),
            "Data is expected to be stored in a contigious memory and be equal in length to the ::length attribute");
    TEST_ASSERT_EQUAL_MESSAGE(0, frame[5],
            "Only last byte should be set");
    TEST_ASSERT_EQUAL_MESSAGE(255, frame[7],
            "Only last byte should be set");

}

void test_static_dataframe_heartbeat() {

    DataFrame frame(Command::Heartbeat);
    TEST_ASSERT_EQUAL_MESSAGE(0, frame.length(),
            "Frame with Command::Heartbeat should not have any data attached to it");
    TEST_ASSERT_EQUAL_MESSAGE(0, std::distance(frame.cbegin(), frame.cend()),
            "Frame with Command::SetDP should not have any data attached to it");
    //test_hexdump("static", static_frame.serialize());

}

void test_dataframe_const() {

    const DataFrame frame(Command::SetDP);
    TEST_ASSERT_EQUAL_MESSAGE(0, frame.length(),
            "Frame with Command::SetDP should not have any data attached to it");
    TEST_ASSERT_EQUAL_MESSAGE(0, std::distance(frame.cbegin(), frame.cend()),
            "Frame with Command::SetDP should not have any data attached to it");

}


void test_dataframe_copy() {

    DataFrame frame(Command::Heartbeat, 0x7f, container{1,2,3});

    DataFrame moved_frame(std::move(frame));
    TEST_ASSERT_EQUAL(3, moved_frame.length());
    TEST_ASSERT_EQUAL(3, moved_frame.length());
    TEST_ASSERT_EQUAL_MESSAGE(0x7f, moved_frame.version(),
            "DataFrame should be movable object");

    DataFrame copied_frame(moved_frame);
    TEST_ASSERT_EQUAL(3, copied_frame.length());
    TEST_ASSERT_EQUAL_MESSAGE(0x7f, copied_frame.version(),
            "DataFrame should not be copyable");

}

void test_dataframe_raw_data() {

    {
        container data = {0x00, 0x00, 0x00, 0x01, 0x01};
        DataFrameView frame(data);

        TEST_ASSERT_MESSAGE((frame.command() == Command::Heartbeat),
                "This message should be parsed as heartbeat");
        TEST_ASSERT_EQUAL_MESSAGE(0, frame.version(),
                "This message should have version == 0");
        TEST_ASSERT_EQUAL_MESSAGE(1, frame.length(),
                "Heartbeat message contains a single byte");
        TEST_ASSERT_EQUAL_MESSAGE(1, frame[0],
                "Heartbeat message contains a single 0x01");

        auto serialized = frame.serialize();
        TEST_ASSERT_MESSAGE(std::equal(data.begin(), data.end(), serialized.begin()),
            "Serialized frame should match the original data");
    }

    {
        container data = {0x00, 0x07, 0x00, 0x05, 0x01, 0x01, 0x00, 0x01, 0x01};
        DataFrameView frame(data);

        TEST_ASSERT_MESSAGE((frame.command() == Command::ReportDP),
                "This message should be parsed as data protocol");
        TEST_ASSERT_MESSAGE(datatype_same(frame, Type::BOOL),
                "This message should have boolean datatype attached to it");
        TEST_ASSERT_EQUAL_MESSAGE(5, frame.length(),
                "Boolean DP contains 5 bytes");

        const DataProtocol<bool> dp(frame.data());
        TEST_ASSERT_EQUAL_MESSAGE(1, dp.id(), "This boolean DP id should be 1");
        TEST_ASSERT_MESSAGE(dp.value(), "This boolean DP value should be true");

        auto serialized = frame.serialize();
        TEST_ASSERT_MESSAGE(std::equal(data.begin(), data.end(), serialized.begin()),
            "Serialized frame should match the original data");
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
    container data = {0x55, 0xaa, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01};
    BufferedStream stream;
    stream.write(data.data(), data.size());

    Transport transport(stream);
    TEST_ASSERT(transport.available());

    for (size_t n = 0; n < data.size(); ++n) {
        transport.read();
    }
    TEST_ASSERT(transport.done());
}

void test_dataframe_report() {
    container input = {0x55, 0xaa, 0x00, 0x07, 0x00, 0x08, 0x02, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x10, 0x26};

    BufferedStream stream;
    stream.write(input.data(), input.size());

    Transport transport(stream);
    while (transport.available()) {
        transport.read();
    }

    TEST_ASSERT(transport.done());

    DataFrameView frame(transport);
    TEST_ASSERT(frame.command() == Command::ReportDP);
    TEST_ASSERT_EQUAL(Type::INT, dataType(frame));
    TEST_ASSERT_EQUAL(8, frame.length());
    TEST_ASSERT_EQUAL(0, frame.version());

    DataProtocol<uint32_t> proto(frame.data());
    TEST_ASSERT_EQUAL(0x02, proto.id());
    TEST_ASSERT_EQUAL(0x10, proto.value());
}

void test_dataframe_echo() {
    BufferedStream stream;
    Transport transport(stream);

    {
        DataProtocol<uint32_t> proto(0x02, 0x66);
        TEST_ASSERT_EQUAL(0x02, proto.id());
        TEST_ASSERT_EQUAL(0x66,proto.value());

        DataFrame frame(Command::SetDP, proto.serialize());
        transport.write(frame.serialize());
    }

    while (transport.available()) {
        transport.read();
    }
    TEST_ASSERT(transport.done());

    {
        DataFrameView frame(transport);
        TEST_ASSERT(frame.command() == Command::SetDP);
        TEST_ASSERT_EQUAL(Type::INT, dataType(frame));
        TEST_ASSERT_EQUAL(8, frame.length());
        TEST_ASSERT_EQUAL(0, frame.version());

        DataProtocol<uint32_t> proto(frame.data());
        TEST_ASSERT_EQUAL(0x02, proto.id());
        TEST_ASSERT_EQUAL(0x66, proto.value());
    }
}

int main(int argc, char** argv) {

    UNITY_BEGIN();

    RUN_TEST(test_dpmap);
    RUN_TEST(test_static_dataframe_bool);
    RUN_TEST(test_static_dataframe_int);
    RUN_TEST(test_static_dataframe_heartbeat);
    RUN_TEST(test_dataframe_const);
    RUN_TEST(test_dataframe_copy);
    RUN_TEST(test_dataframe_raw_data);
    RUN_TEST(test_dataframe_report);
    RUN_TEST(test_dataframe_echo);
    RUN_TEST(test_transport);

    return UNITY_END();

}
