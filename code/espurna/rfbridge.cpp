/*

RF BRIDGE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "rfbridge.h"

#if RFB_SUPPORT

#include "api.h"
#include "relay.h"
#include "terminal.h"
#include "mqtt.h"
#include "ws.h"
#include "utils.h"

BrokerBind(RfbridgeBroker);

#include <algorithm>
#include <bitset>
#include <cstring>
#include <list>
#include <memory>

// -----------------------------------------------------------------------------
// GLOBALS TO THE MODULE
// -----------------------------------------------------------------------------

unsigned char _rfb_repeats = RFB_SEND_REPEATS;

#if RFB_PROVIDER == RFB_PROVIDER_RCSWITCH

#include <RCSwitch.h>
RCSwitch * _rfb_modem;
bool _rfb_receive { false };
bool _rfb_transmit { false };

#else

constexpr bool _rfb_receive { true };
constexpr bool _rfb_transmit { true };

#endif

// -----------------------------------------------------------------------------
// MATCH RECEIVED CODE WITH THE SPECIFIC RELAY ID
// -----------------------------------------------------------------------------

#if RELAY_SUPPORT

struct RfbRelayMatch {
    RfbRelayMatch() = default;
    RfbRelayMatch(unsigned char id_, PayloadStatus status_) :
        id(id_),
        status(status_),
        _found(true)
    {}

    bool ok() {
        return _found;
    }

    void reset(unsigned char id_, PayloadStatus status_) {
        id = id_;
        status = status_;
        _found = true;
    }

    unsigned char id { 0u };
    PayloadStatus status { PayloadStatus::Unknown };

    private:

    bool _found { false };
};

struct RfbLearn {
    unsigned long ts;
    unsigned char id;
    bool status;
};

// Usage depends on the implementation. Will either:
// - efm8bb1: wait until learn OK / TIMEOUT code
// - rc-switch: receiver loop will check `ts` vs RFB_LEARN_TIMEOUT
static std::unique_ptr<RfbLearn> _rfb_learn;

// Individual lock for the relay, prevent rfbStatus from re-sending the code we just received
static std::bitset<RelaysMax> _rfb_relay_status_lock;

#endif // RELAY_SUPPORT

// -----------------------------------------------------------------------------
// EFM8BB1 PROTOCOL PARSING
// -----------------------------------------------------------------------------

constexpr uint8_t RfbDefaultProtocol { 0u };

constexpr uint8_t CodeStart { 0xAAu };
constexpr uint8_t CodeEnd { 0x55u };

constexpr uint8_t CodeAck { 0xA0u };

// both stock and https://github.com/Portisch/RF-Bridge-EFM8BB1/
// sending:
constexpr uint8_t CodeLearn { 0xA1u };

// receiving:
constexpr uint8_t CodeLearnTimeout { 0xA2u };
constexpr uint8_t CodeLearnOk { 0xA3u };
constexpr uint8_t CodeRecvBasic = { 0xA4u };
constexpr uint8_t CodeSendBasic = { 0xA5u };

// only https://github.com/Portisch/RF-Bridge-EFM8BB1/
constexpr uint8_t CodeRecvProto { 0xA6u };
constexpr uint8_t CodeRecvBucket { 0xB1u };

struct RfbParser {
    using callback_type = void(uint8_t, const std::vector<uint8_t>&);
    using state_type = void(RfbParser::*)(uint8_t);

    // AA XX ... 55
    // ^~~~~     ~~ - protocol head + tail
    //    ^~        - message code
    //       ^~~    - actual payload is always 9 bytes
    static constexpr size_t PayloadSizeBasic { 9ul };
    static constexpr size_t MessageSizeBasic { PayloadSizeBasic + 3ul };

    static constexpr size_t MessageSizeMax { 112ul };

    RfbParser() = delete;
    RfbParser(const RfbParser&) = delete;

    explicit RfbParser(callback_type* callback) :
        _callback(callback)
    {}

    RfbParser(RfbParser&&) = default;

    void stop(uint8_t c) {
    }

    void start(uint8_t c) {
        switch (c) {
        case CodeStart:
            _state = &RfbParser::read_code;
            break;
        default:
            _state = &RfbParser::stop;
            break;
        }
    }

    void read_code(uint8_t c) {
        _payload_code = c;
        switch (c) {
        // Generic ACK signal. We *expect* this after our requests
        case CodeAck:
        // *Expect* any code within a certain window.
        // Only matters to us, does not really do anything but help us to signal that the next code needs to be recorded
        case CodeLearnTimeout:
            _state = &RfbParser::read_end;
            break;
        // both stock and https://github.com/Portisch/RF-Bridge-EFM8BB1/
        // receive 9 bytes, where first 3 2-byte tuples are timings
        // and the last 3 bytes are the actual payload
        case CodeLearnOk:
        case CodeRecvBasic:
            _payload_length = PayloadSizeBasic;
            _state = &RfbParser::read_until_length;
            break;
        // specific to the https://github.com/Portisch/RF-Bridge-EFM8BB1/
        // receive N bytes, where the 1st byte is the protocol ID and the next N-1 bytes are the payload
        case CodeRecvProto:
            _state = &RfbParser::read_length;
            break;
        // unlike CodeRecvProto, we don't have any length byte here :/ for some reason, it is there only when sending
        // just bail out when we find CodeEnd
        // (TODO: is number of buckets somehow convertible to the 'expected' size?)
        case CodeRecvBucket:
            _state = &RfbParser::read_length;
            break;
        default:
            _state = &RfbParser::stop;
            break;
        }
    }

    void read_end(uint8_t c) {
        if (CodeEnd == c) {
            _callback(_payload_code, _payload);
        }
        _state = &RfbParser::stop;
    }

    void read_until_end(uint8_t c) {
        if (CodeEnd == c) {
            read_end(c);
            return;
        }
        _payload.push_back(c);
    }

    void read_until_length(uint8_t c) {
        _payload.push_back(c);

        if ((_payload_offset + _payload_length) == _payload.size()) {
            switch (_payload_code) {
            case CodeLearnOk:
            case CodeRecvBasic:
            case CodeRecvProto:
                _state = &RfbParser::read_end;
                break;
            case CodeRecvBucket:
                _state = &RfbParser::read_until_end;
                break;
            default:
                _state = &RfbParser::stop;
                break;
            }

            _payload_length = 0u;
        }
    }

    void read_length(uint8_t c) {
        switch (_payload_code) {
        case CodeRecvProto:
            _payload_length = c;
            break;
        case CodeRecvBucket:
            _payload_length = c * 2;
            break;
        default:
            _state = &RfbParser::stop;
            return;
        }

        _payload.push_back(c);
        _payload_offset = _payload.size();
        _state = &RfbParser::read_until_length;
    }

    bool loop(uint8_t c) {
        (this->*_state)(c);
        return (_state != &RfbParser::stop);
    }

    void reset() {
        _payload.clear();
        _payload_length = 0u;
        _payload_offset = 0u;
        _payload_code = 0u;
        _state = &RfbParser::start;
    }

    void reserve(size_t size) {
        _payload.reserve(size);
    }

    private:

    callback_type* _callback { nullptr };
    state_type _state { &RfbParser::start };

    std::vector<uint8_t> _payload;

    size_t _payload_length { 0ul };
    size_t _payload_offset { 0ul };

    uint8_t _payload_code { 0ul };
};

// -----------------------------------------------------------------------------
// MESSAGE SENDER
//
// Depends on the selected provider. While we do serialize RCSwitch results,
// we don't want to pass around such byte-array everywhere since we already
// know all of the required data members and can prepare a basic POD struct
// -----------------------------------------------------------------------------

#if RFB_PROVIDER == RFB_PROVIDER_EFM8BB1

struct RfbMessage {
    RfbMessage(const RfbMessage&) = default;
    RfbMessage(RfbMessage&&) = default;

    explicit RfbMessage(uint8_t (&data)[RfbParser::PayloadSizeBasic], unsigned char repeats_) :
        repeats(repeats_)
    {
        std::copy(data, data + sizeof(data), code);
    }

    uint8_t code[RfbParser::PayloadSizeBasic] { 0u };
    uint8_t repeats { 1u };
};

#elif RFB_PROVIDER == RFB_PROVIDER_RCSWITCH

struct RfbMessage {
    using code_type = decltype(std::declval<RCSwitch>().getReceivedValue());

    static constexpr size_t BufferSize = sizeof(code_type) + 5;

    uint8_t protocol;
    uint16_t timing;
    uint8_t bits;
    code_type code;
    uint8_t repeats;
};

#endif // RFB_PROVIDER == RFB_PROVIDER_EFM8BB1

static std::list<RfbMessage> _rfb_message_queue;

void _rfbLearnImpl();
void _rfbReceiveImpl();
void _rfbSendImpl(const RfbMessage& message);

// -----------------------------------------------------------------------------
// WEBUI INTEGRATION
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _rfbWebSocketSendCodeArray(JsonObject& root, unsigned char start, unsigned char size) {
    JsonObject& rfb = root.createNestedObject("rfb");
    rfb["size"] = size;
    rfb["start"] = start;

    JsonArray& on = rfb.createNestedArray("on");
    JsonArray& off = rfb.createNestedArray("off");

    for (uint8_t id=start; id<start+size; id++) {
        on.add(rfbRetrieve(id, true));
        off.add(rfbRetrieve(id, false));
    }
}

void _rfbWebSocketOnVisible(JsonObject& root) {
    root["rfbVisible"] = 1;
}

void _rfbWebSocketOnConnected(JsonObject& root) {
    root["rfbRepeat"] = getSetting("rfbRepeat", RFB_SEND_REPEATS);
    root["rfbCount"] = relayCount();
    #if RFB_PROVIDER == RFB_PROVIDER_RCSWITCH
        root["rfbdirectVisible"] = 1;
        root["rfbRX"] = getSetting("rfbRX", RFB_RX_PIN);
        root["rfbTX"] = getSetting("rfbTX", RFB_TX_PIN);
    #endif
}

void _rfbWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "rfblearn") == 0) rfbLearn(data["id"], data["status"]);
    if (strcmp(action, "rfbforget") == 0) rfbForget(data["id"], data["status"]);
    if (strcmp(action, "rfbsend") == 0) rfbStore(data["id"], data["status"], data["data"].as<const char*>());
}

bool _rfbWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "rfb", 3) == 0);
}

void _rfbWebSocketOnData(JsonObject& root) {
    _rfbWebSocketSendCodeArray(root, 0, relayCount());
}

#endif // WEB_SUPPORT

// -----------------------------------------------------------------------------
// RELAY <-> CODE MATCHING
// -----------------------------------------------------------------------------

#if RFB_PROVIDER == RFB_PROVIDER_EFM8BB1

// we only care about last 6 chars (3 bytes in hex),
// since in 'default' mode rfbridge only handles a single protocol
bool _rfbCompare(const char* lhs, const char* rhs, size_t length) {
    return (0 == std::memcmp((lhs + length - 6), (rhs + length - 6), 6));
}

#elif RFB_PROVIDER == RFB_PROVIDER_RCSWITCH

// protocol is [2:3), actual payload is [10:), as bit length may vary
// although, we don't care if it does, since we expect length of both args to be the same
bool _rfbCompare(const char* lhs, const char* rhs, size_t length) {
    return (0 == std::memcmp((lhs + 2), (rhs + 2), 2))
        && (0 == std::memcmp((lhs + 10), (rhs + 10), length - 10));
}

#endif // RFB_PROVIDER == RFB_PROVIDER_EFM8BB1

#if RELAY_SUPPORT

// try to find the 'code' saves as either rfbON# or rfbOFF#
//
// **always** expect full length code as input to simplify comparison
// previous implementation tried to help MQTT / API requests to match based on the saved code,
// thus requiring us to 'return' value from settings as the real code, replacing input
RfbRelayMatch _rfbMatch(const char* code) {

    if (!relayCount()) {
        return {};
    }

    const auto len = strlen(code);

    // we gather all available options, as the kv store might be defined in any order
    // scan kvs only once, since we want both ON and OFF options and don't want to depend on the relayCount()
    RfbRelayMatch matched;

    using namespace settings;
    kv_store.foreach([code, len, &matched](kvs_type::KeyValueResult&& kv) {
        const auto key = kv.key.read();
        PayloadStatus status = key.startsWith(F("rfbON"))
            ? PayloadStatus::On : key.startsWith(F("rfbOFF"))
            ? PayloadStatus::Off : PayloadStatus::Unknown;

        if (PayloadStatus::Unknown == status) {
            return;
        }

        const auto value = kv.value.read();
        if (len != value.length()) {
            return;
        }

        if (!_rfbCompare(code, value.c_str(), len)) {
            return;
        }

        // note: strlen is constexpr here
        const char* id_ptr = key.c_str() + (
            (PayloadStatus::On == status) ? strlen("rfbON") : strlen("rfbOFF"));
        if (*id_ptr == '\0') {
            return;
        }

        char *endptr = nullptr;
        const auto id = strtoul(id_ptr, &endptr, 10);
        if (endptr == id_ptr || endptr[0] != '\0' || id > std::numeric_limits<uint8_t>::max() || id >= relayCount()) {
            return;
        }

        // when we see the same id twice, we match the opposite statuses
        if (matched.ok() && (id == matched.id)) {
            matched.status = PayloadStatus::Toggle;
            return;
        }

        matched.reset(matched.ok()
            ? std::min(static_cast<uint8_t>(id), matched.id)
            : static_cast<uint8_t>(id),
            status
        );
    });


    return matched;

}

void _rfbLearnFromString(std::unique_ptr<RfbLearn>& learn, const char* buffer) {
    if (!learn) return;

    DEBUG_MSG_P(PSTR("[RF] Learned relay ID %u after %u ms\n"), learn->id, millis() - learn->ts);
    rfbStore(learn->id, learn->status, buffer);

    // Websocket update needs to happen right here, since the only time
    // we send these in bulk is at the very start of the connection
#if WEB_SUPPORT
    auto id = learn->id;
    wsPost([id](JsonObject& root) {
        _rfbWebSocketSendCodeArray(root, id, 1);
    });
#endif

    learn.reset(nullptr);
}

bool _rfbRelayHandler(const char* buffer, bool locked = false) {
    bool result { false };

    auto match = _rfbMatch(buffer);
    if (match.ok()) {
        DEBUG_MSG_P(PSTR("[RF] Matched with the relay ID %u\n"), match.id);
        _rfb_relay_status_lock.set(match.id, locked);

        switch (match.status) {
        case PayloadStatus::On:
        case PayloadStatus::Off:
            relayStatus(match.id, (PayloadStatus::On == match.status));
            result = true;
            break;
        case PayloadStatus::Toggle:
            relayToggle(match.id);
            result = true;
        case PayloadStatus::Unknown:
            break;
        }
    }

    return result;
}

#endif // RELAY_SUPPORT

// -----------------------------------------------------------------------------
// RF handler implementations
// -----------------------------------------------------------------------------

#if RFB_PROVIDER == RFB_PROVIDER_EFM8BB1

void _rfbEnqueue(uint8_t (&code)[RfbParser::PayloadSizeBasic], unsigned char repeats = 1u) {
    if (!_rfb_transmit) return;
    _rfb_message_queue.push_back(RfbMessage(code, repeats));
}

bool _rfbEnqueue(const char* code, unsigned char repeats = 1u) {
    uint8_t buffer[RfbParser::PayloadSizeBasic] { 0u };
    if (hexDecode(code, strlen(code), buffer, sizeof(buffer))) {
        _rfbEnqueue(buffer, repeats);
        return true;
    }

    DEBUG_MSG_P(PSTR("[RF] Cannot decode the message\n"));
    return false;
}

void _rfbSendRaw(const uint8_t* message, unsigned char size) {
    Serial.write(message, size);
}

void _rfbAckImpl() {
    static uint8_t message[3] {
        CodeStart, CodeAck, CodeEnd
    };

    DEBUG_MSG_P(PSTR("[RF] Sending ACK\n"));
    Serial.write(message, sizeof(message));
    Serial.flush();
}

void _rfbLearnImpl() {
    static uint8_t message[3] {
        CodeStart, CodeLearn, CodeEnd
    };

    DEBUG_MSG_P(PSTR("[RF] Sending LEARN\n"));
    Serial.write(message, sizeof(message));
    Serial.flush();
}

void _rfbSendImpl(const RfbMessage& message) {
    Serial.write(CodeStart);
    Serial.write(CodeSendBasic);
    _rfbSendRaw(message.code, sizeof(message.code));
    Serial.write(CodeEnd);
    Serial.flush();
}

void _rfbParse(uint8_t code, const std::vector<uint8_t>& payload) {
    switch (code) {

    case CodeAck:
        DEBUG_MSG_P(PSTR("[RF] Received ACK\n"));
        break;

    case CodeLearnTimeout:
        _rfbAckImpl();
#if RELAY_SUPPORT
        if (_rfb_learn) {
            DEBUG_MSG_P(PSTR("[RF] Learn timeout after %u ms\n"), millis() - _rfb_learn->ts);
            _rfb_learn.reset(nullptr);
        }
#endif
        break;

    case CodeLearnOk:
    case CodeRecvBasic: {
        _rfbAckImpl();

        char buffer[(RfbParser::PayloadSizeBasic * 2) + 1] = {0};
        if (hexEncode(payload.data(), payload.size(), buffer, sizeof(buffer))) {
            DEBUG_MSG_P(PSTR("[RF] Received code: %s\n"), buffer);

#if RELAY_SUPPORT
            if (CodeLearnOk == code) {
                _rfbLearnFromString(_rfb_learn, buffer);
            } else {
                _rfbRelayHandler(buffer, true);
            }
#endif

#if MQTT_SUPPORT
            mqttSend(MQTT_TOPIC_RFIN, buffer, false, false);
#endif

#if BROKER_SUPPORT
            RfbridgeBroker::Publish(RfbDefaultProtocol, buffer + 12);
#endif
        }
        break;
    }

    case CodeRecvProto:
    case CodeRecvBucket: {
        _rfbAckImpl();
        char buffer[(RfbParser::MessageSizeMax * 2) + 1] = {0};
        if (hexEncode(payload.data(), payload.size(), buffer, sizeof(buffer))) {
            DEBUG_MSG_P(PSTR("[RF] Received %s code: %s\n"),
                (CodeRecvProto == code) ? "advanced" : "bucket", buffer
            );

#if MQTT_SUPPORT
            mqttSend(MQTT_TOPIC_RFIN, buffer, false, false);
#endif

#if BROKER_SUPPORT
            // ref. https://github.com/Portisch/RF-Bridge-EFM8BB1/wiki/0xA6#example-of-a-received-decoded-protocol
            RfbridgeBroker::Publish(payload[0], buffer + 2);
#endif
        } else {
            DEBUG_MSG_P(PSTR("[RF] Received 0x%02X (%u bytes)\n"), code, payload.size());
        }
        break;
    }

    }
}

static RfbParser _rfb_parser(_rfbParse);

void _rfbReceiveImpl() {

    while (Serial.available()) {
        auto c = Serial.read();
        if (c < 0) {
            continue;
        }

        // narrowing is justified, as `c` can only contain byte-sized value
        if (!_rfb_parser.loop(static_cast<uint8_t>(c))) {
            _rfb_parser.reset();
        }
    }

}

// note that we don't care about queue here, just dump raw message as-is
void _rfbSendRawFromPayload(const char * raw) {
    auto rawlen = strlen(raw);
    if (rawlen > (RfbParser::MessageSizeMax * 2)) return;
    if ((rawlen < 6) || (rawlen & 1)) return;

    DEBUG_MSG_P(PSTR("[RF] Sending RAW MESSAGE \"%s\"\n"), raw);

    size_t bytes = 0;
    uint8_t message[RfbParser::MessageSizeMax] { 0u };
    if ((bytes = hexDecode(raw, rawlen, message, sizeof(message)))) {
        if (message[0] != CodeStart) return;
        if (message[bytes - 1] != CodeEnd) return;
        _rfbSendRaw(message, bytes);
    }
}

#elif RFB_PROVIDER == RFB_PROVIDER_RCSWITCH

namespace {

size_t _rfb_bytes_for_bits(size_t bits) {
    decltype(bits) bytes = 0;
    decltype(bits) need = 0;

    while (need < bits) {
        need += 8u;
        bytes += 1u;
    }

    return bytes;
}

// TODO: RCSwitch code type: long unsigned int != uint32_t, thus the specialization
static_assert(sizeof(uint32_t) == sizeof(long unsigned int), "");

template <typename T>
T _rfb_bswap(T value);

template <>
[[gnu::unused]] uint32_t _rfb_bswap(uint32_t value) {
    return __builtin_bswap32(value);
}

template <>
[[gnu::unused]] long unsigned int _rfb_bswap(long unsigned int value) {
    return __builtin_bswap32(value);
}

template <>
[[gnu::unused]] uint64_t _rfb_bswap(uint64_t value) {
    return __builtin_bswap64(value);
}

}

void _rfbEnqueue(uint8_t protocol, uint16_t timing, uint8_t bits, RfbMessage::code_type code, unsigned char repeats = 1u) {
    if (!_rfb_transmit) return;
    _rfb_message_queue.push_back(RfbMessage{protocol, timing, bits, code, repeats});
}

void _rfbEnqueue(const char* message, unsigned char repeats = 1u) {
    uint8_t buffer[RfbMessage::BufferSize] { 0u };
    if (hexDecode(message, strlen(message), buffer, sizeof(buffer))) {
        const auto bytes = _rfb_bytes_for_bits(buffer[4]);

        uint8_t raw_code[sizeof(RfbMessage::code_type)] { 0u };
        std::memcpy(&raw_code[sizeof(raw_code) - bytes], &buffer[5], bytes);

        RfbMessage::code_type code;
        std::memcpy(&code, raw_code, sizeof(code));

        _rfbEnqueue(buffer[1], (buffer[2] << 8) | buffer[3], buffer[4], _rfb_bswap(code), repeats);
        return;
    }

    DEBUG_MSG_P(PSTR("[RF] Cannot decode the message\n"));
}

void _rfbLearnImpl() {
    DEBUG_MSG_P(PSTR("[RF] Entering LEARN mode\n"));
}

void _rfbSendImpl(const RfbMessage& message) {

    if (!_rfb_transmit) return;

    // TODO: note that this seems to be setting global setting
    //       if code for some reason forgets this, we end up with the previous value
    _rfb_modem->setProtocol(message.protocol);
    if (message.timing) {
        _rfb_modem->setPulseLength(message.timing);
    }

    yield();

    _rfb_modem->send(message.code, message.bits);
    _rfb_modem->resetAvailable();

}

// Try to mimic the basic RF message format. although, we might have different size of the code itself
// Skip leading zeroes and only keep the useful data
//
// TODO: 'timing' value shooould be relatively small,
//       since it's original intent was to be used with 16bit ints
// TODO: both 'protocol' and 'bitlength' fit in a byte, despite being declared as 'unsigned int'

size_t _rfbModemPack(uint8_t (&out)[RfbMessage::BufferSize], RfbMessage::code_type code, unsigned int protocol, unsigned int timing, unsigned int bits) {
    static_assert((sizeof(decltype(code)) == 4) || (sizeof(decltype(code)) == 8), "");

    size_t index = 0;
    out[index++] = 0xC0;
    out[index++] = static_cast<uint8_t>(protocol);
    out[index++] = static_cast<uint8_t>(timing >> 8);
    out[index++] = static_cast<uint8_t>(timing);
    out[index++] = static_cast<uint8_t>(bits);

    auto bytes = _rfb_bytes_for_bits(bits);
    if (bytes > (sizeof(out) - index)) {
        return 0;
    }

    // manually overload each bswap, since we can't use ternary here
    // (and `if constexpr (...)` is only available starting from Arduino Core 3.x.x)
    decltype(code) swapped = _rfb_bswap(code);

    uint8_t raw[sizeof(swapped)];
    std::memcpy(raw, &swapped, sizeof(raw));

    while (bytes) {
        out[index++] = raw[sizeof(raw) - (bytes--)];
    }

    return index;
}

void _rfbLearnFromReceived(std::unique_ptr<RfbLearn>& learn, const char* buffer) {
    if (millis() - learn->ts > RFB_LEARN_TIMEOUT) {
        DEBUG_MSG_P(PSTR("[RF] Learn timeout after %u ms\n"), millis() - learn->ts);
        learn.reset(nullptr);
        return;
    }

    _rfbLearnFromString(learn, buffer);
}

void _rfbReceiveImpl() {

    if (!_rfb_receive) return;

    // TODO: rc-switch isr handler sets 4 variables at the same time and never checks their existence before overwriting them
    //       thus, we can't *really* trust that all 4 are from the same reading :/
    // TODO: in theory, we may also expirience memory tearing while doing 2 separate 32bit reads on the 64bit code value,
    //       while isr handler *may* write into it at the same time

    auto rf_code = _rfb_modem->getReceivedValue();
    if (!rf_code) {
        return;
    }

#if RFB_RECEIVE_DELAY
    static unsigned long last = 0;
    if (millis() - last < RFB_RECEIVE_DELAY) {
        _rfb_modem->resetAvailable();
        return;
    }

    last = millis();
#endif

    uint8_t message[RfbMessage::BufferSize];
    auto real_msgsize = _rfbModemPack(
        message,
        rf_code,
        _rfb_modem->getReceivedProtocol(),
        _rfb_modem->getReceivedDelay(),
        _rfb_modem->getReceivedBitlength()
    );

    char buffer[(sizeof(message) * 2) + 1] = {0};
    if (hexEncode(message, real_msgsize, buffer, sizeof(buffer))) {
        DEBUG_MSG_P(PSTR("[RF] Received code: %s\n"), buffer);

#if RELAY_SUPPORT
        if (_rfb_learn) {
            _rfbLearnFromReceived(_rfb_learn, buffer);
        } else {
            _rfbRelayHandler(buffer, true);
        }
#endif

#if MQTT_SUPPORT
        mqttSend(MQTT_TOPIC_RFIN, buffer, false, false);
#endif

#if BROKER_SUPPORT
        RfbridgeBroker::Publish(message[1], buffer + 10);
#endif
    }

    _rfb_modem->resetAvailable();

}

#endif // RFB_PROVIDER == ...

void _rfbSendQueued() {

    if (!_rfb_transmit) return;
    if (_rfb_message_queue.empty()) return;

    static unsigned long last = 0;
    if (millis() - last < RFB_SEND_DELAY) return;
    last = millis();

    auto message = _rfb_message_queue.front();
    _rfb_message_queue.pop_front();

    _rfbSendImpl(message);

    // Sometimes we really want to repeat the message, not only to rely on built-in transfer repeat
    if (message.repeats > 1) {
        message.repeats -= 1;
        _rfb_message_queue.push_back(std::move(message));
    }

    yield();

}

// Check if the payload looks like a HEX code (plus comma, specifying the 'repeats' arg for the queue)
void _rfbSendFromPayload(const char * payload) {

    decltype(_rfb_repeats) repeats { _rfb_repeats };
    size_t len { strlen(payload) };

    const char* sep { strchr(payload, ',') };
    if (sep && (*(sep + 1) != '\0')) {
        char *endptr = nullptr;
        repeats = strtoul(sep, &endptr, 10);
        if (endptr == payload || endptr[0] != '\0') {
            return;
        }
        len -= strlen(sep);
    }

    if (!len || (len & 1)) {
        return;
    }

    DEBUG_MSG_P(PSTR("[RF] Enqueuing MESSAGE '%s' %u time(s)\n"), payload, repeats);

    // We postpone the actual sending until the loop, as we may've been called from MQTT or HTTP API
    // RFB_PROVIDER implementation should select the appropriate de-serialization function
    _rfbEnqueue(payload, repeats);

}

void _rfbLearnStartFromPayload(const char* payload) {
    // The payload must be the `relayID,mode` (where mode is either 0 or 1)
    const char* sep = strchr(payload, ',');
    if (nullptr == sep) {
        return;
    }

    // ref. RelaysMax, we only have up to 2 digits
    char relay[3] {0, 0, 0};
    if ((sep - payload) > 2) {
        return;
    }

    std::copy(payload, sep, relay);

    char *endptr = nullptr;
    const auto id = strtoul(relay, &endptr, 10);
    if (endptr == &relay[0] || endptr[0] != '\0') {
        return;
    }

    if (id >= relayCount()) {
        DEBUG_MSG_P(PSTR("[RF] Invalid relay ID (%u)\n"), id);
        return;
    }

    ++sep;
    if ((*sep == '0') || (*sep == '1')) {
        rfbLearn(id, (*sep != '0'));
    }
}

#if MQTT_SUPPORT

void _rfbMqttCallback(unsigned int type, const char * topic, char * payload) {

    if (type == MQTT_CONNECT_EVENT) {

        char buffer[strlen(MQTT_TOPIC_RFLEARN) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_RFLEARN);
        mqttSubscribe(buffer);

        if (_rfb_transmit) {
            mqttSubscribe(MQTT_TOPIC_RFOUT);
        }

#if RFB_PROVIDER == RFB_PROVIDER_EFM8BB1
        mqttSubscribe(MQTT_TOPIC_RFRAW);
#endif

    }

    if (type == MQTT_MESSAGE_EVENT) {

        String t = mqttMagnitude((char *) topic);

        if (t.startsWith(MQTT_TOPIC_RFLEARN)) {
            _rfbLearnStartFromPayload(payload);
            return;
        }

        if (t.equals(MQTT_TOPIC_RFOUT)) {
#if RELAY_SUPPORT
            // we *sometimes* want to check the code against available rfbON / rfbOFF
            // e.g. in case we want to control some external device and have an external remote.
            // - when remote press happens, relays stay in sync when we receive the code via the processing loop
            // - when we send the code here, we never register it as *sent*, thus relays need to be made in sync manually
            if (!_rfbRelayHandler(payload)) {
#endif
                _rfbSendFromPayload(payload);
#if RELAY_SUPPORT
            }
#endif
            return;
        }

#if RFB_PROVIDER == RFB_PROVIDER_EFM8BB1
        if (t.equals(MQTT_TOPIC_RFRAW)) {
            // in case this is RAW message, we should not match anything and just send it as-is to the serial
            _rfbSendRawFromPayload(payload);
            return;
        }
#endif

    }

}

#endif // MQTT_SUPPORT

#if API_SUPPORT

void _rfbApiSetup() {

    apiReserve(3u);

    apiRegister({
        MQTT_TOPIC_RFOUT, Api::Type::Basic, ApiUnusedArg,
        apiOk, // just a stub, nothing to return
        [](const Api&, ApiBuffer& buffer) {
            _rfbSendFromPayload(buffer.data);
        }
    });

    apiRegister({
        MQTT_TOPIC_RFLEARN, Api::Type::Basic, ApiUnusedArg,
        [](const Api&, ApiBuffer& buffer) {
            if (_rfb_learn) {
                snprintf_P(buffer.data, buffer.size, PSTR("learning id:%u,status:%c"),
                    _rfb_learn->id, _rfb_learn->status ? 't' : 'f'
                );
            } else {
                snprintf_P(buffer.data, buffer.size, PSTR("waiting"));
            }
        },
        [](const Api&, ApiBuffer& buffer) {
            _rfbLearnStartFromPayload(buffer.data);
        }
    });

#if RFB_PROVIDER == RFB_PROVIDER_EFM8BB1
    apiRegister({
        MQTT_TOPIC_RFRAW, Api::Type::Basic, ApiUnusedArg,
        apiOk, // just a stub, nothing to return
        [](const Api&, ApiBuffer& buffer) {
            _rfbSendRawFromPayload(buffer.data);
        }
    });
#endif

}

#endif // API_SUPPORT

#if TERMINAL_SUPPORT

void _rfbInitCommands() {

#if RELAY_SUPPORT
    terminalRegisterCommand(F("RFB.LEARN"), [](const terminal::CommandContext& ctx) {

        if (ctx.argc != 3) {
            terminalError(ctx, F("RFB.LEARN <ID> <STATUS>"));
            return;
        }

        int id = ctx.argv[1].toInt();
        if (id >= relayCount()) {
            terminalError(ctx, F("Invalid relay ID"));
            return;
        }

        rfbLearn(id, (ctx.argv[2].toInt()) == 1);

        terminalOK(ctx);

    });

    terminalRegisterCommand(F("RFB.FORGET"), [](const terminal::CommandContext& ctx) {

        if (ctx.argc < 2) {
            terminalError(ctx, F("RFB.FORGET <ID> [<STATUS>]"));
            return;
        }

        int id = ctx.argv[1].toInt();
        if (id >= relayCount()) {
            terminalError(ctx, F("Invalid relay ID"));
            return;
        }

        if (ctx.argc == 3) {
            rfbForget(id, (ctx.argv[2].toInt()) == 1);
        } else {
            rfbForget(id, true);
            rfbForget(id, false);
        }

        terminalOK(ctx);
    });
#endif // if RELAY_SUPPORT

#if RFB_PROVIDER == RFB_PROVIDER_EFM8BB1
    terminalRegisterCommand(F("RFB.WRITE"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc != 2) {
            terminalError(ctx, F("RFB.WRITE <PAYLOAD>"));
            return;
        }
        _rfbSendRawFromPayload(ctx.argv[1].c_str());
        terminalOK(ctx);
    });
#endif

}

#endif // TERMINAL_SUPPORT

// -----------------------------------------------------------------------------
// PUBLIC
// -----------------------------------------------------------------------------

void rfbStore(unsigned char id, bool status, const char * code) {
    settings_key_t key { status ? F("rfbON") : F("rfbOFF"), id };
    setSetting(key, code);
    DEBUG_MSG_P(PSTR("[RF] Saved %s => \"%s\"\n"), key.toString().c_str(), code);
}

String rfbRetrieve(unsigned char id, bool status) {
    return getSetting({ status ? F("rfbON") : F("rfbOFF"), id });
}

void rfbStatus(unsigned char id, bool status) {
    // TODO: This is a left-over from the old implementation. Right now we set this lock when relay handler
    //       is called within the receiver, while this is called from either relayStatus or relay loop calling
    //       this via provider callback. This prevents us from re-sending the code we just received.
    // TODO: Consider having 'origin' of the relay change. Either supply relayStatus with an additional arg,
    //       or track these statuses directly.
    if (!_rfb_relay_status_lock[id]) {
        String value = rfbRetrieve(id, status);
        if (value.length() && !(value.length() & 1)) {
            _rfbSendFromPayload(value.c_str());
        }
    }

    _rfb_relay_status_lock[id] = false;
}

void rfbLearn(unsigned char id, bool status) {
    _rfb_learn.reset(new RfbLearn { millis(), id, status });
    _rfbLearnImpl();
}

void rfbForget(unsigned char id, bool status) {

    delSetting({status ? F("rfbON") : F("rfbOFF"), id});

    // Websocket update needs to happen right here, since the only time
    // we send these in bulk is at the very start of the connection
#if WEB_SUPPORT
    wsPost([id](JsonObject& root) {
        _rfbWebSocketSendCodeArray(root, id, 1);
    });
#endif

}

// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

#if RFB_PROVIDER == RFB_PROVIDER_RCSWITCH

// TODO: remove this in 1.16.0

void _rfbSettingsMigrate(int version) {
    if (!version || (version > 4)) {
        return;
    }

    auto migrate_code = [](String& out, const String& in) -> bool {
        out = "";

        if (18 == in.length()) {
            uint8_t bits { 0u };
            if (!hexDecode(in.c_str() + 8, 2, &bits, 1)) {
                return false;
            }

            auto bytes = _rfb_bytes_for_bits(bits);
            out = in.substring(0, 10);
            out += (in.c_str() + in.length() - (2 * bytes));

            return in != out;
        }

        return false;
    };

    String buffer;

    for (unsigned char index = 0; index < relayCount(); ++index) {
        const settings_key_t on_key {F("rfbON"), index};
        if (migrate_code(buffer, getSetting(on_key))) {
            setSetting(on_key, buffer);
        }

        const settings_key_t off_key {F("rfbOFF"), index};
        if (migrate_code(buffer, getSetting(off_key))) {
            setSetting(off_key, buffer);
        }
    }
}

#endif

void rfbSetup() {

#if RFB_PROVIDER == RFB_PROVIDER_EFM8BB1

    _rfb_parser.reserve(RfbParser::MessageSizeBasic);

#elif RFB_PROVIDER == RFB_PROVIDER_RCSWITCH

    _rfbSettingsMigrate(migrateVersion());

    {
        auto rx = getSetting("rfbRX", RFB_RX_PIN);
        auto tx = getSetting("rfbTX", RFB_TX_PIN);

        // TODO: tag gpioGetLock with a NAME string, skip log here
        _rfb_receive = gpioValid(rx);
        _rfb_transmit = gpioValid(tx);
        if (!_rfb_transmit && !_rfb_receive) {
            DEBUG_MSG_P(PSTR("[RF] Neither RX or TX are set\n"));
            return;
        }

        _rfb_modem = new RCSwitch();
        if (_rfb_receive) {
            _rfb_modem->enableReceive(rx);
            DEBUG_MSG_P(PSTR("[RF] RF receiver on GPIO %u\n"), rx);
        }
        if (_rfb_transmit) {
            auto transmit = getSetting("rfbTransmit", RFB_TRANSMIT_REPEATS);
            _rfb_modem->enableTransmit(tx);
            _rfb_modem->setRepeatTransmit(transmit);
            DEBUG_MSG_P(PSTR("[RF] RF transmitter on GPIO %u\n"), tx);
        }
    }

#endif

#if MQTT_SUPPORT
    mqttRegister(_rfbMqttCallback);
#endif

#if API_SUPPORT
    _rfbApiSetup();
#endif

#if WEB_SUPPORT
    wsRegister()
        .onVisible(_rfbWebSocketOnVisible)
        .onConnected(_rfbWebSocketOnConnected)
        .onData(_rfbWebSocketOnData)
        .onAction(_rfbWebSocketOnAction)
        .onKeyCheck(_rfbWebSocketOnKeyCheck);
#endif

#if TERMINAL_SUPPORT
    _rfbInitCommands();
#endif

    _rfb_repeats = getSetting("rfbRepeat", RFB_SEND_REPEATS);

    // Note: as rfbridge protocol is simplistic enough, we rely on Serial queue to deliver timely updates
    //       learn / command acks / etc. are not queued, only RF messages are
    espurnaRegisterLoop([]() {
        _rfbReceiveImpl();
        _rfbSendQueued();
    });

}

#endif // RFB_SUPPORT
