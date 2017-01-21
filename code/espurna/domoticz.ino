/*

DOMOTICZ MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_DOMOTICZ

template<typename T> void domoticzSend(const char * key, T value) {
    unsigned int idx = getSetting(key).toInt();
    if (idx > 0) {
        char payload[45];
        sprintf(payload, "{\"idx\": %d, \"nvalue\": %s, \"svalue\": \"\"}", idx, String(value).c_str());
        mqttSendRaw(getSetting("dczTopicIn", DOMOTICZ_IN_TOPIC).c_str(), payload);
    }
}

#endif
