/*

UART_MQTT MODULE

Copyright (C) 2018 by Albert Weterings
Adapted by Xose Pérez <xose dot perez at gmail dot com>

Support queueing and handling input without termination by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "uartmqtt.h"

#if UART_MQTT_SUPPORT

#include "espurna.h"
#include "mqtt.h"
#include "utils.h"

#include <array>
#include <queue>

#if UART_MQTT_SOFTWARE_SERIAL
#include <SoftwareSerial.h>
#endif

namespace espurna {
namespace uart_mqtt {
namespace {

namespace build {

// TODO: these time values are arbitrary, specific values might need to depend on the baudrate?
// TODO: output is not a ring-buffer, overflow is handled by stalling until we have more space
constexpr auto ReadInterval = duration::Milliseconds { 100 };
constexpr auto WriteWindow = duration::Milliseconds { 100 };

constexpr size_t BufferSize { UART_MQTT_BUFFER_SIZE };
constexpr size_t SerializedSize { (BufferSize * 2) + 1 };

constexpr uint8_t TerminateIn { UART_MQTT_TERMINATE_IN };
constexpr uint8_t TerminateOut { UART_MQTT_TERMINATE_OUT };

constexpr bool Encode { UART_MQTT_ENCODE };
constexpr bool Decode { UART_MQTT_DECODE };

constexpr size_t Baudrate PROGMEM { UART_MQTT_BAUDRATE };
constexpr auto Config PROGMEM = UART_MQTT_CONFIG;

constexpr uint8_t RxPin { UART_MQTT_RX_PIN };
constexpr uint8_t TxPin { UART_MQTT_TX_PIN };

constexpr bool uart0_normal() {
    return (build::TxPin == 1) && (build::RxPin == 3);
}

constexpr bool uart0_swapped() {
    return (build::TxPin == 15) && (build::RxPin == 13);
}

HardwareSerial& uart0() {
    return Serial;
}

} // namespace build

// Output is capped, prepare using a fixed-size buffer
using Serialized = std::array<char, build::SerializedSize>;

// TODO: use hwserial buffer directly? needs additional code to
// use `peekBuffer()` and (possibly) allocate based on `BufferSize`
// (aka serial rx buffer size)
using Buffer = std::array<uint8_t, build::BufferSize>;

// Prefer smaller output instead of using `Serialized` directly
using Queue = std::queue<String>;

#if UART_MQTT_SOFTWARE_SERIAL
Stream& software_serial_port() {
    static auto& port = ([]() -> Stream& {
        auto* port = new SoftwareSerial(build::RxPin, build::TxPin);
        port->begin(build::Baudrate, SWSERIAL_8N1);
        return *port;
    })();

    return port;
}
#endif

#if __cplusplus >= 201703L
#define CONSTEXPR constexpr
#else
#define CONSTEXPR
#endif

Stream& hardware_port() {
    // TODO: instantiate port outside of here, without Arduino config stuff
    // TODO: this swap() thing is esp8266-specific
    static auto& port = ([]() -> Stream& {
        auto& port = build::uart0();

        port.begin(build::Baudrate, build::Config);
        if CONSTEXPR (build::uart0_swapped()) {
            port.flush();
            port.swap();
        }

        return port;
    })();

    return port;
}

Stream& port() {
#if UART_MQTT_SOFTWARE_SERIAL
    if CONSTEXPR (build::uart0_normal() || build::uart0_swapped()) {
        return hardware_port();
    }

    return software_serial_port();
#else
    return hardware_port();
#endif
}

#undef CONSTEXPR

namespace internal {

Buffer buffer;
auto cursor = buffer.begin();

Queue queue;

} // namespace internal

struct Span {
    Span() = delete;
    constexpr Span(const uint8_t* data, size_t size) :
        _data(data),
        _size(size)
    {}

    constexpr Span(const uint8_t* begin, const uint8_t* end) :
        _data(begin),
        _size(end - begin)
    {}

    constexpr const uint8_t* data() const {
        return _data;
    }

    constexpr size_t size() const {
        return _size;
    }

    constexpr const uint8_t* begin() const {
        return _data;
    }

    constexpr const uint8_t* end() const {
        return _data + _size;
    }

private:
    const uint8_t* _data;
    size_t _size;
};

String serialize(Span bytes, bool encode) {
    String out;

    Serialized string;
    if (encode) {
        const auto length = hexEncode(
            bytes.data(), bytes.size(),
            string.data(), string.size());

        if (length) {
            out.concat(string.data(), length);
        }
    } else {
        const auto length = std::min(string.size(), bytes.size());
        bytes = Span(bytes.begin(), length);
            
        std::copy(bytes.begin(), bytes.end(),
            string.begin());
        out.concat(string.begin(), length);
    }

    return out;
}

// Client shares the internal payload buffer for the whole 'connection', so it is possible
// to lose data here when network is either too slow or the network stack did not (yet)
// have time to send the previously buffered data.
void send(String data) {
    mqttSendRaw(
        mqttTopic(MQTT_TOPIC_UARTIN, false).c_str(),
        data.c_str(), false, 0);
}

void send(Span span, bool encode) {
    send(serialize(span, encode));
}

void read_no_termination(Stream& stream, bool encode) {
    const size_t capacity = std::distance(internal::cursor, internal::buffer.end());
    const size_t available = stream.available();
    if (available && capacity) {
        internal::cursor += stream.readBytes(
            internal::cursor,
            std::min(capacity, available));
    }

    using Clock = time::CoreClock;
    static auto last = Clock::now();

    const auto now = Clock::now();
    if ((internal::cursor == internal::buffer.end())
        || ((internal::cursor != internal::buffer.begin())
            && (now - last > build::ReadInterval)))
    {
        last = now;
        if (mqttConnected()) {
            send({internal::buffer.data(), internal::cursor}, encode);
        }

        internal::cursor = internal::buffer.begin();
    }
}

void read(Stream& stream, uint8_t termination, bool encode) {
    if (termination == 0) {
        read_no_termination(stream, encode);
        return;
    }

    const size_t available = stream.available();
    const size_t capacity = std::distance(internal::cursor, internal::buffer.end());
    if (available && capacity) {
        const auto length = std::min(capacity, available);
        internal::cursor += stream.readBytes(internal::cursor, length);
            
        if (!mqttConnected()) {
            internal::cursor = internal::buffer.begin();
            return;
        }

        const auto begin = internal::buffer.begin();
        const auto cursor = internal::cursor;

        do {
            auto it = std::find(begin, cursor, termination);
            if (it == cursor) {
                break;
            }

            // `std::find` points to `termination`
            const auto data = Span(begin, it - 1);
            if (data.size()) {
                send(data, encode);
            }
            
            ++it;
            internal::cursor = std::copy(it, cursor, begin);
        } while (internal::cursor != begin);
    }

    if (!capacity) {
        internal::cursor = internal::buffer.begin();
        return;
    }
}

// Only handle writes in the main loop(), both HW and SW serial streams may block
void enqueue(String data) {
    internal::queue.emplace(std::move(data));
}

void write(Stream& stream, uint8_t termination, bool decode) {
    using Clock = time::CoreClock;
    
    const auto start = Clock::now();
    while (!internal::queue.empty() && (Clock::now() - start < build::WriteWindow)) {
        const auto& front = internal::queue.front();
        if (decode) {
            Buffer decoded;
            const auto size = hexDecode(
                front.begin(), front.length(),
                decoded.data(), decoded.size());

            if (size) {
                stream.write(decoded.data(), size);
            }

            if (size && termination) {
                stream.write(termination);
            }
        } else {
            stream.write(front.begin(), front.length());
            if (termination) {
                stream.write(termination);
            }
        }

        internal::queue.pop();
    }
}


void mqtt_callback(unsigned int type, const char* topic, const char* payload) {
    static constexpr char Subscription[] = MQTT_TOPIC_UARTOUT;

    switch (type) {
    case MQTT_CONNECT_EVENT:
        mqttSubscribe(Subscription);
        break;
    case MQTT_MESSAGE_EVENT:
        const auto t = mqttMagnitude(topic);
        if (t.equals(Subscription)) {
            enqueue(payload);
        }
        break;
    }
}

void loop() {
    read(port(),
        build::TerminateIn,
        build::Encode);
    write(port(),
        build::TerminateOut,
        build::Decode);
}

void setup() {
    mqttRegister(mqtt_callback);
    espurnaRegisterLoop(loop);
}

} // namespace
} // namespace uart_mqtt
} // namespace espurna

// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

void uartMqttSetup() {
    espurna::uart_mqtt::setup();
}

#endif // UART_MQTT_SUPPORT
