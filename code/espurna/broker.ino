/*

BROKER MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if BROKER_SUPPORT

#include <vector>

std::vector<void (*)(const unsigned char, const char *, unsigned char, const char *)> _broker_callbacks;

// -----------------------------------------------------------------------------

void brokerRegister(void (*callback)(const unsigned char, const char *, unsigned char, const char *)) {
    _broker_callbacks.push_back(callback);
}

void brokerPublish(const unsigned char type, const char * topic, unsigned char id, const char * message) {
    //DEBUG_MSG_P(PSTR("[BROKER] Message %s[%u] => %s\n"), topic, id, message);
    for (unsigned char i=0; i<_broker_callbacks.size(); i++) {
        (_broker_callbacks[i])(type, topic, id, message);
    }
}

void brokerPublish(const unsigned char type, const char * topic, const char * message) {
    brokerPublish(type, topic, 0, message);
}

#endif // BROKER_SUPPORT
