/*

RPN RULES MODULE
Use RPNLib library (https://github.com/xoseperez/rpnlib)
Copyright (C) 2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if RPN_RULES_SUPPORT

#include <rpnlib.h>

#include "rpnrules.h"

#include "light.h"
#include "mqtt.h"
#include "ntp.h"
#include "ntp_timelib.h"
#include "relay.h"
#include "rfbridge.h"
#include "rpc.h"
#include "rtcmem.h"
#include "sensor.h"
#include "terminal.h"
#include "wifi.h"
#include "ws.h"

#include <forward_list>
#include <list>
#include <type_traits>
#include <vector>

// -----------------------------------------------------------------------------

namespace espurna {
namespace rpnrules {
namespace {

struct Runner {
    enum class Policy {
        OneShot,
        Periodic
    };

    Runner() = default;
    Runner(Policy policy, unsigned long period) :
        _policy(policy),
        _period(period),
        _last(millis())
    {}

    Policy policy() const {
        return _policy;
    }

    unsigned long period() const {
        return _period;
    }

    unsigned long last() const {
        return _last;
    }

    explicit operator bool() const {
        return _expired;
    }

    bool match(Policy policy, unsigned long period) const {
        return (policy == _policy) && (period == _period);
    }

    bool expired(unsigned long timestamp) {
        if ((timestamp - _last) >= _period) {
            _expired = true;
            _last = timestamp;
            return true;
        }

        return false;
    }

    void reset() {
        _expired = false;
    }

private:
    Policy _policy { Policy::Periodic };

    uint32_t _period { 0ul };
    uint32_t _last { 0ul };

    bool _expired { false };
};

// -----------------------------------------------------------------------------

namespace build {

constexpr bool sticky() {
    return 1 == RPN_STICKY;
}

constexpr unsigned long delay() {
    return RPN_DELAY;
}

} // namespace build

namespace settings {
namespace keys {

PROGMEM_STRING(Sticky, "rpnSticky");
PROGMEM_STRING(Delay, "rpnDelay");
PROGMEM_STRING(Rule, "rpnRule");

PROGMEM_STRING(Topic, "rpnTopic");
PROGMEM_STRING(Name, "rpnName");

} // namespace keys

bool sticky() {
    return getSetting(keys::Sticky, build::sticky());
}

unsigned long delay() {
    return getSetting(keys::Delay, build::delay());
}

String rule(size_t index) {
    return getSetting({keys::Rule, index});
}

String topic(size_t index) {
    return getSetting({keys::Topic, index});
}

String name(size_t index) {
    return getSetting({keys::Name, index});
}

} // namespace settings

namespace internal {

rpn_context context;
bool run = false;
unsigned long run_delay = 0;
unsigned long run_last = 0;

using Runners = std::forward_list<Runner>;
Runners runners;

} // namespace internal

void schedule() {
    internal::run = true;
}

bool scheduled() {
    return internal::run;
}

void reset(bool next) {
    internal::run_last = millis();
    internal::run = next;
}

void reset() {
    reset(false);
}

bool due() {
    if (scheduled()) {
        auto timestamp = millis();
        if (timestamp - internal::run_last > internal::run_delay) {
            reset();
            return true;
        }
    }

    return false;
}

// enables us to use rules without any events firing, simply by having an internal timer scheduling the loop
// *MUST* run rules loop at least once (at boot, via external event, etc.), so the runners code is executed

struct RunnersHandler {
    RunnersHandler() = delete;
    RunnersHandler(const RunnersHandler&) = delete;
    RunnersHandler& operator=(const RunnersHandler&) = delete;

    RunnersHandler(RunnersHandler&&) = default;
    RunnersHandler& operator=(RunnersHandler&&) = delete;

    explicit RunnersHandler(internal::Runners& runners) :
        _runners(runners)
    {
        auto ts = millis();
        for (auto& runner : runners) {
            if (runner.expired(ts)) {
                schedule();
            }
        }
    }

    ~RunnersHandler() {
        _runners.remove_if([](const Runner& runner) {
            return (Runner::Policy::OneShot == runner.policy()) && static_cast<bool>(runner);
        });

        for (auto& runner : _runners) {
            runner.reset();
        }
    }

private:
    internal::Runners& _runners;
};

// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT
namespace terminal {

String valueToString(const rpn_value& value) {
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

char stackTypeTag(rpn_stack_value::Type type) {
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

void showStack(Print& output) {
    output.print(F("Stack:\n"));

    auto index = rpn_stack_size(internal::context);
    if (index) {
        rpn_stack_foreach(internal::context, [&](rpn_stack_value::Type type, const rpn_value& value) {
            output.printf_P(PSTR("%c      %02u: %s\n"),
                stackTypeTag(type), index--,
                valueToString(value).c_str());
        });
        return;
    }

    output.print(F("      (empty)\n"));
}

PROGMEM_STRING(Runners, "RPN.RUNNERS");

void runners(::terminal::CommandContext&& ctx) {
    if (internal::runners.empty()) {
        terminalError(ctx, F("No active runners"));
        return;
    }

    for (auto& runner : internal::runners) {
        char buffer[128] = {0};
        snprintf_P(buffer, sizeof(buffer),
            PSTR("%p %s %u ms, last %u ms\n"),
            &runner,
            (Runner::Policy::Periodic == runner.policy())
                ? "every"
                : "one-shot",
            runner.period(), runner.last());
        ctx.output.print(buffer);
    }

    terminalOK(ctx);
}

PROGMEM_STRING(Variables, "RPN.VARS");

void variables(::terminal::CommandContext&& ctx) {
    rpn_variables_foreach(internal::context,
        [&ctx](const String& name, const rpn_value& value) {
            char buffer[256] = {0};
            snprintf_P(buffer, sizeof(buffer),
                PSTR("      %s: %s\n"),
                name.c_str(), valueToString(value).c_str());
            ctx.output.print(buffer);
        });
    terminalOK(ctx);
}

PROGMEM_STRING(Operators, "RPN.OPS");

void operators(::terminal::CommandContext&& ctx) {
    rpn_operators_foreach(internal::context,
        [&ctx](const String& name, size_t argc, rpn_operator::callback_type) {
            char buffer[128] = {0};
            snprintf_P(buffer, sizeof(buffer),
                PSTR("      %s (%d)\n"),
                name.c_str(), argc);
            ctx.output.print(buffer);
        });
    terminalOK(ctx);
}

PROGMEM_STRING(Test, "RPN.TEST");

void test(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 2) {
        terminalError(ctx, F("Wrong arguments"));
        return;
    }

    const char* ptr = ctx.argv[1].c_str();
    ctx.output.printf_P(PSTR("Expression: \"%s\"\n"), ctx.argv[1].c_str());

    if (!rpn_process(internal::context, ptr)) {
        rpn_stack_clear(internal::context);
        char buffer[64] = {0};
        snprintf_P(buffer, sizeof(buffer),
            PSTR("at %u (category %d code %d)"),
            internal::context.error.position,
            static_cast<int>(internal::context.error.category),
            internal::context.error.code);
        terminalError(ctx, buffer);
        return;
    }

    showStack(ctx.output);
    rpn_stack_clear(internal::context);

    terminalOK(ctx);
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Runners, runners},
    {Variables, variables},
    {Operators, operators},
    {Test, test},
};

void setup() {
    espurna::terminal::add(Commands);
}

} // namespace terminal
#endif // TERMINAL_SUPPORT

#if WEB_SUPPORT
namespace web {

#if MQTT_SUPPORT
static constexpr std::array<espurna::settings::query::IndexedSetting, 2> Settings PROGMEM {
    {{settings::keys::Name, settings::name},
     {settings::keys::Topic, settings::topic}}
};

size_t countMqttNames() {
    size_t index { 0 };
    for (;;) {
        auto name = espurna::settings::Key(settings::keys::Name, index);
        if (!espurna::settings::has(name.value())) {
            break;
        }

        ++index;
    }

    return index;
}
#endif

bool onKeyCheck(espurna::StringView key, const JsonVariant& value) {
    return espurna::settings::query::samePrefix(key, STRING_VIEW("rpn"));
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, PSTR("rpn"));
}

void onConnected(JsonObject& root) {
    root[FPSTR(settings::keys::Sticky)] = rpnrules::settings::sticky();
    root[FPSTR(settings::keys::Delay)] = rpnrules::settings::delay();

    JsonArray& rules = root.createNestedArray(F("rpnRules"));

    size_t index { 0 };
    String rule;
    for (;;) {
        rule = rpnrules::settings::rule(index++);
        if (!rule.length()) {
            break;
        }

        rules.add(rule);
    }

#if MQTT_SUPPORT
    espurna::web::ws::EnumerableConfig config{ root, STRING_VIEW("rpnTopics") };
    config(STRING_VIEW("topics"), countMqttNames(), Settings);
#endif
}

} // namespace web
#endif // WEB_SUPPORT

#if MQTT_SUPPORT
namespace mqtt {

struct Variable {
    String name;
    rpn_value value;
};

std::forward_list<Variable> variables;

void subscribe() {
    size_t index { 0 };
    String topic;

    for(;;) {
        topic = rpnrules::settings::topic(index++);
        if (!topic.length()) {
            break;
        }

        mqttSubscribeRaw(topic.c_str());
    }
}

rpn_value process_variable(espurna::StringView payload) {
    auto tmp = std::make_unique<rpn_context>();

    rpn_value out;
    if (!rpn_process(*tmp, payload.begin())) {
        return out;
    }

    if (rpn_stack_size(*tmp) != 1) {
        return out;
    }

    out = rpn_stack_pop(*tmp);
    return out;
}

void callback(unsigned int type, StringView topic, StringView payload) {
    if (type == MQTT_CONNECT_EVENT) {
        subscribe();
        return;
    }

    if (type == MQTT_MESSAGE_EVENT) {
        if (!payload.length()) {
            return;
        }

        if ((payload[0] == '&') || (payload[0] == '$')) {
            return;
        }

        size_t count { 0 };
        String rpnTopic;

        for (;;) {
            const auto index = count++;
            rpnTopic = rpnrules::settings::topic(index);
            if (!rpnTopic.length()) {
                break;
            }

            if (rpnTopic == topic) {
                const auto name = rpnrules::settings::name(index);
                if (!name.length()) {
                    break;
                }

                auto value = process_variable(payload);
                if (value.isNull() || value.isError()) {
                    return;
                }

                for (auto& variable : variables) {
                    if (variable.name == name) {
                        variable.value = std::move(value);
                        return;
                    }
                }

                variables.emplace_front(
                    Variable{
                        .name = std::move(name),
                        .value = std::move(value),
                    });

                return;
            }
        }
        return;
    }
}

void init(rpn_context& context) {
    mqttRegister(callback);

    rpn_operator_set(context, "mqtt_send", 2, [](rpn_context& ctxt) -> rpn_error {
        rpn_value message;
        rpn_stack_pop(ctxt, message);

        rpn_value topic;
        rpn_stack_pop(ctxt, topic);

        return ::mqttSendRaw(topic.toString().c_str(), message.toString().c_str())
                ? rpn_operator_error::Ok
                : rpn_operator_error::CannotContinue;
    });
}

} // namespace mqtt
#endif // MQTT_SUPPORT

namespace operators {
namespace runners {

rpn_operator_error handle(rpn_context& ctxt, Runner::Policy policy, unsigned long time) {
    for (auto& runner : internal::runners) {
        if (runner.match(policy, time)) {
            return static_cast<bool>(runner)
                ? rpn_operator_error::Ok
                : rpn_operator_error::CannotContinue;
        }
    }

    internal::runners.emplace_front(policy, time);
    return rpn_operator_error::CannotContinue;
}

void init(rpn_context& context) {
    rpn_operator_set(context, "oneshot_ms", 1, [](rpn_context& ctxt) -> rpn_error {
        auto every = rpn_stack_pop(ctxt);
        return handle(ctxt, Runner::Policy::OneShot, every.toUint());
    });

    rpn_operator_set(context, "every_ms", 1, [](rpn_context & ctxt) -> rpn_error {
        auto every = rpn_stack_pop(ctxt);
        return handle(ctxt, Runner::Policy::Periodic, every.toUint());
    });
}

} // namespace runners

#if NTP_SUPPORT

namespace ntp {

template <typename T, size_t Size>
using SplitType = std::integral_constant<bool, sizeof(T) == Size>;

using SplitTimestamp = SplitType<time_t, 8>;

static_assert((sizeof(time_t) == 4) || (sizeof(time_t) == 8), "");
constexpr size_t TimestampSize { SplitTimestamp{} ? 2 : 1 };

using TimestampFunc = rpn_int(*)(time_t);

rpn_error popTimestampPair(rpn_context& ctxt, TimestampFunc func) {
    rpn_value rhs = rpn_stack_pop(ctxt);
    rpn_value lhs = rpn_stack_pop(ctxt);

    auto timestamp = (static_cast<long long>(lhs.toInt()) << 32ll)
        | (static_cast<long long>(rhs.toInt()));

    rpn_value value(func(timestamp));
    rpn_stack_push(ctxt, value);

    return 0;
}

rpn_error popTimestampSingle(rpn_context& ctxt, TimestampFunc func) {
    rpn_value input = rpn_stack_pop(ctxt);
    rpn_value result(func(input.toInt()));
    rpn_stack_push(ctxt, result);
    return 0;
}

void pushTimestampPair(rpn_context& ctxt, time_t timestamp) {
    rpn_value lhs(static_cast<rpn_int>((static_cast<long long>(timestamp) >> 32ll) & 0xffffffffll));
    rpn_value rhs(static_cast<rpn_int>(static_cast<long long>(timestamp) & 0xffffffffll));

    rpn_stack_push(ctxt, lhs);
    rpn_stack_push(ctxt, rhs);
}

void pushTimestampSingle(rpn_context& ctxt, time_t timestamp) {
    rpn_value result(static_cast<rpn_int>(timestamp));
    rpn_stack_push(ctxt, result);
}

inline rpn_error popTimestamp(const std::true_type&, rpn_context& ctxt, TimestampFunc func) {
    return popTimestampPair(ctxt, func);
}

inline rpn_error popTimestamp(const std::false_type&, rpn_context& ctxt, TimestampFunc func) {
    return popTimestampSingle(ctxt, func);
}

rpn_error popTimestamp(rpn_context& ctxt, TimestampFunc func) {
    return popTimestamp(SplitTimestamp{}, ctxt, func);
}

inline void pushTimestamp(const std::true_type&, rpn_context& ctxt, time_t timestamp) {
    pushTimestampPair(ctxt, timestamp);
}

inline void pushTimestamp(const std::false_type&, rpn_context& ctxt, time_t timestamp) {
    pushTimestampSingle(ctxt, timestamp);
}

void pushTimestamp(rpn_context& ctxt, time_t timestamp) {
    pushTimestamp(SplitTimestamp{}, ctxt, timestamp);
}

rpn_error now(rpn_context & ctxt) {
    if (ntpSynced()) {
        pushTimestamp(ctxt, ::now());
        return 0;
    }

    return rpn_operator_error::CannotContinue;
}

rpn_error genericTimestampFunc(rpn_context & ctxt, TimestampFunc func) {
    return popTimestamp(ctxt, func);
}

namespace internal {

bool tick_minute { false };
bool tick_hour { false };

} // namespace

rpn_error tickMinute(rpn_context& ctxt) {
    if (internal::tick_minute) {
        internal::tick_minute = false;
        return 0;
    }

    return rpn_operator_error::CannotContinue;
}

rpn_error tickHour(rpn_context& ctxt) {
    if (internal::tick_hour) {
        internal::tick_hour = false;
        return 0;
    }

    return rpn_operator_error::CannotContinue;
}

#define registerGenericTimestampOperator(context, name, func)\
    rpn_operator_set(context, name, TimestampSize, [](rpn_context& ctxt) {\
        return genericTimestampFunc(ctxt, func);\
    })

void init(rpn_context& context) {
    ntpOnTick([](NtpTick tick) {
        switch (tick) {
        case NtpTick::EveryMinute:
            internal::tick_minute = true;
            break;
        case NtpTick::EveryHour:
            internal::tick_hour = true;
            break;
        }

        schedule();
    });

    rpn_operator_set(context, "tick_1h", 0, tickHour);
    rpn_operator_set(context, "tick_1m", 0, tickMinute);

    rpn_operator_set(context, "utc", 0, now);
    rpn_operator_set(context, "now", 0, now);

    registerGenericTimestampOperator(context, "utc_month", ::utc_month);
    registerGenericTimestampOperator(context, "month", ::month);

    registerGenericTimestampOperator(context, "utc_day", ::utc_day);
    registerGenericTimestampOperator(context, "day", ::day);

    registerGenericTimestampOperator(context, "utc_dow", ::utc_weekday);
    registerGenericTimestampOperator(context, "dow", ::weekday);

    registerGenericTimestampOperator(context, "utc_hour", ::utc_hour);
    registerGenericTimestampOperator(context, "hour", ::hour);

    registerGenericTimestampOperator(context, "utc_minute", ::utc_hour);
    registerGenericTimestampOperator(context, "minute", ::hour);
}

#undef registerGenericTimestampOperator

} // namespace ntp

#endif // NTP_SUPPORT

#if RELAY_SUPPORT

namespace relay {

void updateVariables(size_t id, bool status) {
    char name[32] = {0};
    snprintf(name, sizeof(name), "relay%zu", id);

    rpn_variable_set(internal::context, name, rpn_value(status));
    schedule();
}

// Accept relay number (unsigned) and numeric API status value (unsigned - 0, 1 and 2)
rpn_error status(rpn_context & ctxt, bool force) {
    rpn_value id;
    rpn_value status;

    rpn_stack_pop(ctxt, id);
    rpn_stack_pop(ctxt, status);

    rpn_uint value = status.toUint();
    if (value == 2) {
        ::relayToggle(id.toUint());
    } else if (::relayStatusTarget(id.toUint()) != (value == 1)) {
        ::relayStatus(id.toUint(), value == 1);
    }

    return 0;
}

void init(rpn_context& context) {
    relayOnStatusChange(updateVariables);

    // always apply status, allow to reset timers when called
    rpn_operator_set(context, "relay_reset", 2, [](rpn_context& ctxt) {
        return status(ctxt, true);
    });

    // only update status when target status differs, keep running timers
    rpn_operator_set(context, "relay", 2, [](rpn_context& ctxt) {
        return status(ctxt, false);
    });
}

} // namespace relay

#endif // RELAY_SUPPORT

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

namespace light {

void updateVariables() {
    auto channels = lightChannels();

    char name[32] = {0};
    for (decltype(channels) channel = 0; channel < channels; ++channel) {
        auto value = rpn_value(static_cast<rpn_int>(lightChannel(channel)));
        snprintf(name, sizeof(name), "channel%u", channel);
        rpn_variable_set(internal::context, name, std::move(value));
    }

    schedule();
}

void init(rpn_context& context) {
    lightOnReport(updateVariables);

    rpn_operator_set(context, "update", 0, [](rpn_context& ctxt) -> rpn_error {
        ::lightUpdate();
        return 0;
    });

    rpn_operator_set(context, "brightness", 0, [](rpn_context& ctxt) -> rpn_error {
        rpn_value value { static_cast<rpn_int>(::lightBrightness()) };
        rpn_stack_push(ctxt, value);
        return 0;
    });

    rpn_operator_set(context, "set_brightness", 1, [](rpn_context& ctxt) -> rpn_error {
        rpn_value value;
        rpn_stack_pop(ctxt, value);

        ::lightBrightness(value.toInt());
        return 0;
    });

    rpn_operator_set(context, "channel", 2, [](rpn_context& ctxt) -> rpn_error {
        rpn_value id;
        rpn_stack_pop(ctxt, id);

        rpn_value value;
        rpn_stack_pop(ctxt, value);

        ::lightChannel(id.toUint(), id.toInt());
        return 0;
    });
}

} // namespace light

#endif // LIGHT_PROVIDER

#if RFB_SUPPORT
namespace rfbridge {

struct Code {
    unsigned char protocol;
    String raw;
    size_t count;
    decltype(millis()) last;
};

struct Match {
    unsigned char protocol;
    String raw;
};

namespace build {

constexpr uint32_t RepeatWindow { 2000ul };
constexpr uint32_t MatchWindow { 2000ul };
constexpr uint32_t StaleDelay { 10000ul };

} // namespace build

namespace settings {
namespace keys {

PROGMEM_STRING(RepeatWindow, "rfbRepeatWindow");
PROGMEM_STRING(MatchWindow, "rfbWatchWindow");
PROGMEM_STRING(StaleDelay, "rfbStaleDelay");

} // namespace keys

uint32_t repeatWindow() {
    return getSetting(keys::RepeatWindow, build::RepeatWindow);
}

uint32_t matchWindow() {
    return getSetting(keys::MatchWindow, build::MatchWindow);
}

uint32_t staleDelay() {
    return getSetting(keys::StaleDelay, build::StaleDelay);
}

} // namespace settings

namespace internal {

// TODO: in theory, we could do with forward_list. however, this would require a more complicated removal process,
//       as we would no longer know the previous element and would need to track 2 elements at a time
using Codes = std::list<Code>;
Codes codes;

Codes::iterator find(Codes& container, unsigned char protocol, StringView match) {
    return std::find_if(container.begin(), container.end(),
        [protocol, &match](const Code& code) {
            return (code.protocol == protocol) && (code.raw == match);
        });
}

uint32_t repeat_window { build::RepeatWindow };
uint32_t match_window { build::MatchWindow };
uint32_t stale_delay { build::StaleDelay };

} // namespace internal

rpn_error sequence(rpn_context& ctxt) {
    auto raw_second = rpn_stack_pop(ctxt);
    auto proto_second = rpn_stack_pop(ctxt);

    auto raw_first = rpn_stack_pop(ctxt);
    auto proto_first = rpn_stack_pop(ctxt);

    // find 2 codes in the same order and save pointers
    Match match[2] {
        {static_cast<unsigned char>(proto_first.toUint()), raw_first.toString()},
        {static_cast<unsigned char>(proto_second.toUint()), raw_second.toString()}
    };
    Code* refs[2] {nullptr, nullptr};

    for (auto& recent : internal::codes) {
        if ((refs[0] != nullptr) && (refs[1] != nullptr)) {
            break;
        }
        for (int index = 0; index < 2; ++index) {
            if ((refs[index] == nullptr)
             && (match[index].protocol == recent.protocol)
             && (match[index].raw == recent.raw)) {
                refs[index] = &recent;
            }
        }
    }

    if ((refs[0] == nullptr) || (refs[1] == nullptr)) {
        return rpn_operator_error::CannotContinue;
    }

    // purge codes to avoid matching again on the next rules run
    if ((millis() - refs[0]->last) > (millis() - refs[1]->last)) {
        internal::codes.remove_if([&refs](Code& code) {
            return (refs[0] == &code) || (refs[1] == &code);
        });
        return rpn_operator_error::Ok;
    }

    return rpn_operator_error::CannotContinue;
}

rpn_error sendCode(rpn_context& ctxt) {
    auto code = rpn_stack_pop(ctxt);
    if (!code.isString()) {
        return rpn_operator_error::InvalidArgument;
    }

    ::rfbSend(code.toString());
    return rpn_operator_error::Ok;
}

rpn_error popCode(rpn_context& ctxt) {
    auto code = rpn_stack_pop(ctxt);
    auto proto = rpn_stack_pop(ctxt);

    auto result = internal::find(internal::codes, proto.toUint(), code.toString());
    if (result == internal::codes.end()) {
        return rpn_operator_error::CannotContinue;
    }

    internal::codes.erase(result);
    return rpn_operator_error::Ok;
}

rpn_error codeInfo(rpn_context& ctxt) {
    auto code = rpn_stack_pop(ctxt);
    auto proto = rpn_stack_pop(ctxt);

    auto result = internal::find(internal::codes, proto.toUint(), code.toString());
    if (result == internal::codes.end()) {
        return rpn_operator_error::CannotContinue;
    }

    rpn_stack_push(ctxt, rpn_value(
        static_cast<rpn_uint>((*result).count)));
    rpn_stack_push(ctxt, rpn_value(
        static_cast<rpn_uint>((*result).last)));

    return rpn_operator_error::Ok;
}

rpn_error matchAndWait(rpn_context& ctxt) {
    auto code = rpn_stack_pop(ctxt);
    auto proto = rpn_stack_pop(ctxt);
    auto count = rpn_stack_pop(ctxt);
    auto time = rpn_stack_pop(ctxt);

    auto result = internal::find(internal::codes, proto.toUint(), code.toString());
    if (result == internal::codes.end()) {
        return rpn_operator_error::CannotContinue;
    }

    if ((*result).count < count.toUint()) {
        return rpn_operator_error::CannotContinue;
    }

    // purge code to avoid matching again on the next rules run
    if (rpn_operator_error::Ok == operators::runners::handle(ctxt, Runner::Policy::OneShot, time.toUint())) {
        internal::codes.erase(result);
        return rpn_operator_error::Ok;
    }

    return rpn_operator_error::CannotContinue;
}

rpn_error match(rpn_context& ctxt) {
    auto code = rpn_stack_pop(ctxt);
    auto proto = rpn_stack_pop(ctxt);
    auto count = rpn_stack_pop(ctxt);

    auto result = internal::find(internal::codes, proto.toUint(), code.toString());
    if (result == internal::codes.end()) {
        return rpn_operator_error::CannotContinue;
    }

    // only process recent codes, ignore when rule is processing outside of this small window
    if (millis() - (*result).last >= internal::match_window) {
        return rpn_operator_error::CannotContinue;
    }

    // purge code to avoid matching again on the next rules run
    if ((*result).count == count.toUint()) {
        internal::codes.erase(result);
        return rpn_operator_error::Ok;
    }

    return rpn_operator_error::CannotContinue;
}

void codeHandler(unsigned char protocol, StringView raw_code) {
    // remove really old codes that we have not seen in a while to avoid memory exhaustion
    auto ts = millis();
    auto old = std::remove_if(internal::codes.begin(), internal::codes.end(), [ts](Code& code) {
        return (ts - code.last) >= internal::stale_delay;
    });

    if (old != internal::codes.end()) {
        internal::codes.erase(old, internal::codes.end());
    }

    auto result = internal::find(internal::codes, protocol, raw_code);
    if (result != internal::codes.end()) {
        // we also need to reset the counter at a certain point to allow next batch of repeats to go through
        if (millis() - (*result).last >= internal::repeat_window) {
            (*result).count = 0;
        }
        (*result).last = millis();
        (*result).count += 1u;
    } else {
        internal::codes.push_back({protocol, raw_code.toString(), 1u, millis()});
    }

    schedule();
}

PROGMEM_STRING(RfbCodes, "RFB.CODES");

void rfb_codes(::terminal::CommandContext&& ctx) {
    for (auto& code : internal::codes) {
        char buffer[128] = {0};
        snprintf_P(buffer, sizeof(buffer),
            PSTR("proto=%u raw=\"%s\" count=%u last=%u\n"),
            code.protocol, code.raw.c_str(), code.count, code.last);
        ctx.output.print(buffer);
    }

    terminalOK(ctx);
}

static ::terminal::Command RfbCommands[] PROGMEM {
    {RfbCodes, rfb_codes},
};

void init(rpn_context& context) {
    // - Repeat window is an arbitrary time, just about 3-4 more times it takes for
    //   a code to be sent again when holding a generic remote button
    //   Code counter is reset to 0 when outside of the window.
    // - Stale delay allows the handler to remove really old codes.
    //   (TODO: can this happen in loop() cb instead?)
    internal::repeat_window = settings::repeatWindow();
    internal::match_window = settings::matchWindow();
    internal::stale_delay = settings::staleDelay();

#if TERMINAL_SUPPORT
    espurna::terminal::add(RfbCommands);
#endif

    // Main bulk of the processing goes on in here
    ::rfbOnCode(codeHandler);

    // And codes can later be accessed by operators
    rpn_operator_set(context, "rfb_send", 1, sendCode);
    rpn_operator_set(context, "rfb_pop", 2, popCode);
    rpn_operator_set(context, "rfb_info", 2, codeInfo);
    rpn_operator_set(context, "rfb_sequence", 4, sequence);
    rpn_operator_set(context, "rfb_match", 3, match);
    rpn_operator_set(context, "rfb_match_wait", 4, matchAndWait);
}

} // namespace rfbridge
#endif // RFB_SUPPORT

#if SENSOR_SUPPORT
namespace sensor {

void updateVariables(const espurna::sensor::Value& value) {
    static_assert(std::is_same<decltype(value.value), rpn_float>::value, "");

    auto topic = value.topic;
    topic.replace("/", "");

    rpn_variable_set(internal::context,
            topic, rpn_value(static_cast<rpn_float>(value.value)));
}

void init(rpn_context&) {
    sensorOnMagnitudeRead(updateVariables);
}

} // namespace sensor
#endif // SENSOR_SUPPORT

#if DEBUG_SUPPORT
namespace debug {

void init(rpn_context& context) {
    rpn_operator_set(context, "dbgmsg", 1, [](rpn_context & ctxt) -> rpn_error {
        rpn_value message;
        rpn_stack_pop(ctxt, message);

        DEBUG_MSG_P(PSTR("[RPN] %s\n"), message.toString().c_str());
        return 0;
    });
}

} // namespace debug
#endif

namespace system {

using SystemSleepAction = bool(*)(sleep::Microseconds);

rpn_error with_sleep_duration(rpn_context& ctxt, SystemSleepAction action) {
    auto value = rpn_stack_pop(ctxt).checkedToUint();
    if (!value.ok()) {
        return value.error();
    }

    if (!action(sleep::Microseconds{ value.value() })) {
        return rpn_operator_error::CannotContinue;
    }

    return 0;
}

void init(rpn_context& context) {
    rpn_operator_set(context, "delay", 1, [](rpn_context& ctxt) -> rpn_error {
        auto ms = rpn_stack_pop(ctxt);
        delay(ms.toUint());
        return 0;
    });

    rpn_operator_set(context, "yield", 0, [](rpn_context& ctxt) -> rpn_error {
        yield();
        return 0;
    });

    rpn_operator_set(context, "reset", 0, [](rpn_context& ctxt) -> rpn_error {
        static bool once = ([]() {
            prepareReset(CustomResetReason::Rule);
            return true;
        })();

        return once
            ? rpn_operator_error::CannotContinue
            : rpn_operator_error::Ok;
    });

    rpn_operator_set(internal::context, "millis", 0, [](rpn_context & ctxt) -> rpn_error {
        rpn_stack_push(ctxt, rpn_value(static_cast<uint32_t>(millis())));
        return 0;
    });

    rpn_operator_set(context, "light_sleep", 1, [](rpn_context& ctxt) -> rpn_error {
        return with_sleep_duration(ctxt, [](sleep::Microseconds time) -> bool {
            return instantLightSleep(time);
        });
    });

    rpn_operator_set(context, "deep_sleep", 1, [](rpn_context& ctxt) -> rpn_error {
        return with_sleep_duration(ctxt, instantDeepSleep);
    });

    rpn_operator_set(context, "mem?", 0, [](rpn_context& ctxt) -> rpn_error {
        rpn_stack_push(ctxt, rpn_value(::rtcmemStatus()));
        return 0;
    });

    rpn_operator_set(context, "mem_write", 2, [](rpn_context& ctxt) -> rpn_error {
        auto addr = rpn_stack_pop(ctxt).toUint();
        auto value = rpn_stack_pop(ctxt).toUint();

        if (addr < RTCMEM_BLOCKS) {
            auto* rtcmem = reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR);
            *(rtcmem + addr) = value;
            return 0;
        }

        return rpn_operator_error::InvalidArgument;
    });

    rpn_operator_set(context, "mem_read", 1, [](rpn_context& ctxt) -> rpn_error {
        auto addr = rpn_stack_pop(ctxt).toUint();

        if (addr < RTCMEM_BLOCKS) {
            auto* rtcmem = reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR);
            rpn_uint result = *(rtcmem + addr);
            rpn_stack_push(ctxt, rpn_value(result));
            return 0;
        }

        return rpn_operator_error::InvalidArgument;
    });
}

} // namespace system

namespace wifi {

void init(rpn_context& context) {
    rpn_operator_set(context, "stations", 0, [](rpn_context& ctxt) -> rpn_error {
        rpn_stack_push(ctxt, rpn_value {
            static_cast<rpn_uint>(wifiApStations()) });
        return 0;
    });

    rpn_operator_set(context, "disconnect", 0, [](rpn_context& ctxt) -> rpn_error {
        wifiDisconnect();
        yield();
        return 0;
    });

    rpn_operator_set(context, "rssi", 0, [](rpn_context& ctxt) -> rpn_error {
        const rpn_int rssi = wifiConnected()
            ? wifi_station_get_rssi()
            : -127;
        rpn_stack_push(ctxt, rpn_value { rssi });
        return 0;
    });
}

} // namespace wifi

void init(rpn_context& context) {
    rpn_init(context);

    runners::init(context);
    system::init(context);
    wifi::init(context);

#if DEBUG_SUPPORT
    debug::init(context);
#endif
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    light::init(context);
#endif
#if MQTT_SUPPORT
    mqtt::init(context);
#endif
#if NTP_SUPPORT
    ntp::init(context);
#endif
#if RFB_SUPPORT
    rfbridge::init(context);
#endif
#if RELAY_SUPPORT
    relay::init(context);
#endif
#if SENSOR_SUPPORT
    sensor::init(context);
#endif
}

} // namespace operators

void init() {
    operators::init(internal::context);

    // XXX: workaround for the vector 2x growth on push. will need to fix this in the rpnlib
    internal::context.operators.shrink_to_fit();

    DEBUG_MSG_P(PSTR("[RPN] Registered %u operators\n"), internal::context.operators.size());
}

void run() {
#if MQTT_SUPPORT
    if (!mqtt::variables.empty()) {
        schedule();
    }

    for (auto& variable : mqtt::variables) {
        rpn_variable_set(internal::context, variable.name, variable.value);
    }
    mqtt::variables.clear();
#endif

    if (!due()) {
        return;
    }

    size_t index { 0 };
    String rule;
    for (;;) {
        rule = settings::rule(index++);
        if (!rule.length()) {
            break;
        }

        rpn_process(internal::context, rule.c_str());
        rpn_stack_clear(internal::context);
    }

    if (!settings::sticky()) {
        rpn_variables_clear(internal::context);
    }
}

void loop() {
    RunnersHandler handler(internal::runners);
    run();
}

void configure() {
#if MQTT_SUPPORT
    if (mqttConnected()) {
        mqtt::subscribe();
    }
#endif
    internal::run_delay = rpnrules::settings::delay();
}

void setup() {
    init();
    configure();

#if TERMINAL_SUPPORT
    terminal::setup();
#endif

#if WEB_SUPPORT
    wsRegister()
        .onVisible(web::onVisible)
        .onConnected(web::onConnected)
        .onKeyCheck(web::onKeyCheck);
#endif

    espurnaRegisterReload(configure);
    espurnaRegisterLoop(loop);

    reset(true);
}

} // namespace
} // namespace rpnrules
} // namespace espurna

void rpnSetup() {
    espurna::rpnrules::setup();
}

#endif // RPN_RULES_SUPPORT
