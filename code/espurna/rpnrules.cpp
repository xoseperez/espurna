/*

RPN RULES MODULE
Use RPNLib library (https://github.com/xoseperez/rpnlib)
Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "rpnrules.h"

#if RPN_RULES_SUPPORT

#include "broker.h"
#include "mqtt.h"
#include "ntp.h"
#include "relay.h"
#include "terminal.h"
#include "ws.h"

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

    root["rpnSticky"] = getSetting("rpnSticky", 1 == RPN_STICKY);
    root["rpnDelay"] = getSetting("rpnDelay", RPN_DELAY);
    JsonArray& rules = root.createNestedArray("rpnRules");

    unsigned char i = 0;
    String rule = getSetting({"rpnRule", i});
    while (rule.length()) {
        rules.add(rule);
        rule = getSetting({"rpnRule", ++i});
    }

    #if MQTT_SUPPORT
        i=0;
        JsonArray& topics = root.createNestedArray("rpnTopics");
        JsonArray& names = root.createNestedArray("rpnNames");
        String rpn_topic = getSetting({"rpnTopic", i});
        while (rpn_topic.length() > 0) {
            String rpn_name = getSetting({"rpnName", i});
            topics.add(rpn_topic);
            names.add(rpn_name);
            rpn_topic = getSetting({"rpnTopic", ++i});
        }
    #endif

}

#if MQTT_SUPPORT

void _rpnMQTTSubscribe() {
    unsigned char i = 0;
    String rpn_topic = getSetting({"rpnTopic", i});
    while (rpn_topic.length()) {
        mqttSubscribeRaw(rpn_topic.c_str());
        rpn_topic = getSetting({"rpnTopic", ++i});
    }
}

void _rpnMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        _rpnMQTTSubscribe();
    }

    if (type == MQTT_MESSAGE_EVENT) {
        unsigned char i = 0;
        String rpn_topic = getSetting({"rpnTopic", i});
        while (rpn_topic.length()) {
            if (rpn_topic.equals(topic)) {
                String rpn_name = getSetting({"rpnName", i});
                if (rpn_name.length()) {
                    rpn_variable_set(_rpn_ctxt, rpn_name.c_str(), atof(payload));
                    _rpn_last = millis();
                    _rpn_run = true;
                    break;
                }
            }
            rpn_topic = getSetting({"rpnTopic", ++i});
        }
    }

}
#endif // MQTT_SUPPORT

void _rpnConfigure() {
    #if MQTT_SUPPORT
        if (mqttConnected()) _rpnMQTTSubscribe();
    #endif
    _rpn_delay = getSetting("rpnDelay", RPN_DELAY);
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

#if NTP_SUPPORT

bool _rpnNtpNow(rpn_context & ctxt) {
    if (!ntpSynced()) return false;
    rpn_stack_push(ctxt, now());
    return true;
}

bool _rpnNtpFunc(rpn_context & ctxt, int (*func)(time_t)) {
    float timestamp;
    rpn_stack_pop(ctxt, timestamp);
    rpn_stack_push(ctxt, func(time_t(timestamp)));
    return true;
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


void _rpnInit() {

    // Init context
    rpn_init(_rpn_ctxt);

    // Time functions need NTP support
    // TODO: since 1.14.2, timelib+ntpclientlib are no longer used with latest Cores
    //       `now` is always in UTC, `utc_...` functions to be used instead to convert time
    #if NTP_SUPPORT && !NTP_LEGACY_SUPPORT
        rpn_operator_set(_rpn_ctxt, "utc", 0, _rpnNtpNow);
        rpn_operator_set(_rpn_ctxt, "now", 0, _rpnNtpNow);

        rpn_operator_set(_rpn_ctxt, "utc_month", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, utc_month);
        });
        rpn_operator_set(_rpn_ctxt, "month", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, month);
        });

        rpn_operator_set(_rpn_ctxt, "utc_day", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, utc_day);
        });
        rpn_operator_set(_rpn_ctxt, "day", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, day);
        });

        rpn_operator_set(_rpn_ctxt, "utc_dow", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, utc_weekday);
        });
        rpn_operator_set(_rpn_ctxt, "dow", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, weekday);
        });

        rpn_operator_set(_rpn_ctxt, "utc_hour", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, utc_hour);
        });
        rpn_operator_set(_rpn_ctxt, "hour", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, hour);
        });

        rpn_operator_set(_rpn_ctxt, "utc_minute", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, utc_minute);
        });
        rpn_operator_set(_rpn_ctxt, "minute", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, minute);
        });
    #endif

    // TODO: 1.14.0 weekday(...) conversion seemed to have 0..6 range with Monday as 0
    //       using classic Sunday as first, but instead of 0 it is 1
    //       Implementation above also uses 1 for Sunday, staying compatible with TimeLib
    #if NTP_SUPPORT && NTP_LEGACY_SUPPORT
        rpn_operator_set(_rpn_ctxt, "utc", 0, [](rpn_context & ctxt) {
            if (!ntpSynced()) return false;
            rpn_stack_push(ctxt, ntpLocal2UTC(now()));
            return true;
        });
        rpn_operator_set(_rpn_ctxt, "now", 0, _rpnNtpNow);

        rpn_operator_set(_rpn_ctxt, "month", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, month);
        });
        rpn_operator_set(_rpn_ctxt, "day", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, day);
        });
        rpn_operator_set(_rpn_ctxt, "dow", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, weekday);
        });
        rpn_operator_set(_rpn_ctxt, "hour", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, hour);
        });
        rpn_operator_set(_rpn_ctxt, "minute", 1, [](rpn_context & ctxt) {
            return _rpnNtpFunc(ctxt, minute);
        });
    #endif

    // Dumps RPN stack contents
    rpn_operator_set(_rpn_ctxt, "debug", 0, [](rpn_context & ctxt) {
        _rpnDump();
        return true;
    });

    // Accept relay number and numeric API status value (0, 1 and 2)
    #if RELAY_SUPPORT

        rpn_operator_set(_rpn_ctxt, "relay", 2, [](rpn_context & ctxt) {
            float status, id;
            rpn_stack_pop(ctxt, id);
            rpn_stack_pop(ctxt, status);
            if (int(status) == 2) {
                relayToggle(int(id));
            } else {
                relayStatus(int(id), int(status) == 1);
            }
            return true;
        });

    #endif // RELAY_SUPPORT == 1

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
            float value, id;
            rpn_stack_pop(ctxt, id);
            rpn_stack_pop(ctxt, value);
            lightChannel(int(id), int(value));
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

void _rpnRun() {

    unsigned char i = 0;
    String rule = getSetting({"rpnRule", i});
    while (rule.length()) {
        //DEBUG_MSG_P(PSTR("[RPN] Running \"%s\"\n"), rule.c_str());
        rpn_process(_rpn_ctxt, rule.c_str(), true);
        //_rpnDump();
        rule = getSetting({"rpnRule", ++i});
        rpn_stack_clear(_rpn_ctxt);
    }

    if (!getSetting("rpnSticky", 1 == RPN_STICKY)) {
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

    #if NTP_SUPPORT
        NtpBroker::Register([](const NtpTick tick, time_t timestamp, const String& datetime) {
            static const String tick_every_hour(F("tick1h"));
            static const String tick_every_minute(F("tick1m"));

            const char* ptr = 
                (tick == NtpTick::EveryMinute) ? tick_every_minute.c_str() :
                (tick == NtpTick::EveryHour) ? tick_every_hour.c_str() : nullptr;

            if (ptr != nullptr) {
                rpn_variable_set(_rpn_ctxt, ptr, timestamp);
                _rpn_last = millis();
                _rpn_run = true;
            }
        });
    #endif

    StatusBroker::Register(_rpnBrokerStatus);
    SensorReadBroker::Register(_rpnBrokerCallback);

    espurnaRegisterReload(_rpnConfigure);
    espurnaRegisterLoop(_rpnLoop);

}

#endif // RPN_RULES_SUPPORT
