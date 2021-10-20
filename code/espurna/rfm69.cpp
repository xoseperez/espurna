/*

RFM69 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if RFM69_SUPPORT

#define RFM69_PACKET_SEPARATOR ':'

#include <RFM69.h>
#include <RFM69_ATC.h>
#include <SPI.h>

#include "rfm69.h"
#include "mqtt.h"
#include "ws.h"

// -----------------------------------------------------------------------------
// PRIVATE
// -----------------------------------------------------------------------------

namespace rfm69 {

struct Message {
    unsigned long id;
    unsigned char packetID;
    unsigned char senderID;
    unsigned char targetID;
    String key;
    String value;
    int16_t rssi;
};

struct Node {
    unsigned long count = 0;
    unsigned long missing = 0;
    unsigned long duplicates = 0;
    unsigned char lastPacketID = 0;
};

struct Mapping {
    size_t node;
    String key;
    String topic;
};

namespace build {

constexpr size_t maxTopics() {
    return RFM69_MAX_TOPICS;
}

constexpr size_t maxNodes() {
    return RFM69_MAX_NODES;
}

constexpr uint8_t cs() {
    return RFM69_CS_PIN;
}

constexpr uint8_t irq() {
    return RFM69_IRQ_PIN;
}

constexpr bool hardware() {
    return 1 == RFM69_IS_RFM69HW;
}

constexpr uint8_t frequency() {
    return RFM69_FREQUENCY;
}

constexpr uint16_t nodeId() {
    return RFM69_NODE_ID;
}

constexpr uint8_t networkId() {
    return RFM69_NETWORK_ID;
}

constexpr uint8_t gatewayId() {
    return RFM69_GATEWAY_ID;
}

const char* const encryptionKey() {
    return RFM69_ENCRYPTKEY;
}

constexpr bool promiscuous() {
    return 1 == RFM69_PROMISCUOUS;
}

constexpr bool promiscuousSends() {
    return 1 == RFM69_PROMISCUOUS_SENDS;
}

const __FlashStringHelper* rootTopic() {
    return F(RFM69_DEFAULT_TOPIC);
}

constexpr size_t node(size_t) {
    return 0;
}

} // namespace build

namespace settings {

String rootTopic() {
    return getSetting("rfm69Topic", build::rootTopic());
}

String topic(size_t index) {
    return getSetting({"rfm69Topic", index});
}

String key(size_t index) {
    return getSetting({"rfm69Key", index});
}

size_t node(size_t index) {
    return getSetting({"rfm69Node", index}, build::node(index));
}

template <typename T>
void foreachMapping(T&& callback) {
    for (size_t index = 0; index < build::maxTopics(); ++index) {
        auto currentNode = node(index);
        if (0 == currentNode) {
            break;
        }

        Mapping entry{currentNode, key(index), topic(index)};
        if (!entry.key.length() || !entry.topic.length()) {
            break;
        }

        if (!callback(std::move(entry))) {
            break;
        }
    }
}

std::vector<Mapping> mapping() {
    std::vector<Mapping> out;
    foreachMapping([&](rfm69::Mapping&& entry) {
        out.emplace_back(std::move(entry));
        return true;
    });

    return out;
}

} // namespace settings
} // namespace rfm69

// -----------------------------------------------------------------------------

class RFM69Wrap: public RFM69_ATC {
public:
    using RFM69_ATC::RFM69_ATC;

protected:
    // overriding SPI_CLOCK for ESP8266
    void select() {
        noInterrupts();

#if defined (SPCR) && defined (SPSR)
        // save current SPI settings
        _SPCR = SPCR;
        _SPSR = SPSR;
#endif

        // set RFM69 SPI settings
        SPI.setDataMode(SPI_MODE0);
        SPI.setBitOrder(MSBFIRST);

#if defined(__arm__)
        SPI.setClockDivider(SPI_CLOCK_DIV16);
#elif defined(ARDUINO_ARCH_ESP8266)
        SPI.setClockDivider(SPI_CLOCK_DIV2); // speeding it up for the ESP8266
#else
        SPI.setClockDivider(SPI_CLOCK_DIV4);
#endif

        digitalWrite(_slaveSelectPin, LOW);
    }
};

namespace {

std::unique_ptr<RFM69Wrap> _rfm69_radio;
rfm69::Node _rfm69_node_info[rfm69::build::maxNodes()];
size_t _rfm69_node_count;
size_t _rfm69_packet_count;

void _rfm69Clear() {
    for (auto& info : _rfm69_node_info) {
        info.duplicates = 0;
        info.missing = 0;
        info.count = 0;
    }
    _rfm69_node_count = 0;
    _rfm69_packet_count = 0;
}

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _rfm69WebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "rfm69");
}

void _rfm69WebSocketOnConnected(JsonObject& root) {
    root["rfm69Topic"] = rfm69::settings::rootTopic();

    JsonObject& rfm69 = root.createNestedObject("rfm69");
    rfm69["packets"] = _rfm69_packet_count; // TODO: unused?
    rfm69["nodes"] = _rfm69_node_count; // TODO: unused?

    static const char* const keys[] {
        "rfm69Node", "rfm69Key", "rfm69Topic"
    };
    JsonArray& schema = rfm69.createNestedArray("schema");
    schema.copyFrom(keys, sizeof(keys) / sizeof(*keys));

    JsonArray& mappings = rfm69.createNestedArray("mapping");
    for (auto& mapping : rfm69::settings::mapping()) {
        JsonArray& entry = mappings.createNestedArray();
        entry.add(mapping.node);
        entry.add(mapping.key);
        entry.add(mapping.topic);
    }
}

bool _rfm69WebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "rfm69", 5) == 0);
}

void _rfm69WebSocketOnAction(uint32_t client_id, const char* action, JsonObject& data) {
    if (strcmp(action, "rfm69Clear") == 0) {
        _rfm69Clear();
    }
}

#endif // WEB_SUPPORT

void _rfm69CleanNodes(size_t max) {
    size_t id { 0 };
    rfm69::settings::foreachMapping([&](rfm69::Mapping&&) {
        if (id < max) {
            return false;
        }

        ++id;
        return true;
    });

    while (id < rfm69::build::maxTopics()) {
        delSetting({"rfm69Node", id});
        delSetting({"rfm69Key", id});
        delSetting({"rfm69Topic", id});
        ++id;
    }
}

void _rfm69Configure() {
    _rfm69CleanNodes(rfm69::build::maxTopics());
}

// -----------------------------------------------------------------------------
// Radio
// -----------------------------------------------------------------------------

void _rfm69Debug(const char* prefix, const rfm69::Message& message) {
    DEBUG_MSG_P(PSTR("[RFM69] %s: message ID:%05u sender:%03hhu target:%03hhu packet:%03hhu rssi:%-04hd key:%s value:%s\n"),
        prefix,
        message.id, message.senderID, message.targetID, message.packetID,
        message.rssi, message.key.c_str(), message.value.c_str());
}

void _rfm69Process(const rfm69::Message& message) {
    // Is node beyond RFM69_MAX_NODES?
    if (message.senderID >= rfm69::build::maxNodes()) {
        return;
    }

    // Count seen nodes
    if (_rfm69_node_info[message.senderID].count == 0) {
        ++_rfm69_node_count;
    }

    // Detect duplicates and missing messages
    // message ID==0 means device is not sending this info
    if (message.id > 0) {
        if (_rfm69_node_info[message.senderID].count > 0) {
            auto gap = message.packetID - _rfm69_node_info[message.senderID].lastPacketID;
            if (gap == 0) {
                _rfm69_node_info[message.senderID].duplicates = _rfm69_node_info[message.senderID].duplicates + 1;
                return;
            }

            constexpr decltype(gap) Offset { 1 };
            if ((gap > Offset) && (message.id > Offset)) {
                _rfm69_node_info[message.senderID].missing = _rfm69_node_info[message.senderID].missing + gap - Offset;
                DEBUG_MSG_P(PSTR("[RFM69] %hhu missing messages detected\n"), gap - Offset);
            }
        }
    }

    _rfm69Debug("OK", message);

    _rfm69_node_info[message.senderID].lastPacketID = message.packetID;
    _rfm69_node_info[message.senderID].count += 1;

#if WEB_SUPPORT
    {
        auto& info = _rfm69_node_info[message.senderID];

        auto duplicates = info.duplicates;
        auto missing = info.missing;

        wsPost([message, duplicates, missing](JsonObject& root) {
            JsonObject& rfm69 = root.createNestedObject("rfm69");
            rfm69["packets"] = _rfm69_packet_count; // TODO: unused?
            rfm69["nodes"] = _rfm69_node_count; // TODO: unused?

            JsonArray& msg = rfm69.createNestedArray("message");
            msg.add(message.packetID);
            msg.add(message.senderID);
            msg.add(message.targetID);
            msg.add(message.key);
            msg.add(message.value);
            msg.add(message.rssi);
            msg.add(duplicates);
            msg.add(missing);
        });
    }
#endif

    // If we are the target of the message, forward it via MQTT, otherwise quit
    if (!rfm69::build::promiscuousSends() && (rfm69::build::gatewayId() != message.targetID)) {
        return;
    }

#if MQTT_SUPPORT
    // Try to find a matching mapping
    bool found { false };
    rfm69::settings::foreachMapping([&](rfm69::Mapping&& mapping) {
        if ((mapping.node == message.senderID) && (mapping.key == message.key)) {
            found = true;
            mqttSendRaw(mapping.topic.c_str(), message.value.c_str());
            return false;
        }

        return true;
    });

    // Mapping not found, use default topic
    if (!found) {
        auto topic = rfm69::settings::rootTopic();
        if (topic.length() > 0) {
            topic.replace("{node}", String(message.senderID, 10));
            topic.replace("{key}", message.key);
            mqttSendRaw(topic.c_str(), message.value.c_str());
        }
    }
#endif
}

void _rfm69Loop() {
    if (_rfm69_radio->receiveDone()) {
        uint8_t senderID = _rfm69_radio->SENDERID;
        uint8_t targetID = _rfm69_radio->TARGETID;
        int16_t rssi = _rfm69_radio->RSSI;
        uint8_t length = _rfm69_radio->DATALEN;
        char buffer[length + 1];
        strncpy(buffer, (const char *) _rfm69_radio->DATA, length);
        buffer[length] = 0;

        // Do not send ACKs in promiscuous mode,
        // we want to listen without being heard
        if (!rfm69::build::promiscuous()) {
            if (_rfm69_radio->ACKRequested()) {
                _rfm69_radio->sendACK();
            }
        }

        uint8_t parts = 1;
        for (uint8_t i = 0; i < length; ++i) {
            if (buffer[i] == RFM69_PACKET_SEPARATOR) {
                ++parts;
            }
        }

        if (parts > 1) {
            char sep[2] = {RFM69_PACKET_SEPARATOR, 0};

            uint8_t packetID = 0;
            char* key = strtok(buffer, sep);
            char* value = strtok(NULL, sep);
            if (parts > 2) {
                char * packet = strtok(NULL, sep);
                packetID = atoi(packet);
            }

            _rfm69Process(rfm69::Message{
                ++_rfm69_packet_count,
                packetID,
                senderID,
                targetID,
                key,
                value,
                rssi
            });
        }
    }
}

void _rfm69SettingsMigrate(int version) {
    if (version < 8) {
        moveSettings("node", "rfm69Node");
        moveSettings("key", "rfm69Key");
        moveSettings("topic", "rfm69Topic");
    }
}

} // namespace

// -----------------------------------------------------------------------------
// RFM69
// -----------------------------------------------------------------------------

void rfm69Setup() {
    delay(10);

    migrateVersion(_rfm69SettingsMigrate);
    _rfm69Configure();

    _rfm69_radio = std::make_unique<RFM69Wrap>(rfm69::build::cs(), rfm69::build::irq(), rfm69::build::hardware());
    _rfm69_radio->initialize(rfm69::build::frequency(), rfm69::build::nodeId(), rfm69::build::networkId());
    _rfm69_radio->encrypt(rfm69::build::encryptionKey());
    _rfm69_radio->spyMode(rfm69::build::promiscuous());
    _rfm69_radio->enableAutoPower(0);
    if (rfm69::build::hardware()) {
        _rfm69_radio->setHighPower();
    }

    DEBUG_MSG_P(PSTR("[RFM69] Working at %d MHz\n"),
            (rfm69::build::frequency() == RF69_433MHZ) ? 433 :
            (rfm69::build::frequency() == RF69_868MHZ) ? 868 : 915);
    DEBUG_MSG_P(PSTR("[RFM69] Node %u\n"), rfm69::build::nodeId());
    DEBUG_MSG_P(PSTR("[RFM69] Network %u\n"), rfm69::build::networkId());
    if (rfm69::build::promiscuous()) {
        DEBUG_MSG_P(PSTR("[RFM69] Promiscuous mode\n"));
    }

#if WEB_SUPPORT
    wsRegister()
        .onVisible(_rfm69WebSocketOnVisible)
        .onConnected(_rfm69WebSocketOnConnected)
        .onAction(_rfm69WebSocketOnAction)
        .onKeyCheck(_rfm69WebSocketOnKeyCheck);
#endif

    espurnaRegisterLoop(_rfm69Loop);
    espurnaRegisterReload(_rfm69Configure);
}

#endif // RFM69_SUPPORT
