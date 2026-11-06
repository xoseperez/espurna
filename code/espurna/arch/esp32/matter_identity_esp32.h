/*

MATTER BACKEND FOR ESP32: OWN BOARD IDENTITY (ref. matter_identity_esp32.cpp)

*/

#pragma once

#include <Arduino.h>

#include <cstdint>

namespace espurna {
namespace matter {
namespace backend {
namespace identity {

// Before Matter.begin(): this board's setup passcode and discriminator, made on the first call and kept in NVS.
// false: NVS failed, the SDK test codes stay
bool load(uint32_t& passcode, uint16_t& discriminator);

// After Matter.begin(): own device attestation (key and DAC made on the first call) in place of the SDK test one
bool install();

// For the status command, no secrets
String status();

} // namespace identity
} // namespace backend
} // namespace matter
} // namespace espurna
