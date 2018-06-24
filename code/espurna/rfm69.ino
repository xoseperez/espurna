/*

RFM69 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if RFM69_SUPPORT

#include "RFM69Manager.h"

// -----------------------------------------------------------------------------
// Locals
// -----------------------------------------------------------------------------

RFM69Manager * _rfm69_radio;

struct _node_t {
    unsigned long count = 0;
    unsigned long missing = 0;
    unsigned long duplicates = 0;
    unsigned char lastPacketID = 0;
};

_node_t _rfm69_node_info[255];
unsigned char _rfm69_node_count;
unsigned long _rfm69_packet_count;

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

void _rfm69WebSocketOnSend(JsonObject& root) {

    root["rfm69Visible"] = 1;
    root["rfm69Topic"] = getSetting("rfm69Topic", RFM69_DEFAULT_TOPIC);
    root["packetCount"] = _rfm69_packet_count;
    root["nodeCount"] = _rfm69_node_count;
    JsonArray& mappings = root.createNestedArray("mapping");
    for (unsigned char i=0; i<RFM69_MAX_TOPICS; i++) {
        unsigned char node = getSetting("node", i, 0).toInt();
        if (0 == node) break;
        JsonObject& mapping = mappings.createNestedObject();
        mapping["node"] = node;
        mapping["key"] = getSetting("key", i, "");
        mapping["topic"] = getSetting("topic", i, "");
    }

}

bool _rfm69WebSocketOnReceive(const char * key, JsonVariant& value) {
    if (strncmp(key, "rfm69", 5) == 0) return true;
    if (strncmp(key, "node", 4) == 0) return true;
    if (strncmp(key, "key", 3) == 0) return true;
    if (strncmp(key, "topic", 5) == 0) return true;
    return false;
}

void _rfm69WebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {
    if (strcmp(action, "clear-counts") == 0) _rfm69Clear();
}


#endif // WEB_SUPPORT

void _rfm69CleanNodes(unsigned char num) {

    // Look for the last defined node
    int i = 0;
    while (i < num) {
        if (getSetting("node", i, 0).toInt() == 0) break;
        if (getSetting("key", i, "").length() == 0) break;
        if (getSetting("topic", i, "").length() == 0) break;
        ++i;
    }

    // Delete all other settings
    while (i < WIFI_MAX_NETWORKS) {
        delSetting("node", i);
        delSetting("key", i);
        delSetting("topic", i);
        ++i;
    }

}

void _rfm69Configure() {
    _rfm69CleanNodes(RFM69_MAX_TOPICS);
}

// -----------------------------------------------------------------------------
// Radio
// -----------------------------------------------------------------------------

void _rfm69Debug(const char * level, packet_t * data) {

    DEBUG_MSG_P(
        PSTR("[RFM69] %s: messageID:%05d senderID:%03d targetID:%03d packetID:%03d rssi:%-04d name:%s value:%s\n"),
        level,
        data->messageID,
        data->senderID,
        data->targetID,
        data->packetID,
        data->rssi,
        data->name,
        data->value
    );

}

void _rfm69Process(packet_t * data) {

    // Count seen nodes and packets
    if (_rfm69_node_info[data->senderID].count == 0) ++_rfm69_node_count;
    ++_rfm69_packet_count;

    // Detect duplicates and missing packets
    // packetID==0 means device is not sending packetID info
    if (data->packetID > 0) {
        if (_rfm69_node_info[data->senderID].count > 0) {

            unsigned char gap = data->packetID - _rfm69_node_info[data->senderID].lastPacketID;

            if (gap == 0) {
                _rfm69_node_info[data->senderID].duplicates = _rfm69_node_info[data->senderID].duplicates + 1;
                //_rfm69Debug("DUP", data);
                return;
            }

            if ((gap > 1) && (data->packetID > 1)) {
                _rfm69_node_info[data->senderID].missing = _rfm69_node_info[data->senderID].missing + gap - 1;
                DEBUG_MSG_P(PSTR("[RFM69] %u missing packets detected\n"), gap - 1);
            }
        }

    }

    _rfm69Debug("OK ", data);

    _rfm69_node_info[data->senderID].lastPacketID = data->packetID;
    _rfm69_node_info[data->senderID].count = _rfm69_node_info[data->senderID].count + 1;

    // Send info to websocket clients
    {
        char buffer[200];
        snprintf_P(
            buffer,
            sizeof(buffer) - 1,
            PSTR("{\"nodeCount\": %d, \"packetCount\": %lu, \"packet\": {\"senderID\": %u, \"targetID\": %u, \"packetID\": %u, \"name\": \"%s\", \"value\": \"%s\", \"rssi\": %d, \"duplicates\": %d, \"missing\": %d}}"),
            _rfm69_node_count, _rfm69_packet_count,
            data->senderID, data->targetID, data->packetID, data->name, data->value, data->rssi,
            _rfm69_node_info[data->senderID].duplicates , _rfm69_node_info[data->senderID].missing);
        wsSend(buffer);
    }

    // If we are the target of the message, forward it via MQTT, otherwise quit
    if (!RFM69_PROMISCUOUS_SENDS && (RFM69_GATEWAY_ID != data->targetID)) return;

    // Try to find a matching mapping
    for (unsigned int i=0; i<RFM69_MAX_TOPICS; i++) {
        unsigned char node = getSetting("node", i, 0).toInt();
        if (0 == node) break;
        if ((node == data->senderID) && (getSetting("key", i, "").equals(data->name))) {
            mqttSendRaw((char *) getSetting("topic", i, "").c_str(), (char *) String(data->value).c_str());
            return;
        }
    }

    // Mapping not found, use default topic
    String topic = getSetting("rfm69Topic", RFM69_DEFAULT_TOPIC);
    if (topic.length() > 0) {
        topic.replace("{node}", String(data->senderID));
        topic.replace("{key}", String(data->name));
        mqttSendRaw((char *) topic.c_str(), (char *) String(data->value).c_str());
    }

}

void _rfm69Loop() {
    _rfm69_radio->loop();
}

void _rfm69Clear() {
    for(unsigned int i=0; i<255; i++) {
        _rfm69_node_info[i].duplicates = 0;
        _rfm69_node_info[i].missing = 0;
    }
    _rfm69_node_count = 0;
    _rfm69_packet_count = 0;
}

// -----------------------------------------------------------------------------
// RFM69
// -----------------------------------------------------------------------------

void rfm69Setup() {

    delay(10);

    _rfm69Configure();

    _rfm69_radio = new RFM69Manager(RFM69_CS_PIN, RFM69_IRQ_PIN, RFM69_IS_RFM69HW, digitalPinToInterrupt(RFM69_IRQ_PIN));
    _rfm69_radio->initialize(RFM69_FREQUENCY, RFM69_NODE_ID, RFM69_NETWORK_ID, RFM69_ENCRYPTKEY);
    _rfm69_radio->promiscuous(RFM69_PROMISCUOUS);
    _rfm69_radio->onMessage(_rfm69Process);

    #if WEB_SUPPORT
        wsOnSendRegister(_rfm69WebSocketOnSend);
        wsOnReceiveRegister(_rfm69WebSocketOnReceive);
        wsOnAfterParseRegister(_rfm69Configure);
        wsOnActionRegister(_rfm69WebSocketOnAction);
    #endif

    // Register loop
    espurnaRegisterLoop(_rfm69Loop);

}

#endif // RFM69_SUPPORT
