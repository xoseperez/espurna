/*

SCHEDULER MODULE

Copyright (C) 2017 by faina09
Adapted by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "scheduler.h"

#if SCHEDULER_SUPPORT

#include "api.h"
#include "light.h"
#include "mqtt.h"
#include "ntp.h"
#include "ntp_timelib.h"
#include "relay.h"
#include "ws.h"
#include "curtain_kingart.h"

// -----------------------------------------------------------------------------

namespace scheduler {
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

constexpr int defaultType() {
    return SCHEDULER_TYPE_NONE;
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

constexpr const char* const weekdays() {
    return SCHEDULER_WEEKDAYS;
}

constexpr bool restoreLast() {
    return 1 == SCHEDULER_RESTORE_LAST_SCHEDULE;
}

} // namespace build

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
    int type;
    int action;
    bool restore;
    bool utc;
    Weekdays weekdays;
    int hour;
    int minute;
};

using Schedules = std::vector<Schedule>;

namespace debug {

const char* type(int type) {
    switch (type) {
    case SCHEDULER_TYPE_NONE:
        return "none";
    case SCHEDULER_TYPE_SWITCH:
        return "switch";
    case SCHEDULER_TYPE_DIM:
        return "channel";
    case SCHEDULER_TYPE_CURTAIN:
        return "curtain";
    }

    return "unknown";
}

const char* type(const Schedule& schedule) {
    return type(schedule.type);
}

void show(const Schedules& schedules) {
    size_t index { 0 };
    for (auto& schedule : schedules) {
        DEBUG_MSG_P(
            PSTR("[SCH] #%d: %s #%d => %d at %02d:%02d (%s) on %s%s\n"),
            index++, scheduler::debug::type(schedule), schedule.target,
            schedule.action, schedule.hour, schedule.minute,
            schedule.utc ? "UTC" : "local time",
            schedule.weekdays.toString().c_str(),
            schedule.enabled ? "" : " (disabled)");
    }
}

} // namespace debug

namespace settings {

const char* const schema_keys[] PROGMEM = {
    "schEnabled",
    "schTarget",
    "schType",
    "schAction",
    "schRestore",
    "schUTC",
    "schWDs"
    "schHour",
    "schMinute",
};

bool enabled(size_t index) {
    return getSetting({"schEnabled", index}, false);
}

size_t target(size_t index) {
    return getSetting({"schTarget", index}, build::defaultTarget());
}

int type(size_t index) {
    return getSetting({"schType", index}, build::defaultType());
}

int action(size_t index) {
    return getSetting({"schAction", index}, build::action());
}

bool restore(size_t index) {
    return getSetting({"schRestore", index}, build::restoreLast());
}

Weekdays weekdays(size_t index) {
    return Weekdays(getSetting({"schWDs", index}, build::weekdays()));
}

bool utc(size_t index) {
    return getSetting({"schUTC", index}, build::utc());
}

int hour(size_t index) {
    return getSetting({"schHour", index}, build::hour());
}

int minute(size_t index) {
    return getSetting({"schMinute", index}, build::minute());
}

Schedule schedule(size_t index) {
    return Schedule{
        enabled(index),
        target(index),
        type(index),
        action(index),
        restore(index),
        utc(index),
        weekdays(index),
        hour(index),
        minute(index)};
}

void gc(size_t total) {
    for (size_t i = total; i < build::max(); ++i) {
        for (auto* key : schema_keys) {
            delSetting({key, i});
        }
    }
}

Schedules schedules() {
    Schedules out;
    out.reserve(build::max());

    for (size_t i = 0; i < build::max(); ++i) {
        auto current = schedule(i);
        if (current.type == SCHEDULER_TYPE_NONE) {
            break;
        }
        out.push_back(std::move(current));
    }

    return out;
}

void migrate(int version) {
    if (version && (version < 6)) {
        moveSettings("schSwitch", "schTarget");
    }
}

} // namespace settings

// -----------------------------------------------------------------------------

namespace api {
#if API_SUPPORT

namespace internal {

template <typename V = const char*>
bool setKey(const SettingsKey& k, const JsonVariant& v) {
    if (!v.is<V>()) {
        return false;
    }
    return setSetting(k, v.as<V>());
}

template <>
bool setKey<const char*>(const SettingsKey& k, const JsonVariant& v) {
    const auto& tmp = v.as<const char*>();
    if (tmp == nullptr) {
        return false;
    }
    return setSetting(k, tmp);
}

template <typename T = const char*, typename ObjKey>
bool setKey(const SettingsKey& k, const JsonObject& o, const ObjKey& key) {
    if (!o.containsKey(key)) {
        return false;
    }
    return setKey<T>(k, o[key]);
}

} // namespace internal

void print(size_t id, JsonObject& root) {
    auto schedule = settings::schedule(id);
    root["enabled"] = schedule.enabled;
    root["target"] = schedule.target;
    root["type"] = schedule.type;
    root["action"] = schedule.action;
    root["restore"] = schedule.restore;
    root["utc"] = schedule.utc;
    root["weekdays"] = schedule.weekdays.toString();
    root["hour"] = schedule.hour;
    root["minute"] = schedule.minute;
}

bool set(const size_t id, JsonObject& root) {
    if (!internal::setKey<int>({"schType", id}, root, "type")) {
        return false;
    }

    if (!internal::setKey<int>({"schTarget", id}, root, "target")) {
        return false;
    }

    if (root.containsKey("enabled")) {
        const auto& enabled = root["enabled"];
        if (!internal::setKey<bool>({"schEnabled", id}, enabled)) {
            setSetting({"schEnabled", id}, enabled.as<String>());
        }
    }

    if (root.containsKey("utc")) {
        const auto& utc = root["utc"];
        if (!internal::setKey<bool>({"schUTC", id}, utc)) {
            setSetting({"schUTC", id}, utc.as<String>());
        }
    }

    if (root.containsKey("action")) {
        const auto& action = root["action"];
        if (action.is<const char*>()) {
            setSetting({"schAction", id}, int(relayParsePayload(action.as<const char*>())));
        } else {
            internal::setKey<int>({"schAction", id}, action);
        }
    }

    internal::setKey<int>({"schHour", id}, root, "hour");
    internal::setKey<int>({"schMinute", id}, root, "minute");
    internal::setKey({"schWDs", id}, root, "weekdays");

    return true;
}

#endif  // API_SUPPORT
} // namespace api

// -----------------------------------------------------------------------------

namespace web {
#if WEB_SUPPORT

bool onKey(const char* key, JsonVariant&) {
    return (strncmp(key, "sch", 3) == 0);
}

void onVisible(JsonObject& root) {
    if (schedulable()) {
        root["schVisible"] = 1;
    }
}

void onConnected(JsonObject &root){
    if (!schedulable()) return;

    JsonObject& config = root.createNestedObject("schConfig");
    config["max"] = build::max();

    JsonArray& schema = config.createNestedArray("schema");
    schema.copyFrom(settings::schema_keys, sizeof(settings::schema_keys) / sizeof(*settings::schema_keys));

    uint8_t size = 0;

    JsonArray& schedules = config.createNestedArray("schedules");

    for (size_t id = 0; id < build::max(); ++id) {
        auto schedule = settings::schedule(id);
        if (schedule.type == SCHEDULER_TYPE_NONE) {
            break;
        }

        JsonArray& entry = schedules.createNestedArray();
        ++size;

        entry.add(schedule.enabled);

        entry.add(schedule.target);
        entry.add(schedule.type);
        entry.add(schedule.action);

        entry.add(schedule.restore);
        entry.add(schedule.utc);

        entry.add(schedule.weekdays.toString());
        entry.add(schedule.hour);
        entry.add(schedule.minute);
    }

    config["size"] = size;
    config["start"] = 0;
}

#endif
} // namespace web

void action(int type, size_t target, int action) {
    if (SCHEDULER_TYPE_SWITCH == type) {
        if (action == 2) {
            relayToggle(target);
        } else {
            relayStatus(target, action);
        }
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    } else if (SCHEDULER_TYPE_DIM == type) {
        lightChannel(target, action);
        lightUpdate();
#endif
#if CURTAIN_SUPPORT
    } else if (SCHEDULER_TYPE_CURTAIN == sch_type) {
        curtainSetPosition(target, action);
#endif
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
    int type;
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
        DEBUG_MSG_P(PSTR("[SCH] Restoring %s #%u => %u (scheduled at %02d:%02d %d day(s) ago)\n"),
                scheduler::debug::type(v.type), v.target, v.action,
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
                    scheduler::debug::type(schedule),
                    schedule.target, schedule.action);
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

} // namespace scheduler

// -----------------------------------------------------------------------------

void schSetup() {
    scheduler::settings::migrate(migrateVersion());

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
                JsonArray& scheds = root.createNestedArray("schedules");
                for (size_t i = 0; i < scheduler::build::max(); ++i) {
                    if (hasSetting({"schTarget", i})) {
                        auto& root = scheds.createNestedObject();
                        scheduler::api::print(i, root);
                    }
                }
                return true;
            },
            [](ApiRequest&, JsonObject& root) {
                size_t id = 0;
                while (hasSetting({"schType", id})) {
                    ++id;
                }

                if (id < scheduler::build::max()) {
                    return scheduler::api::set(id, root);
                }

                return false;
            });

        apiRegister(
            F(MQTT_TOPIC_SCHEDULE "/+"),
            [](ApiRequest& r, JsonObject& root) {
                const auto& id_param = r.wildcard(0);
                size_t id;
                if (tryParseId(id_param.c_str(), scheduler::build::max, id)) {
                    scheduler::api::print(id, root);
                    return true;
                }
                return false;
            },
            [](ApiRequest& r, JsonObject& root) {
                const auto& id_param = r.wildcard(0);
                size_t id;
                if (tryParseId(id_param.c_str(), scheduler::build::max, id)) {
                    return scheduler::api::set(id, root);
                }
                return false;
            });
    #endif

    static bool initial { true };
    ntpOnTick([](NtpTick tick) {
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
