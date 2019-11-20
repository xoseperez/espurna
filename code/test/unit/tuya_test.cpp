#include <unity.h>
#include <type_traits>

// -----------------------------------------------------------------------------
// Various Arduino objects emulated through the host functions
// -----------------------------------------------------------------------------
// TODO can we build this using some existing library?

#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cstdint>
#include <cassert>

struct Serial {
    void println(const char* some) { return; }
};

Serial Serial1;

uint32_t millis() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

struct Print {
    size_t write(uint8_t c) { std::cout << c; return 1; }
    size_t write(const uint8_t *data, size_t size) { if (!size) return size; for (int n=0; n<size; ++n) { write(data[n]); } return size; }
    size_t write(const char *data, size_t size=0) { if (!size) size = strlen(data); return write((const uint8_t*)data, size); }
    void flush() { std::cout << std::endl; }
};

struct Stream : public Print {
    bool available() { return true; }
    int read() { return 1; }
};

struct StreamString : public Stream {
    void reserve(size_t size) { (void) size; }
};

/*
class PrintRaw {
    public:
        static void write(Print& printer, uint8_t data) {
            printer.write(data);
        }

        static void write(Print& printer, uint8_t* data, size_t size) {
            printer.write(data, size);
        }
};

class PrintHex {
    public:
        static void write(Print& printer, uint8_t data) {
            char buffer[3] = {0};
            snprintf(buffer, sizeof(buffer), "%02x", data);
            printer.write((const uint8_t*)buffer, 2);
        }

        static void write(Print& printer, uint8_t* data, size_t size) {
            for (size_t n=0; n<size; ++n) {
                char buffer[3] = {0};
                snprintf(buffer, sizeof(buffer), "%02x", data[n]);
                printer.write((const uint8_t*)buffer, 2);
            }
        }
};
*/

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

#include "libs/TypeChecks.h"
#include "tuya_types.h"
#include "tuya_util.h"
#include "tuya_transport.h"
#include "tuya_protocol.h"
#include "tuya_dataframe.h"

using namespace Tuya;

bool test_datatype(const DataFrame& frame, const Type expect_type) {
    const auto type = dataType(frame);
    return expect_type == type;
}

void show_datatype(const DataFrame& frame) {
    const auto type = dataType(frame);
    switch (type) {
        case Type::INT:
            std::cout << "INT";
            break;
        case Type::BOOL:
            std::cout << "BOOL";
            break;
        case Type::UNKNOWN:
        default:
            std::cout << "UNKNOWN";
            break;
    }
    std::cout << std::endl;
}

void test_dataprotocol(bool value) {
    auto proto = DataProtocol<bool>(0x02, value);
    assert(proto.id() == 0x02);
    assert(proto.value() == value);

    auto data = proto.serialize();
    std::cout << "+++ DataProtocol bool(" << (value ? "true" : "false") << ")" << std::endl;
    std::cout << "size=" << int(data.size()) << " values=";
    for (auto it = data.begin(); it != data.end(); ++it)
        std::cout << int(*it) << " ";
    std::cout << std::endl;
}

void show_dataframe(const char* tag, const DataFrame& frame) {
    auto data = const_cast<DataFrame&>(frame).serialize();
    show_datatype(frame);
    std::cout << " +++" << tag << " length=" << frame.length << " serialized_len=" << int(data.size());

    std::cout << " values=";
    for (auto it = data.begin(); it != data.end(); ++it)
        std::cout << int(*it) << " ";
    std::cout << std::endl;
}

void test_hexdump(const char* tag, const std::vector<uint8_t>& frame) {
    std::ios oldState(nullptr);
    oldState.copyfmt(std::cout);

    std::cout << "+++" << tag << std::endl;
    for (auto it = frame.cbegin(); it != frame.cend(); ++it)
        std::cout << std::setfill('0') << std::setw(2) << std::hex << int(*it) << " ";
    std::cout << std::endl << "---" << tag << std::endl;

    std::cout.copyfmt(oldState);
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

void test_dataprotocol() {
    test_dataprotocol(true);
    test_dataprotocol(false);
}


void test_static_dataframe_bool() {

    DataFrame frame(Command::SetDP, DataProtocol<bool>(0x02, false).serialize());

    TEST_ASSERT_EQUAL_MESSAGE(0, frame.version,
            "Version should stay 0 unless explicitly set");
    TEST_ASSERT_MESSAGE(frame.commandEquals(Command::SetDP),
            "commandEquals should return true with the same arg as in the constructor");
    TEST_ASSERT_MESSAGE(test_datatype(frame, Type::BOOL),
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

void test_dataframe_const() {

    const DataFrame frame(Command::SetDP);
    TEST_ASSERT_EQUAL_MESSAGE(0, frame.length,
            "Frame with Command::SetDP should not have any data attached to it");
    TEST_ASSERT_EQUAL_MESSAGE(0, std::distance(frame.cbegin(), frame.cend()),
            "Frame with Command::SetDP should not have any data attached to it");

}

void test_static_dataframe_heartbeat() {

    DataFrame frame(Command::Heartbeat);
    TEST_ASSERT_EQUAL_MESSAGE(0, frame.length,
            "Frame with Command::Heartbeat should not have any data attached to it");
    TEST_ASSERT_EQUAL_MESSAGE(0, std::distance(frame.cbegin(), frame.cend()),
            "Frame with Command::SetDP should not have any data attached to it");
    //test_hexdump("static", static_frame.serialize());

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

    const std::vector<uint8_t> data = {0x55, 0xaa, 0x00, 0x07, 0x00, 0x05, 0x01, 0x01, 0x00, 0x01, 0x01, 0x0f};
    DataFrame frame(0u, static_cast<uint8_t>(Command::ReportDP), 5u, data.begin() + 6, data.begin() + 12);
    //show_datatype(frame);
    //std::cout << "length=" << frame.length << std::endl;
    //test_hexdump("input", frame.serialize());

    //std::cout << "[" << millis() << "] -------------------bad dp frame----------------" << std::endl;
    //DataFrame bad_dp_frame(Command::ReportDP, DataProtocol<uint32_t>(0x03, 255).serialize());
    //show_datatype(bad_dp_frame);
    //std::cout << "length=" << bad_dp_frame.length << std::endl;
    //test_hexdump("input", bad_dp_frame.serialize());

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
    UNITY_END();

    /*
    test_states();
    test_dataprotocol();
    test_staticdataframe();
    */

}
