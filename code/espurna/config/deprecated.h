#pragma once

// 1.13.3 added TELNET_PASSWORD build-only flag
// 1.13.4 replaces it with TELNET_AUTHENTICATION runtime setting default
// TODO warning should be removed eventually
#ifdef TELNET_PASSWORD
#warning TELNET_PASSWORD is deprecated! Please replace it with TELNET_AUTHENTICATION
#define TELNET_AUTHENTICATION TELNET_PASSWORD
#endif

// 1.14.0 combines RF_SUPPORT with RFB_DIRECT
#ifdef RF_PIN
#warning RF_PIN is deprecated! Please use RFB_RX_PIN instead
#define RFB_RX_PIN RF_PIN
#endif

// 1.14.0 allow multiple digitals
#ifdef DIGITAL_PIN
#warning DIGITAL_PIN is deprecated! Please use DIGITAL1_PIN instead
#define DIGITAL1_PIN DIGITAL_PIN
#endif

// 1.14.0 allow multiple events
#ifdef EVENTS_PIN
#warning EVENTS_PIN is deprecated! Please use EVENTS1_PIN instead
#define EVENTS1_PIN EVENTS_PIN
#endif

// 1.14.0 unifies mqtt payload options
#ifdef HOMEASSISTANT_PAYLOAD_ON
#warning HOMEASSISTANT_PAYLOAD_ON is deprecated! Global RELAY_MQTT_ON is used instead
#endif

#ifdef HOMEASSISTANT_PAYLOAD_OFF
#warning HOMEASSISTANT_PAYLOAD_OFF is deprecated! Global RELAY_MQTT_OFF is used instead
#endif

#ifdef HOMEASSISTANT_PAYLOAD_AVAILABLE
#warning HOMEASSISTANT_PAYLOAD_AVAILABLE is deprecated! Global MQTT_STATUS_ONLINE is used instead
#endif

#ifdef HOMEASSISTANT_PAYLOAD_NOT_AVAILABLE
#warning HOMEASSISTANT_PAYLOAD_NOT_AVAILABLE is deprecated! Global MQTT_STATUS_OFFLINE is used instead
#endif

// 1.14.0 adds SecureClient
#if MQTT_SUPPORT && MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTT_CLIENT && ASYNC_TCP_SSL_ENABLED
#warning "Current implementation of AsyncMqttClient with axTLS is no longer supported. Consider switching to the SECURE_CLIENT configuration with MQTT_LIBRARY_ARDUINOMQTT or MQTT_LIBRARY_PUBSUBCLIENT. See: https://github.com/xoseperez/espurna/issues/1465"
#endif
