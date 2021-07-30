/*

RELAY MODULE

*/

#pragma once

#include "espurna.h"

namespace relay {
namespace build {

constexpr unsigned long saveDelay() {
    return RELAY_SAVE_DELAY;
}

constexpr size_t dummyCount() {
    return DUMMY_RELAY_COUNT;
}

constexpr int syncMode() {
    return RELAY_SYNC;
}

constexpr float floodWindow() {
    return RELAY_FLOOD_WINDOW;
}

static_assert(floodWindow() >= 0.0f, "");

constexpr unsigned long floodWindowMs() {
    return static_cast<unsigned long>(floodWindow() * 1000.0f);
}

constexpr unsigned long floodChanges() {
    return RELAY_FLOOD_CHANGES;
}

constexpr unsigned long interlockDelay() {
    return RELAY_DELAY_INTERLOCK;
}

constexpr float pulseTime(size_t index) {
    return (
        (index == 0) ? RELAY1_PULSE_TIME :
        (index == 1) ? RELAY2_PULSE_TIME :
        (index == 2) ? RELAY3_PULSE_TIME :
        (index == 3) ? RELAY4_PULSE_TIME :
        (index == 4) ? RELAY5_PULSE_TIME :
        (index == 5) ? RELAY6_PULSE_TIME :
        (index == 6) ? RELAY7_PULSE_TIME :
        (index == 7) ? RELAY8_PULSE_TIME : RELAY_PULSE_TIME
    );
}

static_assert(pulseTime(0) >= 0.0f, "");
static_assert(pulseTime(1) >= 0.0f, "");
static_assert(pulseTime(2) >= 0.0f, "");
static_assert(pulseTime(3) >= 0.0f, "");
static_assert(pulseTime(4) >= 0.0f, "");
static_assert(pulseTime(5) >= 0.0f, "");
static_assert(pulseTime(6) >= 0.0f, "");
static_assert(pulseTime(7) >= 0.0f, "");

constexpr RelayPulse pulseMode(size_t index) {
    return (
        (index == 0) ? RELAY1_PULSE_MODE :
        (index == 1) ? RELAY2_PULSE_MODE :
        (index == 2) ? RELAY3_PULSE_MODE :
        (index == 3) ? RELAY4_PULSE_MODE :
        (index == 4) ? RELAY5_PULSE_MODE :
        (index == 5) ? RELAY6_PULSE_MODE :
        (index == 6) ? RELAY7_PULSE_MODE :
        (index == 7) ? RELAY8_PULSE_MODE : RELAY_PULSE_NONE
    );
}

constexpr unsigned long delayOn(size_t index) {
    return (
        (index == 0) ? RELAY1_DELAY_ON :
        (index == 1) ? RELAY2_DELAY_ON :
        (index == 2) ? RELAY3_DELAY_ON :
        (index == 3) ? RELAY4_DELAY_ON :
        (index == 4) ? RELAY5_DELAY_ON :
        (index == 5) ? RELAY6_DELAY_ON :
        (index == 6) ? RELAY7_DELAY_ON :
        (index == 7) ? RELAY8_DELAY_ON : 0ul
    );
}

constexpr unsigned long delayOff(size_t index) {
    return (
        (index == 0) ? RELAY1_DELAY_OFF :
        (index == 1) ? RELAY2_DELAY_OFF :
        (index == 2) ? RELAY3_DELAY_OFF :
        (index == 3) ? RELAY4_DELAY_OFF :
        (index == 4) ? RELAY5_DELAY_OFF :
        (index == 5) ? RELAY6_DELAY_OFF :
        (index == 6) ? RELAY7_DELAY_OFF :
        (index == 7) ? RELAY8_DELAY_OFF : 0ul
    );
}

constexpr unsigned char pin(size_t index) {
    return (
        (index == 0) ? RELAY1_PIN :
        (index == 1) ? RELAY2_PIN :
        (index == 2) ? RELAY3_PIN :
        (index == 3) ? RELAY4_PIN :
        (index == 4) ? RELAY5_PIN :
        (index == 5) ? RELAY6_PIN :
        (index == 6) ? RELAY7_PIN :
        (index == 7) ? RELAY8_PIN : GPIO_NONE
    );
}

constexpr RelayType type(size_t index) {
    return (
        (index == 0) ? RELAY1_TYPE :
        (index == 1) ? RELAY2_TYPE :
        (index == 2) ? RELAY3_TYPE :
        (index == 3) ? RELAY4_TYPE :
        (index == 4) ? RELAY5_TYPE :
        (index == 5) ? RELAY6_TYPE :
        (index == 6) ? RELAY7_TYPE :
        (index == 7) ? RELAY8_TYPE : RELAY_TYPE_NORMAL
    );
}

constexpr GpioType pinType(size_t index) {
    return (
        (index == 0) ? RELAY1_PIN_TYPE :
        (index == 1) ? RELAY2_PIN_TYPE :
        (index == 2) ? RELAY3_PIN_TYPE :
        (index == 3) ? RELAY4_PIN_TYPE :
        (index == 4) ? RELAY5_PIN_TYPE :
        (index == 5) ? RELAY6_PIN_TYPE :
        (index == 6) ? RELAY7_PIN_TYPE :
        (index == 7) ? RELAY8_PIN_TYPE : GPIO_TYPE_NONE
    );
}

constexpr unsigned char resetPin(size_t index) {
    return (
        (index == 0) ? RELAY1_RESET_PIN :
        (index == 1) ? RELAY2_RESET_PIN :
        (index == 2) ? RELAY3_RESET_PIN :
        (index == 3) ? RELAY4_RESET_PIN :
        (index == 4) ? RELAY5_RESET_PIN :
        (index == 5) ? RELAY6_RESET_PIN :
        (index == 6) ? RELAY7_RESET_PIN :
        (index == 7) ? RELAY8_RESET_PIN : GPIO_NONE
    );
}

constexpr int bootMode(size_t index) {
    return (
        (index == 0) ? RELAY1_BOOT_MODE :
        (index == 1) ? RELAY2_BOOT_MODE :
        (index == 2) ? RELAY3_BOOT_MODE :
        (index == 3) ? RELAY4_BOOT_MODE :
        (index == 4) ? RELAY5_BOOT_MODE :
        (index == 5) ? RELAY6_BOOT_MODE :
        (index == 6) ? RELAY7_BOOT_MODE :
        (index == 7) ? RELAY8_BOOT_MODE : RELAY_BOOT_OFF
    );
}

constexpr RelayProvider provider(size_t index) {
    return (
        (index == 0) ? (RELAY1_PROVIDER) :
        (index == 1) ? (RELAY2_PROVIDER) :
        (index == 2) ? (RELAY3_PROVIDER) :
        (index == 3) ? (RELAY4_PROVIDER) :
        (index == 4) ? (RELAY5_PROVIDER) :
        (index == 5) ? (RELAY6_PROVIDER) :
        (index == 6) ? (RELAY7_PROVIDER) :
        (index == 7) ? (RELAY8_PROVIDER) : RELAY_PROVIDER_NONE
    );
}

constexpr RelayMqttTopicMode mqttTopicMode(size_t index) {
    return (
        (index == 0) ? (RELAY1_MQTT_TOPIC_MODE) :
        (index == 1) ? (RELAY2_MQTT_TOPIC_MODE) :
        (index == 2) ? (RELAY3_MQTT_TOPIC_MODE) :
        (index == 3) ? (RELAY4_MQTT_TOPIC_MODE) :
        (index == 4) ? (RELAY5_MQTT_TOPIC_MODE) :
        (index == 5) ? (RELAY6_MQTT_TOPIC_MODE) :
        (index == 6) ? (RELAY7_MQTT_TOPIC_MODE) :
        (index == 7) ? (RELAY8_MQTT_TOPIC_MODE) : RELAY_MQTT_TOPIC_MODE
    );
}

constexpr const char* const mqttPayloadOn() {
    return RELAY_MQTT_ON;
}

constexpr const char* const mqttPayloadOff() {
    return RELAY_MQTT_OFF;
}

constexpr const char* const mqttPayloadToggle() {
    return RELAY_MQTT_TOGGLE;
}

constexpr const char* const mqttTopicSub(size_t index) {
    return (
        (index == 0) ? (RELAY1_MQTT_TOPIC_SUB) :
        (index == 1) ? (RELAY2_MQTT_TOPIC_SUB) :
        (index == 2) ? (RELAY3_MQTT_TOPIC_SUB) :
        (index == 3) ? (RELAY4_MQTT_TOPIC_SUB) :
        (index == 4) ? (RELAY5_MQTT_TOPIC_SUB) :
        (index == 5) ? (RELAY6_MQTT_TOPIC_SUB) :
        (index == 6) ? (RELAY7_MQTT_TOPIC_SUB) :
        (index == 7) ? (RELAY8_MQTT_TOPIC_SUB) : ""
    );
}

constexpr const char* const mqttTopicPub(size_t index) {
    return (
        (index == 0) ? (RELAY1_MQTT_TOPIC_PUB) :
        (index == 1) ? (RELAY2_MQTT_TOPIC_PUB) :
        (index == 2) ? (RELAY3_MQTT_TOPIC_PUB) :
        (index == 3) ? (RELAY4_MQTT_TOPIC_PUB) :
        (index == 4) ? (RELAY5_MQTT_TOPIC_PUB) :
        (index == 5) ? (RELAY6_MQTT_TOPIC_PUB) :
        (index == 6) ? (RELAY7_MQTT_TOPIC_PUB) :
        (index == 7) ? (RELAY8_MQTT_TOPIC_PUB) : ""
    );
}

constexpr PayloadStatus mqttDisconnectionStatus(size_t index) {
    return (
        (index == 0) ? (RELAY1_MQTT_DISCONNECT_STATUS) :
        (index == 1) ? (RELAY2_MQTT_DISCONNECT_STATUS) :
        (index == 2) ? (RELAY3_MQTT_DISCONNECT_STATUS) :
        (index == 3) ? (RELAY4_MQTT_DISCONNECT_STATUS) :
        (index == 4) ? (RELAY5_MQTT_DISCONNECT_STATUS) :
        (index == 5) ? (RELAY6_MQTT_DISCONNECT_STATUS) :
        (index == 6) ? (RELAY7_MQTT_DISCONNECT_STATUS) :
        (index == 7) ? (RELAY8_MQTT_DISCONNECT_STATUS) : RELAY_MQTT_DISCONNECT_NONE
    );
}

} // namespace build
} // namespace relay
