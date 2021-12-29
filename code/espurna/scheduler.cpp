/*

SCHEDULER MODULE

Copyright (C) 2017 by faina09
Adapted by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if SCHEDULER_SUPPORT

#include "api.h"
#include "light.h"
#include "mqtt.h"
#include "ntp.h"
#include "ntp_timelib.h"
#include "curtain_kingart.h"
#include "relay.h"
#include "scheduler.h"
#include "ws.h"

// -----------------------------------------------------------------------------

namespace scheduler {

enum class Type {
    None,
    Relay,
    Channel,
    Curtain
};

namespace {

struct Weekdays {
    Weekdays() = default;

    // TimeLib mask, with Monday as 1 and Sunday as 7
    // ctime order is Sunday as 0 and Saturday as 6
    explicit Weekdays(const String& pattern) {
        const char* p = pattern.c_str();
        while (*p != '\0') {
            switch (*p) {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
                _mask |= 1 << (*p - '1');
                break;
            }
            ++p;
        }
    }

    String toString() const {
        String out;
        for (int day = 0; day < 7; ++day) {
            if (_mask & (1 << day)) {
                if (out.length()) {
                    out += ',';
                }
                out += static_cast<char>('0' + day + 1);
            }
        }

        return out;
    }

    bool match(const tm& other) const {
        switch (other.tm_wday) {
        case 0:
            return _mask & (1 << 6);
        case 1 ... 6:
            return _mask & (1 << (other.tm_wday - 1));
        }

        return false;
    }

    int mask() const {
        return _mask;
    }

private:
    int _mask { 0 };
};

struct Schedule {
    bool enabled;
    size_t target;
    Type type;
    int action;
    bool restore;
    bool utc;
    Weekdays weekdays;
    int hour;
    int minute;
};

using Schedules = std::vector<Schedule>;

} // namespace
} // namespace scheduler

namespace settings {
namespace options {
namespace {

using ::settings::options::Enumeration;

alignas(4) static constexpr char None[] PROGMEM = "none";
alignas(4) static constexpr char Relay[] PROGMEM = "relay";
alignas(4) static constexpr char Channel[] PROGMEM = "channel";
alignas(4) static constexpr char Curtain[] PROGMEM = "curtain";

static constexpr std::array<Enumeration<scheduler::Type>, 4> SchedulerTypeOptions PROGMEM {
    {{scheduler::Type::None, None},
     {scheduler::Type::Relay, Relay},
     {scheduler::Type::Channel, Channel},
     {scheduler::Type::Curtain, Curtain}}
};

} // namespace
} // namespace options

namespace internal {
namespace {

using options::SchedulerTypeOptions;

} // namespace

template<>
scheduler::Type convert(const String& value) {
    return convert(SchedulerTypeOptions, value, scheduler::Type::None);
}

String serialize(scheduler::Type value) {
    return serialize(SchedulerTypeOptions, value);
}

} // namespace internal
} // namespace settings

namespace scheduler {
namespace {
namespace build {

constexpr size_t max() {
    return SCHEDULER_MAX_SCHEDULES;
}

constexpr int restoreOffsetMax() {
    return 2; // today and yesterday
}

constexpr size_t defaultTarget() {
    return 0; // aka relay#0 or channel#0
}

constexpr Type defaultType() {
    return Type::None;
}

constexpr bool utc() {
    return false; // use local time by default
}

constexpr int hour() {
    return 0;
}

constexpr int minute() {
    return 0;
}

constexpr int action() {
    return 0;
}

const __FlashStringHelper* weekdays() {
    return F(SCHEDULER_WEEKDAYS);
}

constexpr bool restoreLast() {
    return 1 == SCHEDULER_RESTORE_LAST_SCHEDULE;
}

} // namespace build

bool supported(Type type) {
    switch (type) {
    case Type::None:
        break;

    case Type::Relay:
#if RELAY_SUPPORT
        return true;
#else
        return false;
#endif

    case Type::Channel:
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        return true;
#else
        return false;
#endif

    case Type::Curtain:
#if CURTAIN_SUPPORT
        return true;
#else
        return false;
#endif
    }

    return false;
}

bool schedulable() {
    return false
#if RELAY_SUPPORT
    || relayCount()
#endif
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    || lightChannels()
#endif
#if CURTAIN_SUPPORT
    || curtainCount()
#endif
    ;
}

namespace debug {

String type(Type type) {
    return settings::internal::serialize(type);
}

String type(const Schedule& schedule) {
    return type(schedule.type);
}

void show(const Schedules& schedules) {
    size_t index { 0 };
    for (auto& schedule : schedules) {
        DEBUG_MSG_P(
            PSTR("[SCH] #%d: %s #%d => %d at %02d:%02d (%s) on %s%s\n"),
            index++, scheduler::debug::type(schedule).c_str(), schedule.target,
            schedule.action, schedule.hour, schedule.minute,
            schedule.utc ? "UTC" : "local time",
            schedule.weekdays.toString().c_str(),
            schedule.enabled ? "" : " (disabled)");
    }
}

} // namespace debug

namespace settings {
namespace keys {
namespace {

alignas(4) static constexpr char Enabled[] PROGMEM = "schEnabled";
alignas(4) static constexpr char Target[] PROGMEM = "schTarget";
alignas(4) static constexpr char Type[] PROGMEM = "schType";
alignas(4) static constexpr char Action[] PROGMEM = "schAction";
alignas(4) static constexpr char Restore[] PROGMEM = "schRestore";
alignas(4) static constexpr char UseUTC[] PROGMEM = "schUTC";
alignas(4) static constexpr char Weekdays[] PROGMEM = "schWDs";
alignas(4) static constexpr char Hour[] PROGMEM = "schHour";
alignas(4) static constexpr char Minute[] PROGMEM = "schMinute";

} // namespace
} // namespace keys

bool enabled(size_t index) {
    return getSetting({keys::Enabled, index}, false);
}

size_t target(size_t index) {
    return getSetting({keys::Target, index}, build::defaultTarget());
}

Type type(size_t index) {
    return getSetting({keys::Type, index}, build::defaultType());
}

int action(size_t index) {
    return getSetting({keys::Action, index}, build::action());
}

bool restore(size_t index) {
    return getSetting({keys::Restore, index}, build::restoreLast());
}

Weekdays weekdays(size_t index) {
    return Weekdays(getSetting({keys::Weekdays, index}, build::weekdays()));
}

bool utc(size_t index) {
    return getSetting({keys::UseUTC, index}, build::utc());
}

int hour(size_t index) {
    return getSetting({keys::Hour, index}, build::hour());
}

int minute(size_t index) {
    return getSetting({keys::Minute, index}, build::minute());
}

namespace internal {

#define ID_VALUE(NAME, FUNC)\
String NAME (size_t id) {\
    return ::settings::internal::serialize(FUNC(id));\
}

ID_VALUE(enabled, settings::enabled)
ID_VALUE(target, settings::target)
ID_VALUE(type, settings::type)
ID_VALUE(action, settings::action)
ID_VALUE(restore, settings::restore)
ID_VALUE(utc, settings::utc)

String weekdays(size_t index) {
    return settings::weekdays(index).toString();
}

ID_VALUE(hour, settings::hour)
ID_VALUE(minute, settings::minute)

} // namespace internal

static constexpr ::settings::query::IndexedSetting IndexedSettings[] PROGMEM {
    {keys::Enabled, internal::enabled},
    {keys::Target, internal::target},
    {keys::Type, internal::type},
    {keys::Action, internal::action},
    {keys::Restore, internal::restore},
    {keys::UseUTC, internal::utc},
    {keys::Weekdays, internal::weekdays},
    {keys::Hour, internal::hour},
    {keys::Minute, internal::minute}
};

Schedule schedule(size_t index, Type type) {
    return {
        .enabled = enabled(index),
        .target = target(index),
        .type = type,
        .action = action(index),
        .restore = restore(index),
        .utc = utc(index),
        .weekdays = weekdays(index),
        .hour = hour(index),
        .minute = minute(index)
    };
}

Schedule schedule(size_t index) {
    return schedule(index, type(index));
}

size_t count() {
    size_t out { 0 };

    for (size_t index = 0; index < build::max(); ++index) {
        auto type = settings::type(index);
        if (!supported(type)) {
            break;
        }

        ++out;
    }

    return out;
}

void gc(size_t total) {
    for (size_t index = total; index < build::max(); ++index) {
        for (auto setting : IndexedSettings) {
            delSetting({setting.prefix().c_str(), index});
        }
    }
}

Schedules schedules() {
    Schedules out;
    out.reserve(build::max());

    for (size_t index = 0; index < build::max(); ++index) {
        auto type = settings::type(index);
        if (!supported(type)) {
            break;
        }

        out.emplace_back(settings::schedule(index, type));
    }

    return out;
}

void migrate(int version) {
    if (version < 6) {
        moveSettings(PSTR("schSwitch"), keys::Target);
    }
}

namespace query {

bool checkSamePrefix(::settings::StringView key) {
    alignas(4) static constexpr char Prefix[] PROGMEM = "sch";
    return ::settings::query::samePrefix(key, Prefix);
}

String findIndexedValueFrom(::settings::StringView key) {
    return ::settings::query::IndexedSetting::findValueFrom(count(), IndexedSettings, key);
}

void setup() {
    settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findIndexedValueFrom
    });
}

} // namespace query
} // namespace settings

// -----------------------------------------------------------------------------

#if API_SUPPORT
namespace api {
namespace keys {

alignas(4) static constexpr char Enabled[] PROGMEM = "enabled";
alignas(4) static constexpr char Target[] PROGMEM = "enabled";
alignas(4) static constexpr char Type[] PROGMEM = "type";
alignas(4) static constexpr char Action[] PROGMEM = "action";
alignas(4) static constexpr char Restore[] PROGMEM = "restore";
alignas(4) static constexpr char UseUTC[] PROGMEM = "utc";
alignas(4) static constexpr char Weekdays[] PROGMEM = "weekdays";
alignas(4) static constexpr char Hour[] PROGMEM = "hour";
alignas(4) static constexpr char Minute[] PROGMEM = "minute";

} // namespace keys

void print(JsonObject& root, const Schedule& schedule) {
    root[FPSTR(keys::Enabled)] = schedule.enabled;
    root[FPSTR(keys::Target)] = schedule.target;
    root[FPSTR(keys::Type)] = ::settings::internal::serialize(schedule.type);
    root[FPSTR(keys::Action)] = schedule.action;
    root[FPSTR(keys::Restore)] = schedule.restore;
    root[FPSTR(keys::UseUTC)] = schedule.utc;
    root[FPSTR(keys::Weekdays)] = schedule.weekdays.toString();
    root[FPSTR(keys::Hour)] = schedule.hour;
    root[FPSTR(keys::Minute)] = schedule.minute;
}

template <typename T>
bool setFromJsonIf(JsonObject& root, const char* key, size_t id, const char* jsonKey) {
    const auto* jsonKeyFpstr = FPSTR(jsonKey);
    if (root.containsKey(jsonKeyFpstr) && root.is<T>(jsonKeyFpstr)) {
        setSetting({key, id}, ::settings::internal::serialize(root[jsonKeyFpstr].as<T>()));
        return true;
    }

    return false;
}

template <>
bool setFromJsonIf<String>(JsonObject& root, const char* key, size_t id, const char* jsonKey) {
    const auto* jsonKeyFpstr = FPSTR(jsonKey);
    if (root.containsKey(jsonKeyFpstr) && root.is<String>(jsonKeyFpstr)) {
        setSetting({key, id}, root[jsonKeyFpstr].as<String>());
        return true;
    }

    return false;
}

bool set(JsonObject& root, const size_t id) {
    if (setFromJsonIf<int>(root, settings::keys::Type, id, keys::Type)) {
        setFromJsonIf<bool>(root, settings::keys::Enabled, id, keys::Enabled);
        setFromJsonIf<int>(root, settings::keys::Target, id, keys::Target);
        setFromJsonIf<int>(root, settings::keys::Action, id, keys::Action);
        setFromJsonIf<bool>(root, settings::keys::Restore, id, keys::Restore);
        setFromJsonIf<bool>(root, settings::keys::UseUTC, id, keys::UseUTC);
        setFromJsonIf<String>(root, settings::keys::Weekdays, id, keys::Weekdays);
        setFromJsonIf<int>(root, settings::keys::Hour, id, keys::Hour);
        setFromJsonIf<int>(root, settings::keys::Minute, id, keys::Minute);
        return true;
    }

    return false;
}

} // namespace api
#endif  // API_SUPPORT

// -----------------------------------------------------------------------------

namespace web {
#if WEB_SUPPORT

bool onKey(const char* key, JsonVariant&) {
    return (strncmp(key, "sch", 3) == 0);
}

void onVisible(JsonObject& root) {
    if (schedulable()) {
        wsPayloadModule(root, "sch");
    }
}

void onConnected(JsonObject& root){
    if (schedulable()) {
        ::web::ws::EnumerableConfig config{ root, STRING_VIEW("schConfig") };
        config(STRING_VIEW("schedules"), settings::count(), settings::IndexedSettings);

        auto& schedules = config.root();
        schedules["max"] = build::max();
    }
}

#endif
} // namespace web

// TODO: consider providing action as a string, which could be parsed by the
// respective module API (e.g. for lights, there could be + / - offsets)

void action(scheduler::Type type, size_t target, int action) {
    switch (type) {
    case scheduler::Type::None:
        break;

    case scheduler::Type::Relay:
        if (action == 2) {
            relayToggle(target);
        } else {
            relayStatus(target, action);
        }
        break;

    case scheduler::Type::Channel:
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightChannel(target, action);
        lightUpdate();
#endif
        break;

    case scheduler::Type::Curtain:
#if CURTAIN_SUPPORT
        curtainSetPosition(target, action);
#endif
        break;
    }
}

void action(const Schedule& schedule) {
    action(schedule.type, schedule.target, schedule.action);
}

// libc underlying implementation allows us to shift month's day (even making it negative)
// although, it is preferable to return the 'correct' struct with all of the fields updated
// XXX: newlib does not support `timegm`, this only makes sense for local time
// but, technically, this *could* do tzset and restore the original TZ before returning

tm localtimeDaysAgo(tm day, int days) {
    if (days) {
        day.tm_mday -= days;
        day.tm_hour = 0;
        day.tm_min = 0;
        day.tm_sec = 0;

        auto ts = mktime(&day);
        tm out{};
        localtime_r(&ts, &out);
        day = out;
    }

    return day;
}

int minutesLeft(const Schedule& schedule, int hour, int minute) {
    return (schedule.hour - hour) * 60 + schedule.minute - minute;
}

int minutesLeft(const Schedule& schedule, const tm& now) {
    return minutesLeft(schedule, now.tm_hour, now.tm_min);
}

// For 'restore'able schedules, do the most recent action, Normal check will take care of the current setting
// Note that this only works for local-time schedules, b/c there is no timegm to compliment mktime to offset the `tm` by a specific number of days

struct RestoredAction {
    size_t target;
    Type type;
    int action;
    int hour;
    int minute;
    int daysAgo;
};

using RestoredActions = std::forward_list<RestoredAction>;

RestoredAction prepareAction(const Schedule& schedule, int daysAgo) {
    return {schedule.target, schedule.type, schedule.action,
        schedule.hour, schedule.minute, daysAgo};
}

void restore(time_t timestamp, const Schedules& schedules) {
    RestoredActions restored;

    tm today;
    localtime_r(&timestamp, &today);

    for (auto& schedule : schedules) {
        if (schedule.enabled && schedule.restore && !schedule.utc) {
            for (int offset = 0; offset < build::restoreOffsetMax(); ++offset) {
                auto offsetDay = localtimeDaysAgo(today, offset);

                // If it is going to happen later this day or right now, simply skip and allow check() to handle it
                // Otherwise, make sure `restored` only contains actions that happened today (or N days ago), and
                // filter by the most recent ones of the same type (i.e. max hour and minute, but min daysAgo)

                if (schedule.weekdays.match(offsetDay)) {
                    if ((offset == 0) && (minutesLeft(schedule, offsetDay) >= 0)) {
                        continue;
                    }

                    auto pending = prepareAction(schedule, offset);
                    auto found = std::find_if(std::begin(restored), std::end(restored),
                            [&](const RestoredAction& lhs) {
                                return (lhs.type == pending.type) && (lhs.target == pending.target);
                            });

                    if (found != std::end(restored)) {
                        if (((*found).daysAgo >= pending.daysAgo)
                         && ((*found).hour <= pending.hour)
                         && ((*found).minute <= pending.minute)) {
                            *found = pending;
                        }
                    } else {
                        restored.push_front(pending);
                    }
                    break;
                }
            }
        }
    }

    for (auto& v : restored) {
      DEBUG_MSG_P(PSTR("[SCH] Restoring %s #%u => %u (scheduled at %02d:%02d "
                       "%d day(s) ago)\n"),
                  scheduler::debug::type(v.type).c_str(), v.target, v.action,
                  v.hour, v.minute, v.daysAgo);
      action(v.type, v.target, v.action);
    }
}

void check(time_t timestamp, const Schedules& schedules) {
    tm utc;
    gmtime_r(&timestamp, &utc);

    tm local;
    localtime_r(&timestamp, &local);

    for (auto& schedule : schedules) {
        if (!schedule.enabled) {
            continue;
        }

        auto& today = schedule.utc
            ? utc
            : local;

        if (!schedule.weekdays.match(today)) {
            continue;
        }

        // 'Next scheduled' only happens at exactly the -15min
        // e.g. at 0:00 updating the scheduler to trigger at 0:14 will not show the notification

        auto left = minutesLeft(schedule, today);
        if (left == 0) {
          DEBUG_MSG_P(PSTR("[SCH] Action at %02d:%02d (%s #%u => %u)\n"),
                      schedule.hour, schedule.minute,
                      scheduler::debug::type(schedule).c_str(), schedule.target,
                      schedule.action);
          action(schedule);
#if DEBUG_SUPPORT
        } else if (left > 0) {
            if ((left % 15 == 0) || (left < 15)) {
                DEBUG_MSG_P(PSTR("[SCH] Next scheduled action at %02d:%02d\n"),
                        schedule.hour, schedule.minute);
            }
#endif
        }
    }
}

} // namespace
} // namespace scheduler

// -----------------------------------------------------------------------------

void schSetup() {
    migrateVersion(scheduler::settings::migrate);
    scheduler::settings::query::setup();

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(scheduler::web::onVisible)
            .onConnected(scheduler::web::onConnected)
            .onKeyCheck(scheduler::web::onKey);
    #endif

    #if API_SUPPORT
        apiRegister(
            F(MQTT_TOPIC_SCHEDULE),
            [](ApiRequest&, JsonObject& root) {
                JsonArray& out = root.createNestedArray("schedules");

                auto schedules = scheduler::settings::schedules();
                for (auto& schedule : schedules) {
                    auto& root = out.createNestedObject();
                    scheduler::api::print(root, schedule);
                }

                return true;
            },
            [](ApiRequest&, JsonObject& root) {
                size_t id = 0;
                while (hasSetting({"schType", id})) {
                    ++id;
                }

                if (id < scheduler::build::max()) {
                    return scheduler::api::set(root, id);
                }

                return false;
            });

        apiRegister(
            F(MQTT_TOPIC_SCHEDULE "/+"),
            [](ApiRequest& req, JsonObject& root) {
                size_t id;
                if (tryParseId(req.wildcard(0).c_str(), scheduler::build::max, id)) {
                    scheduler::api::print(root, scheduler::settings::schedule(id));
                    return true;
                }
                return false;
            },
            [](ApiRequest& req, JsonObject& root) {
                size_t id;
                if (tryParseId(req.wildcard(0).c_str(), scheduler::build::max, id)) {
                    return scheduler::api::set(root, id);
                }
                return false;
            });
    #endif

    static bool initial { true };
    ntpOnTick([](NtpTick tick) {
        if (tick != NtpTick::EveryMinute) {
            return;
        }

        auto timestamp = now();
        auto schedules = scheduler::settings::schedules();
        if (initial) {
            initial = false;
            scheduler::settings::gc(schedules.size());
#if DEBUG_SUPPORT
            scheduler::debug::show(schedules);
#endif
            scheduler::restore(timestamp, schedules);
        }
        scheduler::check(timestamp, schedules);
    });
}

#endif // SCHEDULER_SUPPORT
