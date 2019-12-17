/*

RPN RULES MODULE
Use RPNLib library (https://github.com/xoseperez/rpnlib)
Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if RPN_RULES_SUPPORT

#include "relay.h"

#include <rpnlib.h>

// -----------------------------------------------------------------------------
// Custom commands
// -----------------------------------------------------------------------------

rpn_context _rpn_ctxt;
bool _rpn_run = false;
unsigned long _rpn_delay = RPN_DELAY;
unsigned long _rpn_last = 0;

// -----------------------------------------------------------------------------

bool _rpnWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    return (strncmp(key, "rpn", 3) == 0);
}

void _rpnWebSocketOnConnected(JsonObject& root) {

    root["rpnSticky"] = getSetting("rpnSticky", 1).toInt();
    root["rpnDelay"] = getSetting("rpnDelay", RPN_DELAY).toInt();
    JsonArray& rules = root.createNestedArray("rpnRules");

    unsigned char i = 0;
    String rule = getSetting("rpnRule", i, "");
    while (rule.length()) {
        rules.add(rule);
        rule = getSetting("rpnRule", ++i, "");
    }

    #if MQTT_SUPPORT
        i=0;
        JsonArray& topics = root.createNestedArray("rpnTopics");
        JsonArray& names = root.createNestedArray("rpnNames");
        String rpn_topic = getSetting("rpnTopic", i, "");
        while (rpn_topic.length() > 0) {
            String rpn_name = getSetting("rpnName", i, "");
            topics.add(rpn_topic);
            names.add(rpn_name);
            rpn_topic = getSetting("rpnTopic", ++i, "");
        }
    #endif

}

#if MQTT_SUPPORT

void _rpnMQTTSubscribe() {
    unsigned char i = 0;
    String rpn_topic = getSetting("rpnTopic", i, "");
    while (rpn_topic.length()) {
        mqttSubscribeRaw(rpn_topic.c_str());
        rpn_topic = getSetting("rpnTopic", ++i, "");
    }
}

void _rpnMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        _rpnMQTTSubscribe();
    }

    if (type == MQTT_MESSAGE_EVENT) {
        unsigned char i = 0;
        String rpn_topic = getSetting("rpnTopic", i, "");
        while (rpn_topic.length()) {
            if (rpn_topic.equals(topic)) {
                String rpn_name = getSetting("rpnName", i, "");
                if (rpn_name.length()) {
                    rpn_variable_set(_rpn_ctxt, rpn_name.c_str(), atof(payload));
                    _rpn_last = millis();
                    _rpn_run = true;
                    break;
                }
            }
            rpn_topic = getSetting("rpnTopic", ++i, "");
        }
    }

}
#endif // MQTT_SUPPORT

void _rpnConfigure() {
    #if MQTT_SUPPORT
        if (mqttConnected()) _rpnMQTTSubscribe();
    #endif
    _rpn_delay = getSetting("rpnDelay", RPN_DELAY).toInt();
}

void _rpnBrokerCallback(const String& topic, unsigned char id, double value, const char*) {

    char name[32] = {0};

    snprintf(name, sizeof(name), "%s%u", topic.c_str(), id);
    rpn_variable_set(_rpn_ctxt, name, value);

    _rpn_last = millis();
    _rpn_run = true;

}

void _rpnBrokerStatus(const String& topic, unsigned char id, unsigned int value) {
    _rpnBrokerCallback(topic, id, double(value), nullptr);
}

void _rpnInit() {

    // Init context
    rpn_init(_rpn_ctxt);

    // Time functions
    rpn_operator_set(_rpn_ctxt, "now", 0, [](rpn_context & ctxt) {
        if (!ntpSynced()) return false;
        rpn_stack_push(ctxt, now());
        return true;
    });
    rpn_operator_set(_rpn_ctxt, "utc", 0, [](rpn_context & ctxt) {
        if (!ntpSynced()) return false;
        rpn_stack_push(ctxt, ntpLocal2UTC(now()));
        return true;
    });
    rpn_operator_set(_rpn_ctxt, "dow", 1, [](rpn_context & ctxt) {
        float a;
        rpn_stack_pop(ctxt, a);
        unsigned char dow = (weekday(int(a)) + 5) % 7;
        rpn_stack_push(ctxt, dow);
        return true;
    });
    rpn_operator_set(_rpn_ctxt, "hour", 1, [](rpn_context & ctxt) {
        float a;
        rpn_stack_pop(ctxt, a);
        rpn_stack_push(ctxt, hour(int(a)));
        return true;
    });
    rpn_operator_set(_rpn_ctxt, "minute", 1, [](rpn_context & ctxt) {
        float a;
        rpn_stack_pop(ctxt, a);
        rpn_stack_push(ctxt, minute(int(a)));
        return true;
    });

    // Debug
    rpn_operator_set(_rpn_ctxt, "debug", 0, [](rpn_context & ctxt) {
        _rpnDump();
        return true;
    });

    // Relay operators
    rpn_operator_set(_rpn_ctxt, "relay", 2, [](rpn_context & ctxt) {
        float a, b;
        rpn_stack_pop(ctxt, b); // relay number
        rpn_stack_pop(ctxt, a); // new status
        if (int(a) == 2) {
            relayToggle(int(b));
        } else {
            relayStatus(int(b), int(a) == 1);
        }
        return true;
    });

    // Channel operators
    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT

        rpn_operator_set(_rpn_ctxt, "update", 0, [](rpn_context & ctxt) {
            lightUpdate(true, true);
            return true;
        });

        rpn_operator_set(_rpn_ctxt, "black", 0, [](rpn_context & ctxt) {
            lightColor((unsigned long) 0);
            return true;
        });

        rpn_operator_set(_rpn_ctxt, "channel", 2, [](rpn_context & ctxt) {
            float a, b;
            rpn_stack_pop(ctxt, b); // channel number
            rpn_stack_pop(ctxt, a); // new value
            lightChannel(int(b), int(a));
            return true;
        });

    #endif

}

#if TERMINAL_SUPPORT

void _rpnInitCommands() {

    terminalRegisterCommand(F("RPN.VARS"), [](Embedis* e) {
        unsigned char num = rpn_variables_size(_rpn_ctxt);
        if (0 == num) {
            DEBUG_MSG_P(PSTR("[RPN] No variables\n"));
        } else {
            DEBUG_MSG_P(PSTR("[RPN] Variables:\n"));
            for (unsigned char i=0; i<num; i++) {
                char * name = rpn_variable_name(_rpn_ctxt, i);
                float value;
                rpn_variable_get(_rpn_ctxt, name, value);
                DEBUG_MSG_P(PSTR("   %s: %s\n"), name, String(value).c_str());
            }
        }
        terminalOK();
    });

    terminalRegisterCommand(F("RPN.OPS"), [](Embedis* e) {
        unsigned char num = _rpn_ctxt.operators.size();
        DEBUG_MSG_P(PSTR("[RPN] Operators:\n"));
        for (unsigned char i=0; i<num; i++) {
            DEBUG_MSG_P(PSTR("   %s (%d)\n"), _rpn_ctxt.operators[i].name, _rpn_ctxt.operators[i].argc);
        }
        terminalOK();
    });

    terminalRegisterCommand(F("RPN.TEST"), [](Embedis* e) {
        if (e->argc == 2) {
            DEBUG_MSG_P(PSTR("[RPN] Running \"%s\"\n"), e->argv[1]);
            rpn_process(_rpn_ctxt, e->argv[1], true);
            _rpnDump();
            rpn_stack_clear(_rpn_ctxt);
            terminalOK();
        } else {
            terminalError(F("Wrong arguments"));
        }
    });

}
#endif

void _rpnDump() {
    float value;
    DEBUG_MSG_P(PSTR("[RPN] Stack:\n"));
    unsigned char num = rpn_stack_size(_rpn_ctxt);
    if (0 == num) {
        DEBUG_MSG_P(PSTR("      (empty)\n"));
    } else {
        unsigned char index = num - 1;
        while (rpn_stack_get(_rpn_ctxt, index, value)) {
            DEBUG_MSG_P(PSTR("      %02d: %s\n"), index--, String(value).c_str());
        }
    }
}

void _rpnRun() {

    unsigned char i = 0;
    String rule = getSetting("rpnRule", i, "");
    while (rule.length()) {
        //DEBUG_MSG_P(PSTR("[RPN] Running \"%s\"\n"), rule.c_str());
        rpn_process(_rpn_ctxt, rule.c_str(), true);
        //_rpnDump();
        rule = getSetting("rpnRule", ++i, "");
        rpn_stack_clear(_rpn_ctxt);
    }

    if (getSetting("rpnSticky", 1).toInt() == 0) {
        rpn_variables_clear(_rpn_ctxt);
    }

}

void _rpnLoop() {

    if (_rpn_run && (millis() - _rpn_last > _rpn_delay)) {
        _rpnRun();
        _rpn_run = false;
    }

}

void rpnSetup() {

    // Init context
    _rpnInit();

    // Load & cache settings
    _rpnConfigure();

    // Terminal commands
    #if TERMINAL_SUPPORT
        _rpnInitCommands();
    #endif

    // Websockets
    #if WEB_SUPPORT
        wsRegister()
            .onVisible([](JsonObject& root) { root["rpnVisible"] = 1; })
            .onConnected(_rpnWebSocketOnConnected)
            .onKeyCheck(_rpnWebSocketOnKeyCheck);
    #endif

    // MQTT
    #if MQTT_SUPPORT
        mqttRegister(_rpnMQTTCallback);
    #endif

    StatusBroker::Register(_rpnBrokerStatus);
    SensorReadBroker::Register(_rpnBrokerCallback);

    espurnaRegisterReload(_rpnConfigure);
    espurnaRegisterLoop(_rpnLoop);

}

#endif // RPN_RULES_SUPPORT
