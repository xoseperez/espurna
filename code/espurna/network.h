/*

NETWORKING MODULE

Copyright (C) 2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>
#include <IPAddress.h>

namespace espurna {
namespace network {

using IpFoundCallback = std::function<void(const String& hostname, IPAddress)>;

} // namespace network
} // namespace espurna

IPAddress networkGetHostByName(String);
void networkGetHostByName(String, espurna::network::IpFoundCallback);
void networkSetup();
