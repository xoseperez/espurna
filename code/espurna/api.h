/*

API MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"
#include "web.h"

#if WEB_SUPPORT

bool apiAuthenticate(AsyncWebServerRequest*);
bool apiEnabled();
bool apiRestFul();
String apiKey();

#endif // WEB_SUPPORT == 1

#if WEB_SUPPORT && API_SUPPORT

#include <vector>

constexpr unsigned char ApiUnusedArg = 0u;

struct ApiBuffer {
    constexpr static size_t size = API_BUFFER_SIZE;
    char data[size];

    void erase() {
        std::fill(data, data + size, '\0');
    }
};

struct Api {
    using BasicHandler = void(*)(const Api& api, ApiBuffer& buffer);
    using JsonHandler = void(*)(const Api& api, JsonObject& root);

    enum class Type {
        Basic,
        Json
    };

    Api() = delete;

    Api(const String& path_, Type type_, unsigned char arg_, BasicHandler get_, BasicHandler put_ = nullptr) :
        path(path_),
        type(type_),
        arg(arg_)
    {
        get.basic = get_;
        put.basic = put_;
    }

    Api(const String& path_, Type type_, unsigned char arg_, JsonHandler get_, JsonHandler put_ = nullptr) :
        path(path_),
        type(type_),
        arg(arg_)
    {
        get.json = get_;
        put.json = put_;
    }

    String path;
    Type type;
    unsigned char arg;

    union {
        BasicHandler basic;
        JsonHandler json;
    } get;

    union {
        BasicHandler basic;
        JsonHandler json;
    } put;
};

void apiRegister(const Api& api);

void apiCommonSetup();
void apiSetup();

void apiReserve(size_t);

void apiError(const Api&, ApiBuffer& buffer);
void apiOk(const Api&, ApiBuffer& buffer);

#endif // API_SUPPORT == 1
