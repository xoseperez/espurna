/*

NTP MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if RPN_RULES_SUPPORT

#include "rpnlib.h"
#include <Ticker.h>

// -----------------------------------------------------------------------------
// Custom commands
// -----------------------------------------------------------------------------

rpn_context _rpn_ctxt;
bool _rpn_run = false;
unsigned long _rpn_last = 0;

// -----------------------------------------------------------------------------

bool _rpnWebSocketOnReceive(const char * key, JsonVariant& value) {
    return (strncmp(key, "rpn", 3) == 0);
}

void _rpnWebSocketOnSend(JsonObject& root) {
    
    root["rpnVisible"] = 1;
    JsonArray& rules = root.createNestedArray("rpnRules");
    
    unsigned char i = 0;
    while (String rule = getSetting("rule", i, NULL)) {
        rules.add(rule);
    }

}

void _rpnConfigure() {

}

void _rpnBrokerCallback(const unsigned char type, const char * topic, unsigned char id, const char * payload) {
    
    char name[32] = {0};

    if (BROKER_MSG_TYPE_STATUS == type || BROKER_MSG_TYPE_SENSOR == type) {
        snprintf(name, sizeof(name), "%s%d", topic, id);
    } else if (BROKER_MSG_TYPE_DATETIME == type) {
        strncpy(name, topic, sizeof(name));
    } else {
        return;
    }

    rpn_variable_set(_rpn_ctxt, name, atof(payload));
    _rpn_last = millis();
    _rpn_run = true;

}

void _rpnInit() {

    // Init context
    rpn_init(_rpn_ctxt);

    // Add relay operator
    rpn_operator_set(_rpn_ctxt, "relay", 2, [](rpn_context & ctxt) {
        float a, b;
        rpn_stack_pop(ctxt, b); // new status
        rpn_stack_pop(ctxt, a); // relay number
        relayStatus(int(a), int(b));
        return true;
    });    

}

void _rpnRun() {

    unsigned char i = 0;
    while (String rule = getSetting("rule", i, NULL)) {
        rpn_stack_clear(_rpn_ctxt);
        rpn_process(_rpn_ctxt, rule.c_str());
    }

}

void _rpnLoop() {
    
    if (_rpn_run && (millis() - _rpn_last > RPN_BUFFER_DELAY)) {
        _rpnRun();
        _rpn_run = false;
    }

}

void rpnSetup() {

    // Init context
    _rpnInit();

    // Load & cache settings
    _rpnConfigure();

    // Websockets
    #if WEB_SUPPORT
        wsOnSendRegister(_rpnWebSocketOnSend);
        wsOnReceiveRegister(_rpnWebSocketOnReceive);
    #endif

    brokerRegister(_rpnBrokerCallback);
    espurnaRegisterReload(_rpnConfigure);
    espurnaRegisterLoop(_rpnLoop);

}

#endif
