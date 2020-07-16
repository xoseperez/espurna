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
#include "rpc.h"
#include "sensor.h"
#include "rfbridge.h"
#include "terminal.h"
#include "ws.h"

#include <vector>
#include <Ticker.h>

// -----------------------------------------------------------------------------
// Custom commands
// -----------------------------------------------------------------------------

rpn_context _rpn_ctxt;
bool _rpn_run = false;
unsigned long _rpn_delay = RPN_DELAY;
unsigned long _rpn_last = 0;

struct RpnRunner {
    uint32_t every;
    uint32_t last;
    bool expired;
};

std::vector<RpnRunner> _rpn_runners;


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
                    rpn_value value { atof(payload) };
                    rpn_variable_set(_rpn_ctxt, rpn_name, value);
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

    if (topic == MQTT_TOPIC_RELAY) {
        rpn_variable_set(_rpn_ctxt, name, rpn_value(static_cast<bool>(value)));
    } else {
        rpn_variable_set(_rpn_ctxt, name, rpn_value(value));
    }

    _rpn_run = true;

}

void _rpnBrokerStatus(const String& topic, unsigned char id, unsigned int value) {
    _rpnBrokerCallback(topic, id, double(value), nullptr);
}

#if NTP_SUPPORT

rpn_error _rpnNtpNow(rpn_context & ctxt) {
    if (!ntpSynced()) return rpn_operator_error::CannotContinue;
    rpn_value ts { static_cast<rpn_int>(now()) };
    rpn_stack_push(ctxt, ts);
    return 0;
}

rpn_error _rpnNtpFunc(rpn_context & ctxt, rpn_int (*func)(time_t)) {
    rpn_value value;
    rpn_stack_pop(ctxt, value);

    value = rpn_value(func(value.toInt()));
    rpn_stack_push(ctxt, value);

    return 0;
}

#endif // NTP_SUPPORT

String _rpnValueToString(const rpn_value& value) {
    String out;
    if (value.isString()) {
        out = value.toString();
    } else if (value.isFloat()) {
        out = String(value.toFloat(), 10);
    } else if (value.isInt()) {
        out = String(value.toInt(), 10);
    } else if (value.isUint()) {
        out = String(value.toUint(), 10);
    } else if (value.isBoolean()) {
        out = String(value.toBoolean() ? "true" : "false");
    } else if (value.isNull()) {
        out = F("(null)");
    }
    return out;
}

char _rpnStackTypeTag(rpn_stack_value::Type type) {
    switch (type) {
    case rpn_stack_value::Type::None:
        return 'N';
    case rpn_stack_value::Type::Variable:
        return '$';
    case rpn_stack_value::Type::Array:
        return 'A';
    case rpn_stack_value::Type::Value:
    default:
        return ' ';
    }
}

#if RELAY_SUPPORT

rpn_error _rpnRelayStatus(rpn_context & ctxt, bool force) {
    rpn_value id;
    rpn_value status;

    rpn_stack_pop(ctxt, id);
    rpn_stack_pop(ctxt, status);

    rpn_uint value = status.toUint();
    if (value == 2) {
        relayToggle(id.toUint());
    } else if (relayStatusTarget(id.toUint()) != (value == 1)) {
        relayStatus(id.toUint(), value == 1);
    }

    return 0;
}

#endif // RELAY_SUPPORT

#if RF_SUPPORT

struct rpn_rfbridge_code {
    String raw;
    uint32_t hits;
    decltype(millis()) last;
    decltype(millis()) triggered;
};

static std::list<rpn_rfbridge_code> _rfb_codes;

