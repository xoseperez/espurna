/*

LightFox module

Copyright (C) 2019 by Andrey F. Kupreychik <foxle@quickfox.ru>

*/

#pragma once

#include <cstddef>
#include <memory>

class RelayProviderBase;
std::unique_ptr<RelayProviderBase> lightfoxMakeRelayProvider(size_t);

void lightfoxSetup();
