/*

HOME ASSISTANT MODULE

Original module
Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

Reworked queueing and RAM usage reduction
Copyright (C) 2019-2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#if HOMEASSISTANT_SUPPORT

#include "homeassistant.h"
#include "light.h"
#include "mqtt.h"
#include "relay.h"
#include "sensor.h"
#include "web.h"
#include "ws.h"

#include <ArduinoJson.h>

#include <forward_list>
#include <memory>

namespace espurna {
namespace homeassistant {
namespace {

enum class State {
    Disabled,
    Enabled,
};

namespace build {

constexpr bool enabled() {
    return 1 == HOMEASSISTANT_ENABLED;
}

STRING_VIEW_INLINE(Prefix, HOMEASSISTANT_PREFIX);

constexpr StringView prefix() {
    return Prefix;
}

constexpr bool retain() {
    return 1 == HOMEASSISTANT_RETAIN;
}

STRING_VIEW_INLINE(BirthTopic, HOMEASSISTANT_BIRTH_TOPIC);

constexpr StringView birthTopic() {
    return BirthTopic;
}

STRING_VIEW_INLINE(BirthPayload, HOMEASSISTANT_BIRTH_PAYLOAD);

constexpr StringView birthPayload() {
    return BirthPayload;
}

} // namespace build

namespace settings {
namespace keys {

STRING_VIEW_INLINE(Enabled, "haEnabled");
STRING_VIEW_INLINE(Prefix, "haPrefix");
STRING_VIEW_INLINE(Retain, "haRetain");

STRING_VIEW_INLINE(BirthTopic, "haBirthTopic");
STRING_VIEW_INLINE(BirthPayload, "haBirthPayload");

} // namespace keys

bool enabled() {
    return getSetting(keys::Enabled, build::enabled());
}

String prefix() {
    return getSetting(keys::Prefix, build::prefix());
}

bool retain() {
    return getSetting(keys::Retain, build::retain());
}

String birthTopic() {
    return getSetting(keys::BirthTopic, build::birthTopic());
}

String birthPayload() {
    return getSetting(keys::BirthPayload, build::birthPayload());
}

namespace query {
namespace internal {

#define EXACT_VALUE(NAME, FUNC)\
String NAME () {\
    return espurna::settings::internal::serialize(FUNC());\
}

EXACT_VALUE(enabled, settings::enabled)
EXACT_VALUE(retain, settings::retain)

} // namespace internal

static constexpr espurna::settings::query::Setting Settings[] PROGMEM {
    {keys::Enabled, internal::enabled},
    {keys::Prefix, settings::prefix},
    {keys::Retain, internal::retain},
    {keys::BirthTopic, settings::birthTopic},
    {keys::BirthPayload, settings::birthPayload},
};

STRING_VIEW_INLINE(Prefix, "ha");

bool checkSamePrefix(espurna::StringView key) {
    return key.startsWith(Prefix);
}

espurna::settings::query::Result findFrom(StringView key) {
    return espurna::settings::query::findFrom(Settings, key);
}

void setup() {
    ::settingsRegisterQueryHandler({
        .check = checkSamePrefix,
        .get = findFrom,
    });
}

} // namespace query
} // namespace settings

// Output is supposed to be used as both part of the MQTT config topic and the `uniq_id` field
// TODO: manage UTF8 strings? in case we somehow receive `desc`, like it was done originally

String normalize_ascii(String value, bool lower) {
    for (auto ptr = value.begin(); ptr != value.end(); ++ptr) {
        switch (*ptr) {
        case '\0':
            goto done;
        case '0' ... '9':
        case 'a' ... 'z':
            break;
        case 'A' ... 'Z':
            if (lower) {
                *ptr = (*ptr + 32);
            }
            break;
        default:
            *ptr = '_';
            break;
        }
    }

done:
    return value;
}

String normalize_ascii(StringView value, bool lower) {
    return normalize_ascii(String(value), lower);
}

// Common data used across the discovery payloads.
// ref. https://developers.home-assistant.io/docs/entity_registry_index/

// 'runtime' strings, may be changed in settings
struct ConfigStrings {
    String name;
    String identifier;
    String prefix;
};

using ConfigStringsPtr = std::unique_ptr<ConfigStrings>;

ConfigStringsPtr make_config_strings() {
    return ConfigStringsPtr(
        new ConfigStrings{
            .name = normalize_ascii(systemHostname(), false),
            .identifier = normalize_ascii(systemIdentifier(), true),
            .prefix = settings::prefix(),
        });
}

// 'build-time' strings, always the same for current build
struct BuildStrings {
    String version;
    String manufacturer;
    String device;
};

using BuildStringsPtr = std::unique_ptr<BuildStrings>;

BuildStringsPtr make_build_strings() {
    const auto app = buildApp();
    const auto hardware = buildHardware();
    return BuildStringsPtr(
        new BuildStrings{
            .version = app.version.toString(),
            .manufacturer = hardware.manufacturer.toString(),
            .device = hardware.device.toString(),
        });
}

class Device {
public:
    // XXX: take care when adding / removing keys and values below
    // - `const char*` is copied by pointer value, persistent pointers make sure
    //   it is valid for the duration of this objects lifetime
    // - `F(...)` aka `__FlashStringHelpe` **will take more space**
    //   it is **copied inside of the buffer**, and will take `strlen()` bytes
    // - allocating more objects **will silently corrupt** buffer region
    //   while there are *some* checks, current version is going to break
    static constexpr size_t BufferSize { JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(6) };

    using Buffer = StaticJsonBuffer<BufferSize>;
    using BufferPtr = std::unique_ptr<Buffer>;

    Device() = delete;

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;

    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    Device(ConfigStringsPtr config, BuildStringsPtr build) :
        _config(std::move(config)),
        _build(std::move(build)),
        _buffer(std::make_unique<Buffer>()),
        _root(_buffer->createObject())
    {
        _root["name"] = _config->name.c_str();

        auto& ids = _root.createNestedArray("ids");
        ids.add(_config->identifier.c_str());

        _root["sw"] = _build->version.c_str();
        _root["mf"] = _build->manufacturer.c_str();
        _root["mdl"] = _build->device.c_str();
    }

    const String& name() const {
        return _config->name;
    }

    const String& prefix() const {
        return _config->prefix;
    }

    const String& identifier() const {
        return _config->identifier;
    }

    JsonObject& root() {
        return _root;
    }

private:
    ConfigStringsPtr _config;
    BuildStringsPtr _build;

    BufferPtr _buffer;
    JsonObject& _root;
};

using DevicePtr = std::unique_ptr<Device>;
using JsonBufferPtr = std::unique_ptr<DynamicJsonBuffer>;

class Context {
public:
    Context() = delete;
    Context(DevicePtr device, size_t capacity) :
        _device(std::move(device)),
        _capacity(capacity)
    {}

    const String& name() const {
        return _device->name();
    }

    const String& prefix() const {
        return _device->prefix();
    }

    const String& identifier() const {
        return _device->identifier();
    }

    JsonObject& device() {
        return _device->root();
    }

    void reset() {
        _json = std::make_unique<DynamicJsonBuffer>(_capacity);
    }

    size_t capacity() const {
        return _capacity;
    }

    size_t size() {
        if (_json) {
            return _json->size();
        }

        return 0;
    }

    JsonObject& makeObject() {
        if (!_json) {
            reset();
        }

        return _json->createObject();
    }

private:
    String _prefix;
    DevicePtr _device;

    JsonBufferPtr _json { nullptr };
    size_t _capacity { 0ul };
};

String quote(String&& value) {
    if (value.equalsIgnoreCase("y")
        || value.equalsIgnoreCase("n")
        || value.equalsIgnoreCase("yes")
        || value.equalsIgnoreCase("no")
        || value.equalsIgnoreCase("true")
        || value.equalsIgnoreCase("false")
        || value.equalsIgnoreCase("on")
        || value.equalsIgnoreCase("off"))
    {
        String result;
        result.reserve(value.length() + 2);
        result += '"';
        result += value;
        result += '"';
        return result;
    }

    return std::move(value);
}

// - Discovery object is expected to accept Context reference as input
//   (and all implementations do just that)
// - topic() & message() return refs, since those *may* be called multiple times before advancing to the next 'entity'
// - We use short-hand names right away, since we don't expect this to be used to generate yaml
// - In case the object uses the JSON makeObject() as state, make sure we don't use it (state)
//   and the object itself after next() or ok() return false
// - Make sure JSON state is not created on construction, but lazy-loaded as soon as it is needed.
//   Meaning, we don't cause invalid refs immediatly when there are more than 1 discovery object present and we reset the storage.

class Discovery {
public:
    virtual ~Discovery();

    virtual bool ok() const = 0;

    virtual const String& topic() = 0;
    virtual const String& message() = 0;

    virtual bool prepare();
    virtual bool ready() const;
    virtual bool next() = 0;
};

Discovery::~Discovery() = default;

bool Discovery::prepare() {
    return true;
}

bool Discovery::ready() const {
    return true;
}

#if RELAY_SUPPORT

class RelayDiscovery : public Discovery {
public:
    explicit RelayDiscovery(Context& ctx) :
        _ctx(ctx)
    {}

    JsonObject& root() {
        if (!_root) {
            _root = &_ctx.makeObject();
        }

        return *_root;
    }

    bool ready() const override {
        return _ready;
    }

    bool ok() const override {
        return _ready
            && (_count > 0)
            && (_index < _count);
    }

    const String& uniqueId() {
        if (!_unique_id.length()) {
            _unique_id = _ctx.identifier() + '_' + F("relay") + '_' + String(_index, 10);
        }

        return _unique_id;
    }

    const String& topic() override {
        if (!_topic.length()) {
            _topic = _ctx.prefix();
            _topic += F("/switch/");
            _topic += uniqueId();
            _topic += F("/config");
        }

        return _topic;
    }

    const String& message() override {
        if (!_message.length()) {
            auto& json = root();
            json[F("dev")] = _ctx.device();
            json[F("avty_t")] = _info->availability.c_str();
            json[F("pl_avail")] = _info->payload_available.c_str();
            json[F("pl_not_avail")] = _info->payload_not_available.c_str();
            json[F("pl_on")] = _info->payload_on.c_str();
            json[F("pl_off")] = _info->payload_off.c_str();
            json[F("uniq_id")] = uniqueId();
            json[F("name")] = _ctx.name() + ' ' + String(_index, 10);
            json[F("stat_t")] = mqttTopic(MQTT_TOPIC_RELAY, _index);
            json[F("cmd_t")] = mqttTopicSetter(MQTT_TOPIC_RELAY, _index);
            json.printTo(_message);
        }

        return _message;
    }

    bool prepare() override {
        if (!_ready) {
            _count = relayCount();
            _index = 0;
            _info = _makeInfo();
            _ready = true;
        }

        return _ready;
    }

    bool next() override {
        if (_index < _count) {
            auto current = _index;
            ++_index;
            if ((_index > current) && (_index < _count)) {
                _unique_id = "";
                _topic = "";
                _message = "";
                return true;
            }
        }

        return false;
    }

private:
    struct Info {
        String availability;
        String payload_available;
        String payload_not_available;
        String payload_on;
        String payload_off;
    };

    using InfoPtr = std::unique_ptr<Info>;
    InfoPtr _makeInfo();

    Context& _ctx;
    JsonObject* _root { nullptr };

    InfoPtr _info;
    size_t _index;
    size_t _count;

    bool _ready { false };

    String _unique_id;
    String _topic;
    String _message;
};

RelayDiscovery::InfoPtr RelayDiscovery::_makeInfo() {
    return InfoPtr(
        new Info{
            .availability = mqttTopic(MQTT_TOPIC_STATUS),
            .payload_available = quote(mqttPayloadStatus(true)),
            .payload_not_available = quote(mqttPayloadStatus(false)),
            .payload_on = quote(relayPayload(PayloadStatus::On).toString()),
            .payload_off = quote(relayPayload(PayloadStatus::Off).toString()),
        });
}

#endif

// Example payload:
// {
//  "state": "ON",
//  "brightness": 255,
//  "color_mode": "rgb",
//  "color": {
//    "r": 255,
//    "g": 180,
//    "b": 200,
//  },
//  "transition": 2,
// }

// Notice that we only support JSON schema payloads, leaving it to the user to configure special
// per-channel topics, as those don't really fit into the HASS idea of lights controls for a single device

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

static constexpr char Topic[] = MQTT_TOPIC_LIGHT_JSON;

class LightDiscovery : public Discovery {
public:
    explicit LightDiscovery(Context& ctx) :
        _ctx(ctx)
    {}

    JsonObject& root() {
        if (!_root) {
            _root = &_ctx.makeObject();
        }

        return *_root;
    }

    bool ready() const override {
        return _ready;
    }

    bool prepare() override {
        if (!_ready) {
            _count = lightChannels();
            _ready = true;
        }

        return _ready;
    }

    bool ok() const override {
        return _ready && (_count > 0);
    }

    bool next() override {
        return false;
    }

    const String& uniqueId() {
        if (!_unique_id.length()) {
            _unique_id = _ctx.identifier() + '_' + F("light");
        }

        return _unique_id;
    }

    const String& topic() override {
        if (!_topic.length()) {
            _topic = _ctx.prefix();
            _topic += F("/light/");
            _topic += uniqueId();
            _topic += F("/config");
        }

        return _topic;
    }

    const String& message() override {
        if (!_message.length()) {
            auto& json = root();

            json[F("schema")] = "json";
            json[F("uniq_id")] = uniqueId();

            json[F("name")] = _ctx.name() + ' ' + F("Light");

            json[F("stat_t")] = mqttTopic(Topic);
            json[F("cmd_t")] = mqttTopicSetter(Topic);

            json[F("avty_t")] = mqttTopic(MQTT_TOPIC_STATUS);
            json[F("pl_avail")] = quote(mqttPayloadStatus(true));
            json[F("pl_not_avail")] = quote(mqttPayloadStatus(false));

            // Note that since we send back the values immediately, HS mode sliders
            // *will jump*, as calculations of input do not always match the output.
            // (especially, when gamma table is used, as we modify the results)
            // In case or RGB, channel values input is expected to match the output exactly.

            // Since 2022.9.x we have a different payload setup
            // - https://github.com/xoseperez/espurna/issues/2539
            // - https://github.com/home-assistant/core/blob/2022.9.7/homeassistant/components/mqtt/light/schema_json.py
            // * ignore 'onoff' and 'brightness'
            //   both described as 'Must be the only supported mode'
            // * 'hs' is always supported, but HA UI depends on our setting and
            //   what gets sent in the json payload
            // * 'c' and 'w' mean different things depending on *our* context
            //   'rgbw' - we receive and map to 'w' to our 'warm'
            //   'rgbww' - we receive and map 'c' to our 'cold' and 'w' to our 'warm'
            //   'cw' / 'ww' without 'rgb' are not supported; see 'brightness' or 'color_temp'
            json["brightness"] = true;
            JsonArray& modes = json.createNestedArray("supported_color_modes");

            if (lightHasColor()) {
                modes.add("hs");
                modes.add("rgb");
                if (lightHasWarmWhite() && lightHasColdWhite()) {
                    modes.add("rgbww");
                } else if (lightHasWarmWhite()) {
                    modes.add("rgbw");
                }
            }

            // Mired is only an input, we never send this value back
            // (...besides the internally pinned value, ref. MQTT_TOPIC_MIRED. not used here though)
            // - in RGB mode, we convert the temperature into a specific color
            // - in CCT mode, white channels are used
            if (lightHasColor() || lightHasWhite()) {
                const auto range = lightMiredsRange();
                json["min_mirs"] = range.cold();
                json["max_mirs"] = range.warm();
                modes.add("color_temp");
                modes.add("white");
            }

            if (!modes.size()) {
                modes.add("brightness");
            }

            json.printTo(_message);
        }

        return _message;
    }

private:
    Context& _ctx;
    JsonObject* _root { nullptr };

    bool _ready { false };
    size_t _count;

    String _unique_id;
    String _topic;
    String _message;
};

void heartbeat_rgb(JsonObject& root, JsonObject& color) {
    const auto rgb = lightRgb();

    color["r"] = rgb.red();
    color["g"] = rgb.green();
    color["b"] = rgb.blue();

    if (lightHasWarmWhite() && lightHasColdWhite()) {
        root["color_mode"] = "rgbww";
        color["c"] = lightColdWhite();
        color["w"] = lightWarmWhite();
    } else if (lightHasWarmWhite()) {
        root["color_mode"] = "rgbw";
        color["w"] = lightWarmWhite();
    } else {
        root["color_mode"] = "rgb";
    }
}

void heartbeat_hsv(JsonObject& root, JsonObject& color) {
    root["color_mode"] = "hs";

    const auto hsv = lightHsv();
    color["h"] = hsv.hue();
    color["s"] = hsv.saturation();
}

bool heartbeat(heartbeat::Mask mask) {
    // TODO: mask json payload specifically?
    // or, find a way to detach masking from the system setting / don't use heartbeat timer
    if (mask & heartbeat::Report::Light) {
        DynamicJsonBuffer buffer(512);
        JsonObject& root = buffer.createObject();

        const auto state = lightState();
        root["state"] = state ? "ON" : "OFF";

        if (state) {
            root["brightness"] = lightBrightness();
            if (lightHasColor() && lightColor()) {
                auto& color = root.createNestedObject("color");
                if (lightUseRGB()) {
                    heartbeat_rgb(root, color);
                } else {
                    heartbeat_hsv(root, color);
                }
            }
        }

        String message;
        root.printTo(message);

        static const auto topic = StringView(Topic).toString();
        mqttSendRaw(
            mqttTopic(topic).c_str(),
            message.c_str(), false);
    }

    return true;
}

void publishLightJson() {
    heartbeat(static_cast<heartbeat::Mask>(heartbeat::Report::Light));
}

void receiveLightJson(StringView payload) {
    DynamicJsonBuffer buffer(1024);
    JsonObject& root = buffer.parseObject(payload.begin());
    if (!root.success()) {
        return;
    }

    if (!root.containsKey("state")) {
        return;
    }

    const auto state = root["state"].as<String>();

    if (StringView("ON") == state) {
        lightState(true);
    } else if (StringView("OFF") == state) {
        lightState(false);
    } else {
        return;
    }

    auto transition = lightTransitionTime();
    if (root.containsKey("transition")) {
        using LocalUnit = decltype(lightTransitionTime());
        using RemoteUnit = std::chrono::duration<float>;
        auto seconds = RemoteUnit(root["transition"].as<float>());

        if (seconds.count() > 0.0f) {
            transition = std::chrono::duration_cast<LocalUnit>(seconds);
        }
    }

    if (root.containsKey("color_temp")) {
        const auto mireds = root["color_temp"].as<long>();
        lightTemperature(light::Mireds{ .value = mireds });
    }

    if (root.containsKey("brightness")) {
        lightBrightness(root["brightness"].as<long>());
    }

    if (lightHasColor() && root.containsKey("color")) {
        JsonObject& color = root["color"];
        if (color.containsKey("h")
         && color.containsKey("s"))
        {
            lightHs(
                color["h"].as<long>(),
                color["s"].as<long>());
        } else if (color.containsKey("r")
                && color.containsKey("g")
                && color.containsKey("b"))
        {
            lightRgb({
                color["r"].as<long>(),
                color["g"].as<long>(),
                color["b"].as<long>()});
        }

        if (color.containsKey("w")) {
            lightWarmWhite(color["w"].as<long>());
        }

        if (color.containsKey("c")) {
            lightColdWhite(color["c"].as<long>());
        }
    }

    lightUpdate({transition, lightTransitionStep()});
}

#endif

#if SENSOR_SUPPORT

class SensorDiscovery : public Discovery {
public:
    explicit SensorDiscovery(Context& ctx) :
        _ctx(ctx)
    {}

    JsonObject& root() {
        if (!_root) {
            _root = &_ctx.makeObject();
        }

        return *_root;
    }

    bool ready() const override {
        return _ready;
    }

    bool ok() const override {
        return _ready
            && (_count > 0)
            && (_index < _count);
    }

    const String& topic() override {
        if (!_topic.length()) {
            _topic = _ctx.prefix();
            _topic += F("/sensor/");
            _topic += uniqueId();
            _topic += F("/config");
        }

        return _topic;
    }

    const String& message() override {
        if (!_message.length()) {
            auto& json = root();
            json[F("dev")] = _ctx.device();
            json[F("uniq_id")] = uniqueId();

            json[F("name")] = _ctx.name() + ' ' + name() + ' ' + localId();
            json[F("stat_t")] = mqttTopic(_info.topic);
            json[F("unit_of_meas")] = magnitudeUnitsName(_info.units);

            json.printTo(_message);
        }

        return _message;
    }

    const String& name() {
        if (!_name.length()) {
            _name = magnitudeTypeTopic(_info.type);
        }

        return _name;
    }

    String localId() const {
        return String(_info.index, 10);
    }

    const String& uniqueId() {
        if (!_unique_id.length()) {
            _unique_id = _ctx.identifier() + '_' + name() + '_' + localId();
        }

        return _unique_id;
    }

    bool prepare() override {
        if (!_ready) {
            _ready = sensorReady();
            if (!_ready) {
                return false;
            }

            _count = magnitudeCount();
            _index = 0;

            if (_count > 0) {
                _info = magnitudeInfo(_index);
            }
        }

        return _ready;
    }

    bool next() override {
        if (_index < _count) {
            auto current = _index;
            ++_index;
            if ((_index > current) && (_index < _count)) {
                _info = magnitudeInfo(_index);
                _unique_id = "";
                _name = "";
                _topic = "";
                _message = "";
                return true;
            }
        }

        return false;
    }

private:
    Context& _ctx;
    JsonObject* _root { nullptr };

    sensor::Info _info;
    size_t _count;
    size_t _index;
    bool _ready { false };

    String _unique_id;
    String _name;
    String _topic;
    String _message;
};

#endif

DevicePtr make_device_ptr() {
    return std::make_unique<Device>(
        make_config_strings(),
        make_build_strings());
}

Context make_context() {
    return Context(make_device_ptr(), 2048);
}

// use 5 retries and set a specific duration for each attempt
using Durations = std::array<duration::Milliseconds, 5>;

#if __cplusplus >= 201703L
#define __CONSTEXPR constexpr
#else
#define __CONSTEXPR
#endif

struct Wait {
    using Value = typename Durations::value_type;

    __CONSTEXPR Wait(const Durations& base) :
        _begin(std::begin(base)),
        _end(std::end(base)),
        _it(_begin)
    {}

    void reset() {
        _it = _begin;
    }

    Value value() const {
        return *_it;
    }

    void change_next() {
        _it = next_it(_it);
    }

    bool try_next() {
        auto current = _it;
        change_next();
        return current != _it;
    }

    Value next() {
        change_next();
        return value();
    }

    __CONSTEXPR Value first() const {
        return *_begin;
    }

    bool is_first() const {
        return _it == _begin;
    }

    __CONSTEXPR Value last() const {
        return *_end;
    }

    bool is_last() const {
        return _it == _end;
    }

    __CONSTEXPR size_t count() const {
        return _end - _begin;
    }

private:
    using Iterator = Durations::const_iterator;

    Iterator next_it(Iterator value) {
        const auto next = value + 1;
        if (next < _end) {
            return next;
        }

        return value;
    }

    Iterator _begin;
    Iterator _end;
    Iterator _it;
};

#undef __CONSTEXPR

// intervals between send attempts, usually long enough to push data to the network stack
static constexpr Durations ShortDurations{{
    duration::Milliseconds{ 100 },
    duration::Milliseconds{ 500 },
    duration::Milliseconds{ 1000 },
    duration::Milliseconds{ 2500 },
    duration::Milliseconds{ 5000 },
}};

// longer intervals between initialization attempts, give enough time for external things
static constexpr Durations LongDurations{{
    duration::Seconds{ 5 },
    duration::Seconds{ 10 },
    duration::Seconds{ 15 },
    duration::Seconds{ 30 },
    duration::Seconds{ 60 },
}};

struct Result {
    using Duration = duration::Milliseconds;

    enum class Value {
        Error,
        Ok,
        Retry,
    };

    Result(Duration wait, Value value) :
        _wait(wait),
        _value(value)
    {}

    explicit Result(Duration wait) :
        Result(wait, Value::Ok)
    {}

    Result() :
        Result(ShortDurations.front())
    {}

    bool ok() const {
        return _value == Value::Ok;
    }

    bool retry() const {
        return _value == Value::Retry;
    }

    explicit operator bool() const {
        return ok();
    }

    Duration wait() const {
        return _wait;
    }

private:
    Duration _wait;
    Value _value;
};

Result next_retry(Wait& wait) {
    const auto retry = wait.is_last()
        ? Result::Value::Error
        : Result::Value::Retry;

    const auto value = wait.value();
    wait.try_next();

    return Result(value, retry);
}

// Topic and message are generated on demand and most of JSON payload is cached for re-use to save RAM.
class DiscoveryTask {
public:
    using Entity = std::unique_ptr<Discovery>;
    using Entities = std::forward_list<Entity>;

    DiscoveryTask() = delete;

    DiscoveryTask(const DiscoveryTask&) = delete;
    DiscoveryTask& operator=(const DiscoveryTask&) = delete;

    DiscoveryTask(DiscoveryTask&&) = delete;
    DiscoveryTask& operator=(DiscoveryTask&&) = delete;

    DiscoveryTask(Context ctx, State state) :
        _ctx(std::move(ctx)),
        _state(state)
    {}

    void add(Entity&& entity) {
        _entities.push_front(std::move(entity));
    }

    template <typename T>
    void add() {
        _entities.push_front(std::make_unique<T>(_ctx));
    }

    Result retry_send() {
        return next_retry(_wait_short);
    }

    Context& context() {
        return _ctx;
    }

    bool done() const {
        return _entities.empty();
    }

    State state() const {
        return _state;
    }

    template <typename T>
    Result try_send_one(T&& action);

    Result prepare_all();

private:
    Result next_send() {
        _wait_short.reset();
        return Result(_wait_short.value());
    }

    Result stop_sending() {
        return Result(
            _wait_short.first(),
            Result::Value::Error);
    }

    Context _ctx;

    State _state;
    Entities _entities;

    Wait _wait_short { ShortDurations };
    Wait _wait_long { LongDurations };
};

Result DiscoveryTask::prepare_all() {
    bool prepared { true };
    for (auto& entity : _entities) {
        if (!entity->prepare()) {
            prepared = false;
            break;
        }
    }

    if (!prepared) {
        return next_retry(_wait_long);
    }

    return Result();
}

template <typename T>
Result DiscoveryTask::try_send_one(T&& action) {
    auto it = _entities.begin();

    while (it != _entities.end()) {
        if (!(*it)->ok()) {
            it = _entities.erase_after(
                _entities.before_begin());
            _ctx.reset();
            continue;
        }

        const auto* topic = (*it)->topic().c_str();
        const auto* msg = (State::Enabled == _state)
            ? (*it)->message().c_str()
            : "";

        if (action(topic, msg)) {
            if (!(*it)->next()) {
                it = _entities.erase_after(
                    _entities.before_begin());
                _ctx.reset();
            }

            return next_send();
        }

        return retry_send();
    }

    return stop_sending();
}

using DiscoveryPtr = std::shared_ptr<DiscoveryTask>;
using FlagPtr = std::shared_ptr<bool>;

DiscoveryPtr makeDiscovery(State);

namespace internal {

bool enabled { build::enabled() };
bool retain { build::retain() };

String birthTopic;

timer::SystemTimer task;

void send(DiscoveryPtr, FlagPtr);

void schedule(duration::Milliseconds wait, DiscoveryPtr ptr, FlagPtr flag_ptr) {
    task.schedule_once(
        wait,
        [ptr, flag_ptr]() {
            send(ptr, flag_ptr);
        });
}

void stop() {
    DEBUG_MSG_P(PSTR("[HA] Stopping discovery\n"));
    internal::task.stop();
}

void send(DiscoveryPtr discovery, FlagPtr flag_ptr) {
    if (!mqttConnected() || discovery->done()) {
        stop();
        return;
    }

    auto ready = discovery->prepare_all();
    if (!ready) {
        if (ready.retry()) {
            DEBUG_MSG_P(PSTR("[HA] Discovery not ready, retrying in %zu (ms)\n"),
                ready.wait().count());
            schedule(ready.wait(), discovery, flag_ptr);
        } else {
            stop();
        }

        return;
    }

    auto& flag = *flag_ptr;
    if (!flag) {
        const auto next_send = discovery->retry_send();
        if (next_send.retry()) {
            schedule(next_send.wait(), discovery, flag_ptr);
        } else {
            stop();
        }
        return;
    }

    uint16_t pid { 0u };
    const auto sent = discovery->try_send_one(
        [&](const char* topic, const char* message) {
            pid = ::mqttSendRaw(topic, message, internal::retain, 1);
            return pid > 0;
        });

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
    // Receive acknowledgement from the broker before continuing.
    // Usually a good idea in general, to avoid filling network buffers too quickly.
    //
    // Not needed with LWMQTT, as it is already handled and wrapped in Result
    // Not supported by PubSubClient

    if (sent) {
        flag = false;
        mqttOnPublish(
            pid,
            [flag_ptr]() {
                (*flag_ptr) = true;
            });
    }
#endif

    if (sent.ok() || sent.retry()) {
        schedule(sent.wait(), discovery, flag_ptr);
        return;
    }

    if (!sent) {
        stop();
    }
}

} // namespace internal

DiscoveryPtr makeDiscovery(State state) {
    auto discovery = std::make_shared<DiscoveryTask>(
        make_context(), state);

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    discovery->add<LightDiscovery>();
#endif
#if RELAY_SUPPORT
    discovery->add<RelayDiscovery>();
#endif
#if SENSOR_SUPPORT
    discovery->add<SensorDiscovery>();
#endif

    return discovery;
}

void scheduleDiscovery(duration::Milliseconds duration, DiscoveryPtr discovery) {
    DEBUG_MSG_P(PSTR("[HA] Starting discovery\n"));
    internal::schedule(duration, discovery, std::make_shared<bool>(true));
}

void scheduleDiscovery(DiscoveryPtr discovery) {
    scheduleDiscovery(ShortDurations.front(), discovery);
}

void publishDiscoveryForState(State state) {
    if (!mqttConnected()) {
        return;
    }

    auto discovery = makeDiscovery(state);

    // only happens when nothing is configured to do the add()
    if (discovery->done()) {
        DEBUG_MSG_P(PSTR("[HA] No discovery task(s) available\n"));
        return;
    }

    scheduleDiscovery(discovery);
}

void publishDiscoveryForState(bool state) {
    publishDiscoveryForState(
        state
            ? State::Enabled
            : State::Disabled);
}

void publishDiscoveryForCurrentState() {
    publishDiscoveryForState(internal::enabled);
}

void configure() {
    auto birthTopic = settings::birthTopic();
    const auto birthChanged = birthTopic != internal::birthTopic;
    if (mqttConnected() && birthChanged) {
        if (internal::birthTopic.length()) {
            mqttUnsubscribeRaw(internal::birthTopic.c_str());
        }
        if (birthTopic.length()) {
            mqttSubscribeRaw(birthTopic.c_str());
        }
    }

    if (birthChanged) {
        internal::birthTopic = std::move(birthTopic);
    }

    internal::retain = settings::retain();

    const auto current = internal::enabled;
    internal::enabled = settings::enabled();

    if (mqttConnected() && (current != internal::enabled)) {
        publishDiscoveryForState(current);
    }
}

namespace mqtt {

void onDisconnected() {
    internal::task.stop();
}

void onConnected() {
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    ::mqttSubscribe(Topic);
#endif
    ::espurnaRegisterOnce(publishDiscoveryForCurrentState);
    if (internal::birthTopic.length()) {
        ::mqttSubscribeRaw(internal::birthTopic.c_str());
    }
}

void onMessage(StringView topic, StringView payload) {
#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    auto t = ::mqttMagnitude(topic);
    if (t.equals(Topic)) {
        receiveLightJson(payload);
        return;
    }
#endif

    if ((topic == internal::birthTopic)
     && (payload == settings::birthPayload()))
    {
        publishDiscoveryForCurrentState();
    }
}

void callback(unsigned int type, StringView topic, StringView payload) {
    if (MQTT_DISCONNECT_EVENT == type) {
        onDisconnected();
        return;
    }

    if (MQTT_CONNECT_EVENT == type) {
        onConnected();
        return;
    }

    if (type == MQTT_MESSAGE_EVENT) {
        onMessage(topic, payload);
        return;
    }
}

} // namespace mqtt

namespace web {

#if WEB_SUPPORT

void onAction(uint32_t, const char* action, JsonObject& data) {
    STRING_VIEW_INLINE(Publish, "ha-publish");
    STRING_VIEW_INLINE(State, "state");

    if ((Publish == action) && data.containsKey(State)) {
        publishDiscoveryForState(data[State].as<bool>());
        return;
    }
}

void onVisible(JsonObject& root) {
    wsPayloadModule(root, settings::query::Prefix);
}

void onConnected(JsonObject& root) {
    root[settings::keys::Enabled] = settings::enabled();
    root[settings::keys::Prefix] = settings::prefix();
    root[settings::keys::Retain] = settings::retain();
    root[settings::keys::BirthTopic] = settings::birthTopic();
    root[settings::keys::BirthPayload] = settings::birthPayload();
}

bool onKeyCheck(StringView key, const JsonVariant&) {
    return settings::query::checkSamePrefix(key);
}

#endif

} // namespace web

#if TERMINAL_SUPPORT
namespace terminal {

STRING_VIEW_INLINE(Dump, "HA");

void dump(::terminal::CommandContext&& ctx) {
    settingsDump(ctx, settings::query::Settings);
}

STRING_VIEW_INLINE(Send, "HA.SEND");

void send(::terminal::CommandContext&& ctx) {
    publishDiscoveryForState(State::Enabled);
    terminalOK(ctx);
}

STRING_VIEW_INLINE(Clear, "HA.CLEAR");

void clear(::terminal::CommandContext&& ctx) {
    publishDiscoveryForState(State::Disabled);
    terminalOK(ctx);
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Dump, dump},
    {Clear, clear},
    {Send, send},
};

void setup() {
    espurna::terminal::add(Commands);
}

} // namespace terminal
#endif

void setup() {
#if WEB_SUPPORT
    wsRegister()
        .onAction(web::onAction)
        .onVisible(web::onVisible)
        .onConnected(web::onConnected)
        .onKeyCheck(web::onKeyCheck);
#endif

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    lightOnReport(publishLightJson);
    mqttHeartbeat(heartbeat);
#endif
    mqttRegister(mqtt::callback);

#if TERMINAL_SUPPORT
    terminal::setup();
#endif

    settings::query::setup();

    espurnaRegisterReload(configure);
    configure();
}

} // namespace
} // namespace homeassistant
} // namespace espurna

// This module no longer implements .yaml generation, since we can't:
// - use unique_id in the device config
// - have abbreviated keys
// - have mqtt reliably return the correct status & command payloads when it is disabled
//   (yet? needs reworked configuration section or making functions read settings directly)

void haSetup() {
    espurna::homeassistant::setup();
}

#endif // HOMEASSISTANT_SUPPORT