rpn_error _rpnRfbSequence(rpn_context& ctxt) {
    rpn_value second;
    rpn_stack_pop(ctxt, second);

    rpn_value first;
    rpn_stack_pop(ctxt, first);

    String raw[2] {first.toString(), second.toString()};
    rpn_rfbridge_code* refs[2] {nullptr, nullptr};

    for (auto& recent : _rfb_codes) {
        for (int index = 0; index < 2; ++index) {
            refs[index] = (raw[index] == recent.raw) ? &recent : nullptr;
        }
    }

    if ((refs[0] == nullptr) || (refs[1] == nullptr)) {
        return rpn_operator_error::CannotContinue;
    }

    // purge codes to avoid matching again on the next rules run
    // 'triggered' would be kind of quirky here, since we don't limit ourselves with any window
    if ((millis() - refs[0]->last) > (millis() - refs[1]->last)) {
        _rfb_codes.remove_if([&refs](rpn_rfbridge_code& code) {
            return (refs[0] == &code) || (refs[1] == &code);
        });
        return rpn_operator_error::Ok;
    }

    return rpn_operator_error::CannotContinue;
}

rpn_error _rpnRfbMatcher(rpn_context& ctxt) {
    rpn_value code;
    rpn_stack_pop(ctxt, code);

    rpn_value hits;
    rpn_stack_pop(ctxt, hits);

    String raw_code = code.toString();
    auto result = std::find_if(_rfb_codes.begin(), _rfb_codes.end(), [&raw_code](const rpn_rfbridge_code& code) {
        return code.raw == raw_code;
    });

    if (result == _rfb_codes.end()) {
        return rpn_operator_error::CannotContinue;
    }

    // only process recent codes, ignore when rule is processing outside of this small window
    if (millis() - (*result).last >= 2000) {
        return rpn_operator_error::CannotContinue;
    }

    // hits == 1 is a single click, hits == 5 is long click
    // we sort-of can distinguish single and double via timestamp, but it has a **very** unreliable timing
    if ((*result).hits > hits.toUint()) {
        return rpn_operator_error::CannotContinue;
    }

    // avoid re-triggering immediatly based on something running rules engine again
    if ((*result).triggered && (millis() - (*result).triggered <= 1000)) {
        return rpn_operator_error::CannotContinue;
    }
    (*result).triggered = millis();

    return rpn_operator_error::Ok;
}

void _rpnBrokerRfbridgeCallback(const char* raw_code) {

#if RFB_DIRECT
    raw_code = (raw_code + strlen(raw_code) - 8);
#else
    raw_code = (raw_code + strlen(raw_code) - 6);
#endif

    // expire really old codes to avoid memory exhaustion
    auto ts = millis();
    auto old = std::remove_if(_rfb_codes.begin(), _rfb_codes.end(), [ts](rpn_rfbridge_code& code) {
        return (ts - code.last) >= 10000u;
    });

    if (old != _rfb_codes.end()) {
        _rfb_codes.erase(old, _rfb_codes.end());
    }

    auto result = std::find_if(_rfb_codes.begin(), _rfb_codes.end(), [raw_code](rpn_rfbridge_code& code) {
        return code.raw == raw_code;
    });

    if (result != _rfb_codes.end()) {
        // we also need to reset hits at a certain point to allow repeats to go through
        // number is arbitrary time, just about 3-4 times more than it takes for a code to repeat when holding
        if (millis() - (*result).last >= 2000u) {
            (*result).hits = 0;
        }
        (*result).last = millis();
        (*result).hits += 1u;
        DEBUG_MSG_P(PSTR("[RPN] refresh code=%s hits=%u last=%u\n"), raw_code, (*result).hits, (*result).last);
    } else {
        DEBUG_MSG_P(PSTR("[RPN] new code: %s\n"), raw_code);
        _rfb_codes.push_back({raw_code, 1u, millis(), 0u});
    }

    _rpn_run = true;
}

#endif // RF_SUPPORT

void _rpnDump() {
    DEBUG_MSG_P(PSTR("[RPN] Stack:\n"));

    auto index = rpn_stack_size(_rpn_ctxt);
    if (!index) {
        DEBUG_MSG_P(PSTR("      (empty)\n"));
        return;
    }

    rpn_stack_foreach(_rpn_ctxt, [&index](rpn_stack_value::Type type, const rpn_value& value) {
        DEBUG_MSG_P(PSTR("%c      %02u: %s\n"),
            _rpnStackTypeTag(type), index--,
            _rpnValueToString(value).c_str()
        );
    });
}

