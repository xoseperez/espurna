#pragma once

// 1.13.3 added TELNET_PASSWORD build-only flag
// 1.13.4 replaces it with TELNET_AUTHENTICATION runtime setting default
// TODO warning should be removed eventually
#ifdef TELNET_PASSWORD
#warning TELNET_PASSWORD is deprecated! Please replace it with TELNET_AUTHENTICATION
#define TELNET_AUTHENTICATION TELNET_PASSWORD
#endif

// 1.13.6 combines RF_SUPPORT with RFB_DIRECT
#ifdef RF_PIN
#warning RF_PIN is deprecated! Please use RFB_RX_PIN instead
#define RFB_RX_PIN RF_PIN
#endif

// 1.13.6 allow multiple digitals
#ifdef DIGITAL_PIN
#warning DIGITAL_PIN is deprecated! Please use DIGITAL1_PIN instead
#define DIGITAL1_PIN DIGITAL_PIN
#endif

// 1.13.6 allow multiple events
#ifdef EVENTS_PIN
#warning EVENTS_PIN is deprecated! Please use EVENTS1_PIN instead
#define EVENTS1_PIN EVENTS_PIN
#endif