void _rpnInit() {

    // Init context
    rpn_init(_rpn_ctxt);

    // Time functions need NTP support
    // TODO: since 1.15.0, timelib+ntpclientlib are no longer used with latest Cores
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
        rpn_operator_set(_rpn_ctxt, "utc", 0, [](rpn_context & ctxt) -> rpn_error {
            if (!ntpSynced()) return rpn_operator_error::CannotContinue;
            rpn_value ts { static_cast<rpn_int>(ntpLocal2UTC(now())) };
            rpn_stack_push(ctxt, ts);
            return 0;
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

    // Accept relay number and numeric API status value (0, 1 and 2)
    #if RELAY_SUPPORT

        // apply status and reset timers when called
        rpn_operator_set(_rpn_ctxt, "relay_reset", 2, [](rpn_context & ctxt) {
            return _rpnRelayStatus(ctxt, true);
        });

        // only update status when target status differs, keep running timers
        rpn_operator_set(_rpn_ctxt, "relay", 2, [](rpn_context & ctxt) {
            return _rpnRelayStatus(ctxt, false);
        });

    #endif // RELAY_SUPPORT == 1

    // Channel operators
    #if RELAY_PROVIDER == RELAY_PROVIDER_LIGHT

        rpn_operator_set(_rpn_ctxt, "update", 0, [](rpn_context & ctxt) -> rpn_error {
            lightUpdate(true, true);
            return 0;
        });

        rpn_operator_set(_rpn_ctxt, "black", 0, [](rpn_context & ctxt) -> rpn_error {
            lightColor((unsigned long) 0);
            return 0;
        });

        rpn_operator_set(_rpn_ctxt, "channel", 2, [](rpn_context & ctxt) -> rpn_error {
            rpn_value value;
            rpn_value id;
            rpn_stack_pop(ctxt, id);
            rpn_stack_pop(ctxt, value);
            lightChannel(id.toUint(), id.toInt());
            return 0;
        });

    #endif

    #if RF_SUPPORT
        rpn_operator_set(_rpn_ctxt, "rfb_match", 2, _rpnRfbMatcher);
        rpn_operator_set(_rpn_ctxt, "rfb_sequence", 2, _rpnRfbSequence);
    #endif

    #if MQTT_SUPPORT
        rpn_operator_set(_rpn_ctxt, "mqtt_send", 2, [](rpn_context & ctxt) -> rpn_error {
            rpn_value message;
            rpn_stack_pop(ctxt, message);

            rpn_value topic;
            rpn_stack_pop(ctxt, topic);

            return mqttSendRaw(topic.toString().c_str(), message.toString().c_str())
                ? rpn_operator_error::Ok
                : rpn_operator_error::CannotContinue;
        });
    #endif

    // Some debugging. Dump stack contents
    rpn_operator_set(_rpn_ctxt, "debug", 0, [](rpn_context & ctxt) -> rpn_error {
        _rpnDump();
        return 0;
    });

    // And, simple string logging
    #if DEBUG_SUPPORT
        rpn_operator_set(_rpn_ctxt, "log", 1, [](rpn_context & ctxt) -> rpn_error {
            rpn_value message;
            rpn_stack_pop(ctxt, message);

            DEBUG_MSG_P(PSTR("[RPN] %s\n"), message.toString().c_str());

            return 0;
        });
    #endif

    rpn_operator_set(_rpn_ctxt, "millis", 0, [](rpn_context & ctxt) -> rpn_error {
        rpn_stack_push(ctxt, rpn_value(static_cast<uint32_t>(millis())));
        return 0;
    });

    rpn_operator_set(_rpn_ctxt, "run_every_ms", 1, [](rpn_context & ctxt) -> rpn_error {
        rpn_value every_;
        rpn_stack_pop(ctxt, every_);

        auto every = every_.toUint();
        for (auto& runner : _rpn_runners) {
            if (every == runner.every) {
                return runner.expired
                    ? rpn_operator_error::Ok
                    : rpn_operator_error::CannotContinue;
            }
        }

        _rpn_runners.push_back({every, millis(), false});
        return rpn_operator_error::CannotContinue;
    });

}

#if TERMINAL_SUPPORT

void _rpnInitCommands() {

    terminalRegisterCommand(F("RPN.RUNNERS"), [](const terminal::CommandContext&) {
        for (auto& runner : _rpn_runners) {
            DEBUG_MSG_P(PSTR("[RPN] %p Every %u ms\n"), &runner, runner.every);
        }
    });

    terminalRegisterCommand(F("RPN.VARS"), [](const terminal::CommandContext&) {
        DEBUG_MSG_P(PSTR("[RPN] Variables:\n"));
        rpn_variables_foreach(_rpn_ctxt, [](const String& name, const rpn_value& value) {
            DEBUG_MSG_P(PSTR("   %s: %s\n"), name.c_str(), _rpnValueToString(value).c_str());
        });
        terminalOK();
    });

    terminalRegisterCommand(F("RPN.OPS"), [](const terminal::CommandContext&) {
        DEBUG_MSG_P(PSTR("[RPN] Operators:\n"));
        rpn_operators_foreach(_rpn_ctxt, [](const String& name, size_t argc, rpn_operator_callback_f ptr) {
            DEBUG_MSG_P(PSTR("   %s (%d) -> %p\n"), name.c_str(), argc, ptr);
        });
        terminalOK();
    });

    terminalRegisterCommand(F("RPN.TEST"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc == 2) {
            DEBUG_MSG_P(PSTR("[RPN] Running \"%s\"\n"), ctx.argv[1].c_str());
            rpn_process(_rpn_ctxt, ctx.argv[1].c_str());
            _rpnDump();
            rpn_stack_clear(_rpn_ctxt);
            terminalOK();
        } else {
            terminalError(F("Wrong arguments"));
        }
    });

}
#endif

// enables us to use rules without any events firing
// notice: requires rpnRun to trigger at least once so that we can install runners
void _rpnRunnersCheck() {
    auto ts = millis();
    for (auto& runner : _rpn_runners) {
        if (ts - runner.last >= runner.every) {
            runner.expired = true;
            runner.last = ts;
            _rpn_run = true;
        }
    }
}

void _rpnRunnersReset() {
    for (auto& runner : _rpn_runners) {
        runner.expired = false;
    }
}

void _rpnRun() {

    if (!_rpn_run) {
        return;
    }

    if (millis() - _rpn_last <= _rpn_delay) {
        return;
    }

    _rpn_last = millis();
    _rpn_run = false;

    unsigned char i = 0;
    String rule = getSetting({"rpnRule", i});
    while (rule.length()) {
        //DEBUG_MSG_P(PSTR("[RPN] Running \"%s\"\n"), rule.c_str());
        rpn_process(_rpn_ctxt, rule.c_str());
        //_rpnDump();
        rule = getSetting({"rpnRule", ++i});
        rpn_stack_clear(_rpn_ctxt);
    }

    if (!getSetting("rpnSticky", 1 == RPN_STICKY)) {
        rpn_variables_clear(_rpn_ctxt);
    }

}

void _rpnLoop() {

    _rpnRunnersCheck();
    _rpnRun();
    _rpnRunnersReset();

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
                rpn_value value { static_cast<rpn_int>(timestamp) };
                rpn_variable_set(_rpn_ctxt, ptr, value);
                _rpn_run = true;
            }
        });
    #endif

    StatusBroker::Register(_rpnBrokerStatus);

    #if RF_SUPPORT
        RfbridgeBroker::Register(_rpnBrokerRfbridgeCallback);
    #endif

    #if SENSOR_SUPPORT
        SensorReadBroker::Register(_rpnBrokerCallback);
    #endif

    espurnaRegisterReload(_rpnConfigure);
    espurnaRegisterLoop(_rpnLoop);

    _rpn_last = millis();
    _rpn_run = true;

}

#endif // RPN_RULES_SUPPORT
