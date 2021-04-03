/*

LIGHT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "light.h"

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#include "api.h"
#include "mqtt.h"
#include "relay.h"
#include "rpc.h"
#include "rtcmem.h"
#include "ws.h"
#include "libs/OnceFlag.h"

#include <Ticker.h>
#include <Schedule.h>
#include <ArduinoJson.h>

#include <array>
#include <cstring>
#include <vector>

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
#include <my92xx.h>
#endif

extern "C" {
#include "libs/fs_math.h"
}

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

// default is 8, we only need up to 5
#define PWM_CHANNEL_NUM_MAX Light::ChannelsMax
extern "C" {
#include "libs/pwm.h"
}

#endif

#include "light_config.h"

// -----------------------------------------------------------------------------

namespace Light {

constexpr long Rgb::Min;
constexpr long Rgb::Max;

constexpr long Hsv::HueMin;
constexpr long Hsv::HueMax;

constexpr long Hsv::SaturationMin;
constexpr long Hsv::SaturationMax;

constexpr long Hsv::ValueMin;
constexpr long Hsv::ValueMax;

}

// -----------------------------------------------------------------------------

#if RELAY_SUPPORT

// Setup virtual relays contolling the light's state
// TODO: only do per-channel setup optionally

class LightChannelProvider : public RelayProviderBase {
public:
    LightChannelProvider() = delete;
    explicit LightChannelProvider(size_t id) :
        _id(id)
    {}

    const char* id() const {
        return "light_channel";
    }

    void change(bool status) override {
        lightState(_id, status);
        lightState(true);
        lightUpdate();
    }

private:
    size_t _id { RelaysMax };
};

class LightGlobalProvider : public RelayProviderBase {
public:
    const char* id() const {
        return "light_global";
    }

    void change(bool status) override {
        lightState(status);
        lightUpdate();
    }
};

#endif

struct channel_t {
    channel_t() = default;

    // TODO: set & store pin in the provider
    explicit channel_t(unsigned char pin_, bool inverse_, bool gamma_) :
        pin(pin_),
        inverse(inverse_),
        gamma(gamma_)
    {
        pinMode(pin, OUTPUT);
    }

    explicit channel_t(unsigned char pin_) :
        pin(pin_)
    {
        pinMode(pin, OUTPUT);
    }

    unsigned char pin { GPIO_NONE };    // real GPIO pin
    bool inverse { false };             // re-map the value from [ValueMin:ValueMax] to [ValueMax:ValueMin]
    bool gamma { false };               // apply gamma correction to the target value

    bool state { true };                // is the channel ON

    unsigned char inputValue { Light::ValueMin };   // raw, without the brightness
    unsigned char value { Light::ValueMin };        // normalized, including brightness
    unsigned char target { Light::ValueMin };       // resulting value that will be given to the provider
    float current { Light::ValueMin };              // interim between input and target, used by the transition handler
};

std::vector<channel_t> _light_channels;

namespace Light {

struct Mapping {
    struct Pointers {
        Pointers() = default;
        Pointers(const Pointers&) = default;
        Pointers(Pointers&&) = default;

        Pointers& operator=(const Pointers&) = default;
        Pointers& operator=(Pointers&&) = default;

        Pointers(channel_t* red, channel_t* green, channel_t* blue, channel_t* cold, channel_t* warm) :
            _red(red),
            _green(green),
            _blue(blue),
            _cold(cold),
            _warm(warm)
        {}

        channel_t* red() {
            return _red;
        }

        channel_t* green() {
            return _green;
        }

        channel_t* blue() {
            return _blue;
        }

        channel_t* cold() {
            return _cold;
        }

        channel_t* warm() {
            return _warm;
        }

    private:
        channel_t* _red { nullptr };
        channel_t* _green { nullptr };
        channel_t* _blue { nullptr };
        channel_t* _cold { nullptr };
        channel_t* _warm { nullptr };
    };

    void reset() {
        _pointers = Pointers();
    }

    template <typename ...Args>
    void update(Args... args) {
        _pointers = Pointers(std::forward<Args>(args)...);
    }

    long get(channel_t* ptr) {
        if (ptr) {
            return ptr->target;
        }

        return 0l;
    }

    void set(channel_t* ptr, long value) {
        if (ptr) {
            ptr->inputValue = std::clamp(value, Light::ValueMin, Light::ValueMax);
        }
    }

    long red() {
        return get(_pointers.red());
    }

    void red(long value) {
        set(_pointers.red(), value);
    }

    long green() {
        return get(_pointers.green());
    }

    void green(long value) {
        set(_pointers.green(), value);
    }

    long blue() {
        return get(_pointers.blue());
    }

    void blue(long value) {
        set(_pointers.blue(), value);
    }

    long cold() {
        return get(_pointers.cold());
    }

    void cold(long value) {
        set(_pointers.cold(), value);
    }

    long warm() {
        return get(_pointers.warm());
    }

    void warm(long value) {
        set(_pointers.warm(), value);
    }

private:
    Pointers _pointers;
};

} // namespace Light

Light::Mapping _light_mapping;

void _lightUpdateMapping(size_t channels) {
    switch (channels) {
    case 0:
        break;
    case 1:
        _light_mapping.update(nullptr, nullptr, nullptr, &_light_channels[0], nullptr);
        break;
    case 2:
        _light_mapping.update(nullptr, nullptr, nullptr, &_light_channels[0], &_light_channels[1]);
        break;
    case 3:
        _light_mapping.update(&_light_channels[0], &_light_channels[1], &_light_channels[2], nullptr, nullptr);
        break;
    case 4:
        _light_mapping.update(&_light_channels[0], &_light_channels[1], &_light_channels[2], &_light_channels[3], nullptr);
        break;
    case 5:
        _light_mapping.update(&_light_channels[0], &_light_channels[1], &_light_channels[2], &_light_channels[3], &_light_channels[4]);
        break;
    }
}

bool _light_save = LIGHT_SAVE_ENABLED;
unsigned long _light_save_delay = LIGHT_SAVE_DELAY;
Ticker _light_save_ticker;

unsigned long _light_report_delay = LIGHT_REPORT_DELAY;
Ticker _light_report_ticker;
std::forward_list<LightReportListener> _light_report;

bool _light_has_controls = false;
bool _light_has_color = false;
bool _light_use_rgb = false;
bool _light_use_white = false;
bool _light_use_cct = false;
bool _light_use_gamma = false;

bool _light_state = false;
long _light_brightness = Light::BrightnessMax;

// Default to the Philips Hue value that HA also use.
// https://developers.meethue.com/documentation/core-concepts

// TODO: We only accept this as input, thus setting 'related' channels directly
// will cause the cached mireds value to be used:
// - by brightness function in R G B CW and R G B CW WW as a factor for CW and WW channels
// - by setter in CW and CW WW modes

static_assert(Light::MiredsCold < Light::MiredsWarm, "");

long _light_cold_mireds = Light::MiredsCold;
long _light_warm_mireds = Light::MiredsWarm;

long _light_cold_kelvin = (1000000L / _light_cold_mireds);
long _light_warm_kelvin = (1000000L / _light_warm_mireds);

namespace Light {

constexpr long MiredsDefault { (MiredsCold + MiredsWarm) / 2L };

} // namespace Light

long _light_mireds { Light::MiredsDefault };

namespace {

// In case we somehow forgot to initialize the brightness func, make sure to trigger an exception.
// Just using an `nullptr` may not always trigger an error
// (also, so we also don't have to check whether the pointer is not `nullptr`)

bool _lightApplyBrightnessStub() {
    panic();
    return false;
}

} // namespace

using LightBrightnessFunc = bool(*)();
LightBrightnessFunc _light_brightness_func = _lightApplyBrightnessStub;

bool _light_state_changed = false;
LightStateListener _light_state_listener = nullptr;

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
my92xx* _my92xx { nullptr };
#endif

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
std::unique_ptr<LightProvider> _light_provider;
#endif

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX

namespace settings {
namespace internal {

template <>
my92xx_model_t convert(const String& value) {
    if (value.length() == 1) {
        switch (*value.c_str()) {
        case 0x01:
            return MY92XX_MODEL_MY9291;
        case 0x02:
            return MY92XX_MODEL_MY9231;
        }
    } else {
        if (value == "9291") {
            return MY92XX_MODEL_MY9291;
        } else if (value == "9231") {
            return MY92XX_MODEL_MY9231;
        }
    }

    return Light::build::my92xxModel();
}

} // namespace internal
} // namespace settings

#endif

bool _setValue(size_t, unsigned int) __attribute__((warn_unused_result));
bool _setValue(size_t id, unsigned int value) {
    if (_light_channels[id].value != value) {
        _light_channels[id].value = value;
        return true;
    }

    return false;
}

void _setInputValue(size_t id, long value) {
    _light_channels[id].inputValue = std::clamp(value, Light::ValueMin, Light::ValueMax);
}

void _setRGBInputValue(long red, long green, long blue) {
    _setInputValue(0, red);
    _setInputValue(1, green);
    _setInputValue(2, blue);
}

bool _lightApplyBrightnessChannels(size_t channels) {
    auto scale = static_cast<float>(_light_brightness) / static_cast<float>(Light::BrightnessMax);

    channels = std::min(channels, lightChannels());
    OnceFlag changed;

    for (size_t channel = 0; channel < lightChannels(); ++channel) {
        if (channel >= channels) {
            scale = 1.0f;
        }
        changed = _setValue(channel, _light_channels[channel].inputValue * scale);
    }

    return changed.get();
}

bool _lightApplyBrightnessAll() {
    return _lightApplyBrightnessChannels(lightChannels());
}

bool _lightApplyBrightnessRgb() {
    return _lightApplyBrightnessChannels(3);
}

// Map from normal 153...500 to 0...347, so we get a value 0...1

double _lightMiredFactor() {
    auto cold = static_cast<double>(_light_cold_mireds);
    auto warm = static_cast<double>(_light_warm_mireds);
    auto mireds = static_cast<double>(_light_mireds);

    if (cold < warm) {
        return (mireds - cold) / (warm - cold);
    }

    return 0.0;
}

bool _lightApplyBrightnessColor() {
    OnceFlag changed;

    double brightness = static_cast<double>(_light_brightness) / static_cast<double>(Light::BrightnessMax);

    // Substract the common part from RGB channels and add it to white channel. So [250,150,50] -> [200,100,0,50]
    unsigned char white = std::min({_light_channels[0].inputValue, _light_channels[1].inputValue, _light_channels[2].inputValue});
    for (unsigned int i=0; i < 3; i++) {
        changed = _setValue(i, _light_channels[i].inputValue - white);
    }

    // Split the White Value across 2 White LED Strips.
    if (_light_use_cct) {
        const double factor = _lightMiredFactor();

        _light_channels[3].inputValue = 0;
        changed = _setValue(3, lround((1.0 - factor) * white));

        _light_channels[4].inputValue = 0;
        changed = _setValue(4, lround(factor * white));
    } else {
        _light_channels[3].inputValue = 0;
        changed = _setValue(3, white);
    }

    // Scale up to equal input values. So [250,150,50] -> [200,100,0,50] -> [250, 125, 0, 63]
    unsigned char max_in = std::max({_light_channels[0].inputValue, _light_channels[1].inputValue, _light_channels[2].inputValue});
    unsigned char max_out = std::max({_light_channels[0].value, _light_channels[1].value, _light_channels[2].value, _light_channels[3].value});

    size_t channelSize = _light_use_cct ? 5 : 4;

    if (_light_use_cct) {
        max_out = std::max(max_out, _light_channels[4].value);
    }

    double factor = (max_out > 0) ? (double) (max_in / max_out) : 0;
    for (size_t i = 0; i < channelSize; ++i) {
        changed = _setValue(i, lround((double) _light_channels[i].value * factor * brightness));
    }

    // Scale white channel to match brightness
    for (size_t i = 3; i < channelSize; ++i) {
        changed = _setValue(i, constrain(static_cast<unsigned int>(_light_channels[i].value * LIGHT_WHITE_FACTOR), Light::BrightnessMin, Light::BrightnessMax));
    }

    // For the rest of channels, don't apply brightness, it is already in the inputValue
    // i should be 4 when RGBW and 5 when RGBWW
    for (size_t i = channelSize; i < _light_channels.size(); ++i) {
        changed = _setValue(i, _light_channels[i].inputValue);
    }

    return changed.get();
}

char _lightTag(size_t channels, size_t index) {
    constexpr size_t Columns { 5ul };
    constexpr size_t Rows { 5ul };

    auto row = channels - 1ul;
    if (row < Rows) {
        constexpr char tags[Rows][Columns] = {
            {'W',   0,   0,   0,   0},
            {'W', 'C',   0,   0,   0},
            {'R', 'G', 'B',   0,   0},
            {'R', 'G', 'B', 'W',   0},
            {'R', 'G', 'B', 'W', 'C'},
        };

        return tags[row][index];
    }

    return 0;
}

char _lightTag(size_t index) {
    return _lightTag(_light_channels.size(), index);
}

// UI hint about channel distribution
const char* _lightDesc(size_t channels, size_t index) {
    const __FlashStringHelper* ptr { F("UNKNOWN") };
    switch (_lightTag(channels, index)) {
    case 'W':
        ptr = F("WARM WHITE");
        break;
    case 'C':
        ptr = F("COLD WHITE");
        break;
    case 'R':
        ptr = F("RED");
        break;
    case 'G':
        ptr = F("GREEN");
        break;
    case 'B':
        ptr = F("BLUE");
        break;
    }

    return reinterpret_cast<const char*>(ptr);
}

const char* _lightDesc(size_t index) {
    return _lightDesc(_light_channels.size(), index);
}

// -----------------------------------------------------------------------------
// Input Values
// -----------------------------------------------------------------------------

void _lightFromInteger(unsigned long value, bool brightness) {
    if (brightness) {
        _setRGBInputValue((value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF);
        lightBrightness((value & 0xFF) * Light::BrightnessMax / 255);
    } else {
        _setRGBInputValue((value >> 16) & 0xFF, (value >> 8) & 0xFF, (value) & 0xFF);
    }
}

void _lightFromRgbPayload(const char * rgb) {
    // 9 char #........ , 11 char ...,...,...
    if (!_light_has_color) return;
    if (!rgb || (strlen(rgb) == 0)) return;

    // HEX value is always prefixed, like CSS
    // values are interpreted like RGB + optional brightness
    if (rgb[0] == '#') {
        _lightFromInteger(strtoul(rgb + 1, nullptr, 16), strlen(rgb + 1) > 7);
    // With comma separated string, assume decimal values
    } else {
        const auto channels = _light_channels.size();
        unsigned char count = 0;

        char buf[16] = {0};
        strncpy(buf, rgb, sizeof(buf) - 1);
        char *tok = strtok(buf, ",");
        while (tok != NULL) {
            _setInputValue(count, atoi(tok));
            if (++count == channels) break;
            tok = strtok(NULL, ",");
        }

        // If less than 3 values received, set the rest to 0
        if (count < 2) _setInputValue(1, 0);
        if (count < 3) _setInputValue(2, 0);
        return;
    }
}

// HSV string is expected to be "H,S,V", where:
//   0 <= H <= 360
//   0 <= S <= 100
//   0 <= V <= 100

void _lightFromHsvPayload(const char* hsv) {
    if (!_light_has_color) return;
    if (strlen(hsv) == 0) return;

    char buf[16] = {0};
    strncpy(buf, hsv, sizeof(buf) - 1);

    unsigned char count = 0;
    long values[3] = {0};

    char * tok = strtok(buf, ",");
    while ((count < 3) && (tok != nullptr)) {
        values[count++] = atol(tok);
        tok = strtok(nullptr, ",");
    }

    if (count != 3) {
        return;
    }

    lightHsv({values[0], values[1], values[2]});
}

// Thanks to Sacha Telgenhof for sharing this code in his AiLight library
// https://github.com/stelgenhof/AiLight
// Color temperature is measured in mireds (kelvin = 1e6/mired)
long _toKelvin(const long mireds) {
    return constrain(static_cast<long>(1000000L / mireds), _light_warm_kelvin, _light_cold_kelvin);
}

long _toMireds(const long kelvin) {
    return constrain(static_cast<long>(lround(1000000L / kelvin)), _light_cold_mireds, _light_warm_mireds);
}

void _lightMireds(const long kelvin) {
    _light_mireds = _toMireds(kelvin);
}

void _lightMiredsCCT(const long kelvin) {
    _lightMireds(kelvin);
    const auto factor = _lightMiredFactor();

    auto cold = std::lround(factor * Light::ValueMax);
    auto warm = std::lround((1.0 - factor) * Light::ValueMax);

    _setInputValue(0, cold);
    _setInputValue(1, warm);
}

// TODO: is there a sane way to deduce this back from RGB variant?
// TODO: should mireds require CCT mode, so we only deal with white value?

#if 0

long _lightCCTMireds() {
    auto cold = static_cast<double>(_light_cold_mireds);
    auto warm = static_cast<double>(_light_warm_mireds);

    auto factor = (static_cast<double>(lightColdWhite()) / Light::ValueMax);

    return cold + (factor * (warm - cold));
}

#endif

void _fromKelvin(long kelvin) {

    if (!_light_has_color) {
        if (_light_use_cct) {
            _lightMiredsCCT(kelvin);
        }
        return;
    }

    _lightMireds(kelvin);

    // adjusted by the brightness function
    if (_light_use_cct) {
      _setRGBInputValue(Light::ValueMax, Light::ValueMax, Light::ValueMax);
      return;
    }

    // Calculate color values for the temperature
    kelvin /= 100;
    const unsigned int red = (kelvin <= 66)
        ? Light::ValueMax
        : 329.698727446 * fs_pow((double) (kelvin - 60), -0.1332047592);
    const unsigned int green = (kelvin <= 66)
        ? 99.4708025861 * fs_log(kelvin) - 161.1195681661
        : 288.1221695283 * fs_pow((double) kelvin, -0.0755148492);
    const unsigned int blue = (kelvin >= 66)
        ? Light::ValueMax
        : ((kelvin <= 19)
            ? 0
            : 138.5177312231 * fs_log(kelvin - 10) - 305.0447927307);

    _setRGBInputValue(red, green, blue);

}

void _fromMireds(const long mireds) {
    _fromKelvin(_toKelvin(mireds));
}

// -----------------------------------------------------------------------------
// Output Values
// -----------------------------------------------------------------------------

namespace Light {

unsigned long Rgb::asUlong() const {
    return (_red << 16) | (_green << 8) | _blue;
}

} // namespace Light

Light::Rgb _lightToRgb(bool target) {
    return {
        (target ? _light_channels[0].target : _light_channels[0].inputValue),
        (target ? _light_channels[1].target : _light_channels[1].inputValue),
        (target ? _light_channels[2].target : _light_channels[2].inputValue)};
}

void _lightRgbHexPayload(Light::Rgb rgb, char* out, size_t size) {
    snprintf_P(out, size, PSTR("#%06X"), rgb.asUlong());
}

void _lightRgbHexPayload(char* out, size_t size, bool target = false) {
    _lightRgbHexPayload(_lightToRgb(target), out, size);
}

String _lightRgbHexPayload(bool target) {
    char out[64] { 0 };
    _lightRgbHexPayload(out, sizeof(out), target);
    return out;
}

void _lightHsvPayload(Light::Hsv hsv, char* out, size_t len) {
    snprintf(out, len, "%ld,%ld,%ld", hsv.hue(), hsv.saturation(), hsv.value());
}

void _lightHsvPayload(char* out, size_t len) {
    _lightHsvPayload(lightHsv(), out, len);
}

String _lightHsvPayload() {
    char out[64] { 0 };
    _lightHsvPayload(out, sizeof(out));
    return out;
}

void _lightRgbPayload(Light::Rgb rgb, char* out, size_t size) {
    if (!_light_has_color) {
        static char zeroes[] PROGMEM = "0,0,0";
        if (!size || (size > sizeof(zeroes))) {
            return;
        }

        memcpy_P(out, zeroes, sizeof(zeroes));
        return;
    }

    snprintf_P(out, size, PSTR("%hhu,%hhu,%hhu"), rgb.red(), rgb.green(), rgb.blue());
}

void _lightRgbPayload(char* out, size_t size, bool target) {
    _lightRgbPayload(_lightToRgb(target), out, size);
}

void _lightRgbPayload(char* out, size_t size) {
    _lightRgbPayload(out, size, false);
}

String _lightRgbPayload(bool target = false) {
    char out[32] { 0 };
    _lightRgbPayload(out, sizeof(out), target);
    return out;
}

void _lightFromGroupPayload(const char* payload) {
    char buffer[16] = {0};
    std::strncpy(buffer, payload, sizeof(buffer) - 1);

    auto channels = lightChannels();
    decltype(channels) channel = 0;

    char* tok = std::strtok(buffer, ",");
    while ((channel < channels) && (tok != nullptr)) {
        char* endp { nullptr };
        auto value = strtol(tok, &endp, 10);
        if ((endp == tok) || (*endp != '\0') || (value >= Light::ValueMax)) {
            return;
        }

        lightChannel(channel++, value);
        tok = std::strtok(nullptr, ",");
    }
}

String _lightGroupPayload(bool target) {
    const auto channels = lightChannels();

    String result;
    result.reserve(4 * channels);

    for (auto& channel : _light_channels) {
        if (result.length()) result += ',';
        result += String(target ? channel.target : channel.inputValue);
    }

    return result;
}

int _lightAdjustValue(const int& value, const String& operation) {
    if (!operation.length()) return value;

    // if prefixed with a sign, treat expression as numerical operation
    // otherwise, use as the new value
    int updated = operation.toInt();
    if (operation[0] == '+' || operation[0] == '-') {
        updated = value + updated;
    }

    return updated;
}

void _lightAdjustBrightness(const char* payload) {
    lightBrightness(_lightAdjustValue(lightBrightness(), payload));
}

void _lightAdjustBrightness(const String& payload) {
    _lightAdjustBrightness(payload.c_str());
}

void _lightAdjustChannel(size_t id, const char* payload) {
    lightChannel(id, _lightAdjustValue(lightChannel(id), payload));
}

void _lightAdjustChannel(size_t id, const String& payload) {
    _lightAdjustChannel(id, payload.c_str());
}

void _lightAdjustKelvin(const char* payload) {
    _fromKelvin(_lightAdjustValue(_toKelvin(_light_mireds), payload));
}

void _lightAdjustKelvin(const String& payload) {
    _lightAdjustKelvin(payload.c_str());
}

void _lightAdjustMireds(const char* payload) {
    _fromMireds(_lightAdjustValue(_light_mireds, payload));
}

void _lightAdjustMireds(const String& payload) {
    _lightAdjustMireds(payload.c_str());
}

// -----------------------------------------------------------------------------
// PROVIDER
// -----------------------------------------------------------------------------

namespace {

// Gamma Correction lookup table (8 bit, ~2.2)
// (TODO: could be constexpr, but the gamma table is still loaded into the RAM when marked as if it is a non-constexpr array)
uint8_t _lightGammaMap(uint8_t value) {
    static uint8_t gamma[256] PROGMEM {
        0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
        1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,
        3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,
        6,   7,   7,   7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  11,  11,  11,
        12,  12,  13,  13,  14,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,
        19,  20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,
        29,  30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,
        41,  42,  43,  43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  53,  54,
        55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  71,
        72,  73,  74,  75,  76,  77,  78,  80,  81,  82,  83,  84,  86,  87,  88,  89,
        91,  92,  93,  94,  96,  97,  98,  100, 101, 102, 104, 105, 106, 108, 109, 110,
        112, 113, 115, 116, 118, 119, 121, 122, 123, 125, 126, 128, 130, 131, 133, 134,
        136, 137, 139, 140, 142, 144, 145, 147, 149, 150, 152, 154, 155, 157, 159, 160,
        162, 164, 166, 167, 169, 171, 173, 175, 176, 178, 180, 182, 184, 186, 187, 189,
        191, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
        223, 225, 227, 229, 231, 233, 235, 238, 240, 242, 244, 246, 248, 251, 253, 255
    };

    static_assert(Light::ValueMax < (sizeof(gamma) / sizeof(gamma[0])), "Out-of-bounds array access");
    static_assert(Light::ValueMin >= 0, "Minimal value can't be negative");
    static_assert(Light::ValueMin < Light::ValueMax, "");

    return pgm_read_byte(&gamma[value]);
}

class LightTransitionHandler {
public:
    using Channels = std::vector<channel_t>;

    struct Transition {
        float& value;
        long target;
        float step;
        size_t count;
    };

    explicit LightTransitionHandler(Channels& channels, bool state, LightTransition transition) :
        _state(state),
        _time(transition.time),
        _step(transition.step)
    {
        OnceFlag delayed;
        for (auto& channel : channels) {
            delayed = prepare(channel, state);
        }

        // if nothing to do, ignore transition step & time and just schedule as soon as possible
        if (!delayed) {
            reset();
            return;
        }

        DEBUG_MSG_P(PSTR("[LIGHT] Scheduled transition for %u (ms) every %u (ms)\n"), _time, _step);
    }

    bool prepare(channel_t& channel, bool state) {
        bool target_state = state && channel.state;
        long target = target_state ? channel.value : Light::ValueMin;

        channel.target = target;
        if (channel.gamma) {
            target = _lightGammaMap(static_cast<uint8_t>(target));
        }

        if (channel.inverse) {
            target = Light::ValueMax - target;
        }

        float diff = static_cast<float>(target) - channel.current;
        if (isImmediate(target_state, diff)) {
            Transition transition { channel.current, target, diff, 1};
            _transitions.push_back(std::move(transition));
            return false;
        }

        float step = (diff > 0.0) ? 1.0f : -1.0f;
        float every = static_cast<double>(_time) / std::abs(diff);
        if (every < _step) {
            auto step_ref = static_cast<float>(_step);
            step *= (step_ref / every);
            every = step_ref;
        }
        size_t count = _time / every;

        Transition transition { channel.current, target, step, count };
        _transitions.push_back(std::move(transition));

        show(transition);

        return true;
    }

    void reset() {
        _step = 10;
        _time = 10;
    }

    template <typename StateFunc, typename ValueFunc, typename UpdateFunc>
    bool run(StateFunc&& state, ValueFunc&& value, UpdateFunc&& update) {
        bool next { false };

        if (!_state_notified && _state) {
            _state_notified = true;
            state(_state);
        }

        for (size_t index = 0; index < _transitions.size(); ++index) {
            auto& transition = _transitions[index];
            if (!transition.count) {
                continue;
            }

            if (--transition.count) {
                transition.value += transition.step;
                next = true;
            } else {
                transition.value = transition.target;
            }

            value(index, transition.value);
        }

        if (!_state_notified && !next && !_state) {
            _state_notified = true;
            state(_state);
        }

        update();

        return next;
    }

    bool state() const {
        return _state;
    }

    unsigned long step() const {
        return _step;
    }

    unsigned long time() const {
        return _time;
    }

private:
    bool isImmediate(bool state, float diff) {
        return (!_time || (_step >= _time) || (std::abs(diff) <= std::numeric_limits<float>::epsilon()));
    }

    static void show(const Transition& transition [[gnu::unused]]) {
        DEBUG_MSG_P(PSTR("[LIGHT] Transition from %s to %ld (step %s, %u times)\n"),
            String(transition.value, 2).c_str(),
            transition.target,
            String(transition.step, 2).c_str(),
            transition.count
        );
    }

    std::vector<Transition> _transitions;
    bool _state_notified { false };

    bool _state;
    unsigned long _time;
    unsigned long _step;
};

} // namespace

struct LightUpdateHandler {
    LightUpdateHandler() = default;

    explicit operator bool() {
        return _run;
    }

    void lock() {
        _lock = true;
    }

    void unlock() {
        _lock = false;
    }

    void reset() {
        _lock = false;
        _run = false;
    }

    void set(bool save, LightTransition transition, int report) {
        if (_lock) {
            panic();
        }

        _run = true;

        _save = save;
        _transition = transition;
        _report = report;
    }

    template <typename T>
    void run(T&& callback) {
        if (!_run) {
            panic();
        }

        lock();
        callback(_save, _transition, _report);
        reset();
    }

private:
    bool _save;
    LightTransition _transition;
    int _report;

    bool _run { false };
    bool _lock { false };
};

LightUpdateHandler _light_update;
bool _light_provider_update = false;

std::unique_ptr<LightTransitionHandler> _light_transition;

Ticker _light_transition_ticker;
bool _light_use_transitions = false;
unsigned long _light_transition_time = LIGHT_TRANSITION_TIME;
unsigned long _light_transition_step = LIGHT_TRANSITION_STEP;

void _lightProviderSchedule(unsigned long ms);

#if (LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER) || (LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX)

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
unsigned char _light_my92xx_channel_map[Light::ChannelsMax] = {};
#endif

// there is no PWM stop, but my92xx has some internal state control that will send 0 as values when OFF
void _lightProviderHandleState(bool state [[gnu::unused]]) {
#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
    _my92xx->setState(state);
#endif
}

// See cores/esp8266/WMath.cpp::map
inline bool _lightPwmMap(long value, long& result) {
    constexpr auto divisor = (Light::ValueMax - Light::ValueMin);
    if (divisor != 0l){
        result = (value - Light::ValueMin) * (Light::PwmLimit - Light::PwmMin) / divisor + Light::PwmMin;
        return true;
    }

    return false;
}

// both require original values to be scaled into a PWM frequency
void _lightProviderHandleValue(size_t channel, float value) {
    long pwm;
    if (!_lightPwmMap(std::lround(value), pwm)) {
        return;
    }

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
    pwm_set_duty(pwm, channel);
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
    _my92xx->setChannel(_light_my92xx_channel_map[channel], pwm);
#endif
}

void _lightProviderHandleUpdate() {
#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
    pwm_start();
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
    _my92xx->update();
#endif
}

#elif LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM

void _lightProviderHandleState(bool state) {
    _light_provider->state(state);
}

void _lightProviderHandleValue(size_t channel, float value) {
    _light_provider->channel(channel, value);
}

void _lightProviderHandleUpdate() {
    _light_provider->update();
}

#endif

void _lightProviderUpdate() {
    if (!_light_provider_update) {
        return;
    }

    if (!_light_transition) {
        _light_provider_update = false;
        return;
    }

    auto next = _light_transition->run(
        _lightProviderHandleState,
        _lightProviderHandleValue,
        _lightProviderHandleUpdate);

    if (next) {
        _lightProviderSchedule(_light_transition->step());
    } else {
        _light_transition.reset(nullptr);
    }

    _light_provider_update = false;
}

void _lightProviderSchedule(unsigned long ms) {
    _light_transition_ticker.once_ms(ms, []() {
        _light_provider_update = true;
    });
}

// -----------------------------------------------------------------------------
// PERSISTANCE
// -----------------------------------------------------------------------------

// Layout should match the old union:
//
// union light_rtcmem_t {
//     struct {
//         uint8_t channels[Light::ChannelsMax];
//         uint8_t brightness;
//         uint16_t mired;
//     } __attribute__((packed)) packed;
//     uint64_t value;
// };

struct LightRtcmem {
    //   1 2 3 4 5 6 7 8
    // [ m m b c c c c c ]
    //         ^ ^ ^ ^ ^   channels
    //       ^ ~ ~ ~ ~ ~   brightness
    //   ^ ^ ~ ~ ~ ~ ~ ~   mireds
    //
    // As seen in the rtcmem dump:
    //   `ddccbbaa 112233ee`
    // Where:
    // - 1122 are mireds
    // - 33 is brightness
    // - aabbccddee are channels (from 0 to 5 respectively)
    explicit LightRtcmem(uint64_t value) {
        _mireds = (value >> (8ull * 6ull)) & 0xffffull;
        _brightness = (value >> (8ull * 5ull)) & 0xffull;

        _channels[4] = static_cast<uint8_t>((value >> (8ull * 4ull)));
        _channels[3] = static_cast<uint8_t>((value >> (8ull * 3ull)));
        _channels[2] = static_cast<uint8_t>((value >> (8ull * 2ull)));
        _channels[1] = static_cast<uint8_t>((value >> (8ull * 1ull)));
        _channels[0] = static_cast<uint8_t>((value & 0xffull));
    }

    using Channels = std::array<uint8_t, Light::ChannelsMax>;
    static_assert(Light::ChannelsMax == 5, "");

    LightRtcmem() {
        _channels.fill(Light::ValueMin);
    }

    LightRtcmem(const Channels& channels, long brightness, long mireds) :
        _channels(channels),
        _brightness(brightness),
        _mireds(mireds)
    {}

    uint64_t serialize() const {
        return ((static_cast<uint64_t>(_mireds) & 0xffffull) << (8ull * 6ull))
            | ((static_cast<uint64_t>(_brightness) & 0xffull) << (8ull * 5ull))
            | (static_cast<uint64_t>(_channels[4]) << (8ull * 4ull))
            | (static_cast<uint64_t>(_channels[3]) << (8ull * 3ull))
            | (static_cast<uint64_t>(_channels[2]) << (8ull * 2ull))
            | (static_cast<uint64_t>(_channels[1]) << (8ull * 1ull))
            | (static_cast<uint64_t>(_channels[0]));
    }

    static Channels defaultChannels() {
        Channels out;
        out.fill(Light::ValueMin);
        return out;
    }

    const Channels& channels() const {
        return _channels;
    }

    long brightness() const {
        return _brightness;
    }

    long mireds() const {
        return _mireds;
    }

private:
    Channels _channels;
    long _brightness { Light::BrightnessMax };
    long _mireds { Light::MiredsDefault };
};

bool lightSave() {
    return _light_save;
}

void lightSave(bool save) {
    _light_save = save;
}

void _lightSaveRtcmem() {
    auto channels = LightRtcmem::defaultChannels();
    for (size_t channel = 0; channel < lightChannels(); ++channel) {
        channels[channel] = _light_channels[channel].inputValue;
    }

    LightRtcmem light(channels, _light_brightness, _light_mireds);
    Rtcmem->light = light.serialize();
}

void _lightRestoreRtcmem() {
    uint64_t value = Rtcmem->light;
    LightRtcmem light(value);

    auto& channels = light.channels();
    for (size_t channel = 0; channel < lightChannels(); ++channel) {
        _light_channels[channel].inputValue = channels[channel];
    }

    _light_mireds = light.mireds(); // channels are already set
    lightBrightness(light.brightness());
}

void _lightSaveSettings() {
    if (!_light_save) {
        return;
    }

    for (size_t channel = 0; channel < lightChannels(); ++channel) {
        setSetting({"ch", channel}, _light_channels[channel].inputValue);
    }

    setSetting("brightness", _light_brightness);
    setSetting("mireds", _light_mireds);

    saveSettings();
}

void _lightRestoreSettings() {
    for (size_t channel = 0; channel < lightChannels(); ++channel) {
        auto value = getSetting({"ch", channel}, (channel == 0) ? Light::ValueMax : Light::ValueMin);
        _light_channels[channel].inputValue = value;
    }

    _light_mireds = getSetting("mireds", _light_mireds);
    lightBrightness(getSetting("brightness", Light::BrightnessMax));
}

bool _lightParsePayload(const char* payload) {
    switch (rpcParsePayload(payload)) {
    case PayloadStatus::On:
        lightState(true);
        break;
    case PayloadStatus::Off:
        lightState(false);
        break;
    case PayloadStatus::Toggle:
        lightState(!_light_state);
        break;
    case PayloadStatus::Unknown:
        return false;
    }

    return true;
}

bool _lightParsePayload(const String& payload) {
    return _lightParsePayload(payload.c_str());
}

bool _lightTryParseChannel(const char* p, size_t& id) {
    return tryParseId(p, lightChannels, id);
}

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

int _lightMqttReportMask() {
    return Light::DefaultReport & ~(static_cast<int>(mqttForward() ? Light::Report::None : Light::Report::Mqtt));
}

int _lightMqttReportGroupMask() {
    return _lightMqttReportMask() & ~static_cast<int>(Light::Report::MqttGroup);
}

void _lightUpdateFromMqtt(LightTransition transition) {
    lightUpdate(_light_save, transition, _lightMqttReportMask());
}

void _lightUpdateFromMqtt() {
    _lightUpdateFromMqtt(lightTransition());
}

void _lightUpdateFromMqttGroup() {
    lightUpdate(_light_save, lightTransition(), _lightMqttReportGroupMask());
}

#if MQTT_SUPPORT

// TODO: implement per-module heartbeat mask? e.g. to exclude unwanted topics based on preference, not settings

bool _lightMqttHeartbeat(heartbeat::Mask mask) {
    if (mask & heartbeat::Report::Light)
        lightMQTT();

    return mqttConnected();
}

void _lightMqttCallback(unsigned int type, const char * topic, const char * payload) {

    String mqtt_group_color = getSetting("mqttGroupColor");

    if (type == MQTT_CONNECT_EVENT) {

        mqttSubscribe(MQTT_TOPIC_BRIGHTNESS);

        if (_light_has_color) {
            mqttSubscribe(MQTT_TOPIC_COLOR_RGB);
            mqttSubscribe(MQTT_TOPIC_COLOR_HEX);
            mqttSubscribe(MQTT_TOPIC_COLOR_HSV);
        }

        if (_light_has_color || _light_use_cct) {
            mqttSubscribe(MQTT_TOPIC_MIRED);
            mqttSubscribe(MQTT_TOPIC_KELVIN);
        }

        // Transition config (everything sent after this will use this new value)
        mqttSubscribe(MQTT_TOPIC_TRANSITION);

        // Group color
        if (mqtt_group_color.length() > 0) mqttSubscribeRaw(mqtt_group_color.c_str());

        // Channels
        char buffer[strlen(MQTT_TOPIC_CHANNEL) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_CHANNEL);
        mqttSubscribe(buffer);

        // Global lights control
        if (!_light_has_controls) {
            mqttSubscribe(MQTT_TOPIC_LIGHT);
        }
    }

    if (type == MQTT_MESSAGE_EVENT) {
        // Group color
        if ((mqtt_group_color.length() > 0) && (mqtt_group_color.equals(topic))) {
            _lightFromGroupPayload(payload);
            _lightUpdateFromMqttGroup();
            return;
        }

        // Match topic
        String t = mqttMagnitude((char *) topic);

        // Color temperature in mireds
        if (t.equals(MQTT_TOPIC_MIRED)) {
            _lightAdjustMireds(payload);
            _lightUpdateFromMqtt();
            return;
        }

        // Color temperature in kelvins
        if (t.equals(MQTT_TOPIC_KELVIN)) {
            _lightAdjustKelvin(payload);
            _lightUpdateFromMqtt();
            return;
        }

        // Color
        if (t.equals(MQTT_TOPIC_COLOR_RGB) || t.equals(MQTT_TOPIC_COLOR_HEX)) {
            _lightFromRgbPayload(payload);
            _lightUpdateFromMqtt();
            return;
        }

        if (t.equals(MQTT_TOPIC_COLOR_HSV)) {
            _lightFromHsvPayload(payload);
            _lightUpdateFromMqtt();
            return;
        }

        // Transition setting
        if (t.equals(MQTT_TOPIC_TRANSITION)) {
            lightTransition(strtoul(payload, nullptr, 10), _light_transition_step);
            return;
        }

        // Brightness
        if (t.equals(MQTT_TOPIC_BRIGHTNESS)) {
            _lightAdjustBrightness(payload);
            _lightUpdateFromMqtt();
            return;
        }

        // Channel
        if (t.startsWith(MQTT_TOPIC_CHANNEL)) {
            size_t id;
            if (!_lightTryParseChannel(t.c_str() + strlen(MQTT_TOPIC_CHANNEL) + 1, id)) {
                return;
            }

            _lightAdjustChannel(id, payload);
            _lightUpdateFromMqtt();
            return;
        }

        // Global
        if (t.equals(MQTT_TOPIC_LIGHT)) {
            _lightParsePayload(payload);
            _lightUpdateFromMqtt();
        }

    }

}

void _lightMqttSetup() {
    mqttHeartbeat(_lightMqttHeartbeat);
    mqttRegister(_lightMqttCallback);
}

void lightMQTT() {
    char buffer[20];

    if (_light_has_color) {
        _lightRgbHexPayload(buffer, sizeof(buffer), true);
        mqttSend(MQTT_TOPIC_COLOR_HEX, buffer);

        _lightRgbPayload(buffer, sizeof(buffer), true);
        mqttSend(MQTT_TOPIC_COLOR_RGB, buffer);

        _lightHsvPayload(buffer, sizeof(buffer));
        mqttSend(MQTT_TOPIC_COLOR_HSV, buffer);
    }

    if (_light_has_color || _light_use_cct) {
        snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _light_mireds);
        mqttSend(MQTT_TOPIC_MIRED, buffer);
    }

    for (unsigned int i=0; i < _light_channels.size(); i++) {
        itoa(_light_channels[i].target, buffer, 10);
        mqttSend(MQTT_TOPIC_CHANNEL, i, buffer);
    }

    snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _light_brightness);
    mqttSend(MQTT_TOPIC_BRIGHTNESS, buffer);

    if (!_light_has_controls) {
        snprintf_P(buffer, sizeof(buffer), "%c", _light_state ? '1' : '0');
        mqttSend(MQTT_TOPIC_LIGHT, buffer);
    }
}

void lightMQTTGroup() {
    const String mqtt_group_color = getSetting("mqttGroupColor");
    if (mqtt_group_color.length()) {
        mqttSendRaw(mqtt_group_color.c_str(), _lightGroupPayload(false).c_str());
    }
}

#endif

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

#if API_SUPPORT

template <typename T>
bool _lightApiTryHandle(ApiRequest& request, T&& callback) {
    auto id_param = request.wildcard(0);
    size_t id;
    if (!_lightTryParseChannel(id_param.c_str(), id)) {
        return false;
    }

    return callback(id);
}

bool _lightApiRgbSetter(ApiRequest& request) {
    lightColor(request.param(F("value")), true);
    lightUpdate();
    return true;
}

void _lightApiSetup() {

    if (_light_has_color) {

        apiRegister(F(MQTT_TOPIC_COLOR_RGB),
            [](ApiRequest& request) {
                request.send(_lightRgbPayload(true));
                return true;
            },
            _lightApiRgbSetter
        );

        apiRegister(F(MQTT_TOPIC_COLOR_HEX),
            [](ApiRequest& request) {
                request.send(_lightRgbHexPayload(true));
                return true;
            },
            _lightApiRgbSetter
        );

        apiRegister(F(MQTT_TOPIC_COLOR_HSV),
            [](ApiRequest& request) {
                request.send(_lightHsvPayload());
                return true;
            },
            [](ApiRequest& request) {
                lightColor(request.param(F("value")), false);
                lightUpdate();
                return true;
            }
        );

        apiRegister(F(MQTT_TOPIC_MIRED),
            [](ApiRequest& request) {
                request.send(String(_light_mireds));
                return true;
            },
            [](ApiRequest& request) {
                _lightAdjustMireds(request.param(F("value")));
                lightUpdate();
                return true;
            }
        );

        apiRegister(F(MQTT_TOPIC_KELVIN),
            [](ApiRequest& request) {
                request.send(String(_toKelvin(_light_mireds)));
                return true;
            },
            [](ApiRequest& request) {
                _lightAdjustKelvin(request.param(F("value")));
                lightUpdate();
                return true;
            }
        );

    }

    apiRegister(F(MQTT_TOPIC_TRANSITION),
        [](ApiRequest& request) {
            request.send(String(lightTransitionTime()));
            return true;
        },
        [](ApiRequest& request) {
            auto value = request.param(F("value"));
            lightTransition(strtoul(value.c_str(), nullptr, 10), _light_transition_step);
            return true;
        }
    );

    apiRegister(F(MQTT_TOPIC_BRIGHTNESS),
        [](ApiRequest& request) {
            request.send(String(static_cast<int>(_light_brightness)));
            return true;
        },
        [](ApiRequest& request) {
            _lightAdjustBrightness(request.param(F("value")));
            lightUpdate();
            return true;
        }
    );

    apiRegister(F(MQTT_TOPIC_CHANNEL "/+"),
        [](ApiRequest& request) {
            return _lightApiTryHandle(request, [&](size_t id) {
                request.send(String(static_cast<int>(_light_channels[id].target)));
                return true;
            });
        },
        [](ApiRequest& request) {
            return _lightApiTryHandle(request, [&](size_t id) {
                _lightAdjustChannel(id, request.param(F("value")));
                lightUpdate();
                return true;
            });
        }
    );

    if (!_light_has_controls) {
        apiRegister(F(MQTT_TOPIC_LIGHT),
            [](ApiRequest& request) {
                request.send(lightState() ? "1" : "0");
                return true;
            },
            [](ApiRequest& request) {
                _lightParsePayload(request.param(F("value")));
                lightUpdate();
                return true;
            }
        );
    }
}

#endif // API_SUPPORT


#if WEB_SUPPORT

bool _lightWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    if (strncmp(key, "light", 5) == 0) return true;
    if (strncmp(key, "use", 3) == 0) return true;
    if (strncmp(key, "lt", 2) == 0) return true;
    return false;
}

void _lightWebSocketStatus(JsonObject& root) {
    if (_light_has_color) {
        if (_light_use_rgb) {
            root["rgb"] = lightRgbPayload();
        } else {
            root["hsv"] = lightHsvPayload();
        }
    }
    if (_light_use_cct) {
        JsonObject& mireds = root.createNestedObject("mireds");
        mireds["value"] = _light_mireds;
        mireds["cold"] = _light_cold_mireds;
        mireds["warm"] = _light_warm_mireds;
        root["useCCT"] = _light_use_cct;
    }
    JsonArray& channels = root.createNestedArray("channels");
    for (size_t id = 0; id < _light_channels.size(); ++id) {
        channels.add(lightChannel(id));
    }
    root["brightness"] = lightBrightness();
    root["lightstate"] = lightState();
}

void _lightWebSocketOnVisible(JsonObject& root) {
    root["colorVisible"] = 1;
}

void _lightWebSocketOnConnected(JsonObject& root) {
    root["mqttGroupColor"] = getSetting("mqttGroupColor");
    root["useColor"] = _light_has_color;
    root["useWhite"] = _light_use_white;
    root["useGamma"] = _light_use_gamma;
    root["useTransitions"] = _light_use_transitions;
    root["useRGB"] = _light_use_rgb;
    root["ltSave"] = _light_save;
    root["ltTime"] = _light_transition_time;
    root["ltStep"] = _light_transition_step;
#if RELAY_SUPPORT
    root["ltRelay"] = getSetting("ltRelay", 1 == LIGHT_RELAY_ENABLED);
#endif
}

void _lightWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {

    if (_light_has_color) {
        if (strcmp(action, "color") == 0) {
            if (data.containsKey("rgb")) {
                _lightFromRgbPayload(data["rgb"].as<const char*>());
                lightUpdate();
            }
            if (data.containsKey("hsv")) {
                _lightFromHsvPayload(data["hsv"].as<const char*>());
                lightUpdate();
            }
        }
    }

    if (strcmp(action, "mireds") == 0) {
        _fromMireds(data["mireds"]);
        lightUpdate();
    }

    if (strcmp(action, "channel") == 0) {
        if (data.containsKey("id") && data.containsKey("value")) {
            lightChannel(data["id"].as<unsigned char>(), data["value"].as<int>());
            lightUpdate();
        }
    }

    if (strcmp(action, "brightness") == 0) {
        if (data.containsKey("value")) {
            lightBrightness(data["value"].as<int>());
            lightUpdate();
        }
    }

}

#endif

#if TERMINAL_SUPPORT

void _lightInitCommands() {

    terminalRegisterCommand(F("LIGHT"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc > 1) {
            if (!_lightParsePayload(ctx.argv[1].c_str())) {
                terminalError(ctx, F("Invalid payload"));
                return;
            }
            lightUpdate();
        }

        ctx.output.printf("%s\n", _light_state ? "ON" : "OFF");
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("BRIGHTNESS"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc > 1) {
            _lightAdjustBrightness(ctx.argv[1].c_str());
            lightUpdate();
        }
        ctx.output.printf("%ld\n", lightBrightness());
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("CHANNEL"), [](const terminal::CommandContext& ctx) {
        auto channels = lightChannels();
        if (!channels) {
            terminalError(ctx, F("No channels configured"));
            return;
        }

        auto description = [&](size_t channel) {
            ctx.output.printf("#%u (%s): %hhu\n", channel, _lightDesc(channels, channel), _light_channels[channel].inputValue);
        };

        if (ctx.argc > 2) {
            size_t id;
            if (!_lightTryParseChannel(ctx.argv[1].c_str(), id)) {
                terminalError(ctx, F("Invalid channel ID"));
                return;
            }

            _lightAdjustChannel(id, ctx.argv[2].c_str());
            lightUpdate();
            description(id);
        } else {
            for (size_t index = 0; index < channels; ++index) {
                description(index);
            }
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("RGB"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc > 1) {
            _lightFromRgbPayload(ctx.argv[1].c_str());
            lightUpdate();
        }
        ctx.output.printf_P(PSTR("rgb %s\n"), lightRgbPayload().c_str());
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("HSV"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc > 1) {
            _lightFromHsvPayload(ctx.argv[1].c_str());
            lightUpdate();
        }
        ctx.output.printf_P(PSTR("hsv %s\n"), lightHsvPayload().c_str());
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("KELVIN"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc > 1) {
            _lightAdjustKelvin(ctx.argv[1].c_str());
            lightUpdate();
        }
        ctx.output.printf_P(PSTR("kelvin %ld\n"), _toKelvin(_light_mireds));
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("MIRED"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc > 1) {
            _lightAdjustMireds(ctx.argv[1]);
            lightUpdate();
        }
        ctx.output.printf_P(PSTR("mireds %ld\n"), _light_mireds);
        terminalOK(ctx);
    });

}

#endif // TERMINAL_SUPPORT

size_t lightChannels() {
    return _light_channels.size();
}

bool lightHasColor() {
    return _light_has_color;
}

bool lightUseCCT() {
    return _light_use_cct;
}

bool lightUseRGB() {
    return _light_use_rgb;
}

// -----------------------------------------------------------------------------

Light::Rgb lightRgb() {
    return {_light_mapping.red(), _light_mapping.green(), _light_mapping.blue()};
}

void lightRgb(Light::Rgb rgb) {
    _setRGBInputValue(rgb.red(), rgb.green(), rgb.blue());
}

Light::Hsv _lightHsv(Light::Rgb rgb) {
    auto r = static_cast<double>(rgb.red()) / Light::ValueMax;
    auto g = static_cast<double>(rgb.green()) / Light::ValueMax;
    auto b = static_cast<double>(rgb.blue()) / Light::ValueMax;

    auto max = std::max({r, g, b});
    auto min = std::min({r, g, b});

    auto v = max;

    if (min != max) {
        auto s = (max - min) / max;

        auto delta = max - min;
        auto rc = (max - r) / delta;
        auto gc = (max - g) / delta;
        auto bc = (max - b) / delta;

        double h { 0.0 };
        if (r == max) {
            h = bc - gc;
        } else if (g == max) {
            h = 2.0 + rc - bc;
        } else {
            h = 4.0 + gc - rc;
        }

        h = fs_fmod((h / 6.0), 1.0);
        if (h < 0.0) {
            h = 1.0 + h;
        }

        return Light::Hsv(
            std::lround(h * 360.0),
            std::lround(s * 100.0),
            std::lround(v * 100.0));
    }

    return Light::Hsv(Light::Hsv::HueMin, Light::Hsv::SaturationMin, v);

}

Light::Hsv lightHsv() {
    return _lightHsv(lightRgb());
}

// HSV to RGB transformation -----------------------------------------------
//
// INPUT: [0,100,57]
// IS: [145,0,0]
// SHOULD: [255,0,0]

void lightHsv(Light::Hsv hsv) {
    double r { 0.0 };
    double g { 0.0 };
    double b { 0.0 };

    auto v = static_cast<double>(hsv.value()) / 100.0;
    long brightness { std::lround(v * static_cast<double>(Light::BrightnessMax)) };

    if (hsv.saturation()) {
        auto h = hsv.hue();
        if (h < 0) {
            h = 0;
        } else if (h >= 360) {
            h = 359;
        }

        auto s = static_cast<double>(hsv.saturation()) / 100.0;

        auto c = v * s;

        auto hmod2 = fs_fmod(static_cast<double>(h) / 60.0, 2.0);
        auto x = c * (1.0 - std::abs(hmod2 - 1.0));

        auto m = v - c;

        if ((0 <= h) && (h < 60)) {
            r = c;
            g = x;
        } else if ((60 <= h) && (h < 120)) {
            r = x;
            g = c;
        } else if ((120 <= h) && (h < 180)) {
            g = c;
            b = x;
        } else if ((180 <= h) && (h < 240)) {
            g = x;
            b = c;
        } else if ((240 <= h) && (h < 300)) {
            r = x;
            b = c;
        } else if ((300 <= h) && (h < 360)) {
            r = c;
            b = x;
        }

        r = (r + m) * 255.0;
        g = (g + m) * 255.0;
        b = (b + m) * 255.0;
    } else {
        r = brightness;
        g = brightness;
        b = brightness;
    }

    lightBrightness(brightness);
    _setRGBInputValue(r, g, b);
}

void lightHs(long hue, long saturation) {
    lightHsv({hue, saturation, Light::Hsv::ValueMax});
}

// -----------------------------------------------------------------------------

void lightSetReportListener(LightReportListener func) {
    _light_report.push_front(func);
}

void _lightReport(int report) {
#if MQTT_SUPPORT
    if (report & Light::Report::Mqtt) {
        lightMQTT();
    }

    if (report & Light::Report::MqttGroup) {
        lightMQTTGroup();
    }
#endif

#if WEB_SUPPORT
    if (report & Light::Report::Web) {
        wsPost(_lightWebSocketStatus);
    }
#endif

    for (auto& report : _light_report) {
        report();
    }
}

void _lightReport(Light::Report report) {
    _lightReport(static_cast<int>(report));
}

// Called in the loop() when we received lightUpdate(...) values

void _lightUpdate() {
    if (!_light_update) {
        return;
    }

    auto changed = _light_brightness_func();
    if (!_light_state_changed && !changed) {
        _light_update.reset();
        return;
    }

    _light_state_changed = false;

    _light_update.run([](bool save, LightTransition transition, int report) {
        // Channel output values will be set by the handler class and the specified provider
        // We either set the values immediately or schedule an ongoing transition
        _light_transition = std::make_unique<LightTransitionHandler>(_light_channels, _light_state, transition);
        _lightProviderSchedule(_light_transition->step());

        // Send current state to all available 'report' targets
        // (make sure to delay the report, in case lightUpdate is called repeatedly)
        _light_report_ticker.once_ms(_light_report_delay, [report]() {
            _lightReport(report);
        });

        // Always save to RTCMEM, optionally preserve the state in the settings storage
        _lightSaveRtcmem();
        if (save) {
            _light_save_ticker.once_ms(_light_save_delay, _lightSaveSettings);
        }
    });
}

void lightUpdate(bool save, LightTransition transition, int report) {
#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
    if (!_light_provider) {
        return;
    }
#endif

    if (!lightChannels()) {
        return;
    }

    _light_update.set(save, transition, report);
}

void lightUpdate(bool save, LightTransition transition, Light::Report report) {
    lightUpdate(save, transition, static_cast<int>(report));
}

void lightUpdate(LightTransition transition) {
    lightUpdate(_light_save, transition, Light::DefaultReport);
}

void lightUpdate(bool save) {
    lightUpdate(save, lightTransition(), Light::DefaultReport);
}

void lightUpdate() {
    lightUpdate(lightTransition());
}

void lightState(size_t id, bool state) {
    if ((id < _light_channels.size()) && _light_channels[id].state != state) {
        _light_channels[id].state = state;
        _light_state_changed = true;
    }
}

bool lightState(size_t id) {
    if (id < _light_channels.size()) {
        return _light_channels[id].state;
    }

    return false;
}

void lightState(bool state) {
    if (_light_state != state) {
        _light_state = state;
        if (_light_state_listener) {
            _light_state_listener(state);
        }
        _light_state_changed = true;
    }
}

bool lightState() {
    return _light_state;
}

void lightColor(const char* color, bool rgb) {
    DEBUG_MSG_P(PSTR("[LIGHT] %s: %s\n"), rgb ? "RGB" : "HSV", color);
    if (rgb) {
        _lightFromRgbPayload(color);
    } else {
        _lightFromHsvPayload(color);
    }
}

void lightColor(const String& color, bool rgb) {
    lightColor(color.c_str(), rgb);
}

void lightColor(const char* color) {
    lightColor(color, true);
}

void lightColor(const String& color) {
    lightColor(color.c_str());
}

void lightColor(unsigned long color) {
    _lightFromInteger(color, false);
}

String lightRgbPayload() {
    char str[12];
    _lightRgbPayload(str, sizeof(str));
    return str;
}

String lightHsvPayload() {
    char str[12];
    _lightHsvPayload(str, sizeof(str));
    return str;
}

String lightColor() {
    return _light_use_rgb ? lightRgbPayload() : lightHsvPayload();
}

long lightRed() {
    return _light_mapping.red();
}

void lightRed(long value) {
    _light_mapping.red(value);
}

long lightGreen() {
    return _light_mapping.green();
}

void lightGreen(long value) {
    _light_mapping.green(value);
}

long lightBlue() {
    return _light_mapping.blue();
}

void lightBlue(long value) {
    _light_mapping.blue(value);
}

long lightWarmWhite() {
    return _light_mapping.warm();
}

void lightWarmWhite(long value) {
    _light_mapping.warm(value);
}

long lightColdWhite() {
    return _light_mapping.cold();
}

void lightColdWhite(long value) {
    _light_mapping.cold(value);
}

void lightMireds(long mireds) {
    _fromMireds(mireds);
}

Light::MiredsRange lightMiredsRange() {
    return { _light_cold_mireds, _light_warm_mireds };
}

long lightChannel(size_t id) {
    if (id < _light_channels.size()) {
        return _light_channels[id].inputValue;
    }

    return 0l;
}

void lightChannel(size_t id, long value) {
    if (id < _light_channels.size()) {
        _setInputValue(id, value);
    }
}

void lightChannelStep(size_t id, long steps, long multiplier) {
    lightChannel(id, lightChannel(id) + (steps * multiplier));
}

long lightBrightness() {
    return _light_brightness;
}

void lightBrightness(long brightness) {
    _light_brightness = constrain(brightness, Light::BrightnessMin, Light::BrightnessMax);
}

void lightBrightnessStep(long steps, long multiplier) {
    lightBrightness(static_cast<int>(_light_brightness) + (steps * multiplier));
}

unsigned long lightTransitionTime() {
    return _light_use_transitions ? _light_transition_time : 0;
}

unsigned long lightTransitionStep() {
    return _light_use_transitions ? _light_transition_step : 0;
}

LightTransition lightTransition() {
    return {lightTransitionTime(), lightTransitionStep()};
}

void lightTransition(unsigned long time, unsigned long step) {
    bool save { false };

    _light_use_transitions = (time && step);
    if (_light_use_transitions) {
        save = true;
        _light_transition_time = time;
        _light_transition_step = step;
    }

    setSetting("useTransitions", _light_use_transitions);
    if (save) {
        setSetting("ltTime", _light_transition_time);
        setSetting("ltStep", _light_transition_step);
    }

    saveSettings();
}

void lightTransition(LightTransition transition) {
    lightTransition(transition.time, transition.step);
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
const unsigned long _light_iomux[16] PROGMEM = {
    PERIPHS_IO_MUX_GPIO0_U, PERIPHS_IO_MUX_U0TXD_U, PERIPHS_IO_MUX_GPIO2_U, PERIPHS_IO_MUX_U0RXD_U,
    PERIPHS_IO_MUX_GPIO4_U, PERIPHS_IO_MUX_GPIO5_U, PERIPHS_IO_MUX_SD_CLK_U, PERIPHS_IO_MUX_SD_DATA0_U,
    PERIPHS_IO_MUX_SD_DATA1_U, PERIPHS_IO_MUX_SD_DATA2_U, PERIPHS_IO_MUX_SD_DATA3_U, PERIPHS_IO_MUX_SD_CMD_U,
    PERIPHS_IO_MUX_MTDI_U, PERIPHS_IO_MUX_MTCK_U, PERIPHS_IO_MUX_MTMS_U, PERIPHS_IO_MUX_MTDO_U
};

const unsigned long _light_iofunc[16] PROGMEM = {
    FUNC_GPIO0, FUNC_GPIO1, FUNC_GPIO2, FUNC_GPIO3,
    FUNC_GPIO4, FUNC_GPIO5, FUNC_GPIO6, FUNC_GPIO7,
    FUNC_GPIO8, FUNC_GPIO9, FUNC_GPIO10, FUNC_GPIO11,
    FUNC_GPIO12, FUNC_GPIO13, FUNC_GPIO14, FUNC_GPIO15
};

#endif

namespace {

inline bool _lightUseGamma(size_t channels, size_t index) {
    switch (_lightTag(channels, index)) {
    case 'R':
    case 'G':
    case 'B':
        return true;
    }

    return false;
}

inline bool _lightUseGamma(size_t index) {
    return _lightUseGamma(_light_channels.size(), index);
}

void _lightConfigure() {
    auto channels = _light_channels.size();

    _light_has_color = getSetting("useColor", 1 == LIGHT_USE_COLOR);
    if (_light_has_color && (channels < 3)) {
        _light_has_color = false;
        setSetting("useColor", _light_has_color);
    }

    _light_use_white = getSetting("useWhite", 1 == LIGHT_USE_WHITE);
    if (_light_use_white && (channels < 4) && (channels != 2)) {
        _light_use_white = false;
        setSetting("useWhite", _light_use_white);
    }

    if (_light_has_color) {
        if (_light_use_white) {
            _light_brightness_func = _lightApplyBrightnessColor;
        } else {
            _light_brightness_func = _lightApplyBrightnessRgb;
        }
    } else {
        _light_brightness_func = _lightApplyBrightnessAll;
    }

    _light_use_cct = getSetting("useCCT", 1 == LIGHT_USE_CCT);
    if (_light_use_cct && (((channels < 5) && (channels != 2)) || !_light_use_white)) {
        _light_use_cct = false;
        setSetting("useCCT", _light_use_cct);
    }

    _light_use_rgb = getSetting("useRGB", 1 == LIGHT_USE_RGB);

    _light_cold_mireds = getSetting("ltColdMired", Light::MiredsCold);
    _light_warm_mireds = getSetting("ltWarmMired", Light::MiredsWarm);
    _light_cold_kelvin = (1000000L / _light_cold_mireds);
    _light_warm_kelvin = (1000000L / _light_warm_mireds);

    _light_use_transitions = getSetting("useTransitions", 1 == LIGHT_USE_TRANSITIONS);
    _light_save = getSetting("ltSave", 1 == LIGHT_SAVE_ENABLED);
    _light_save_delay = getSetting("ltSaveDelay", LIGHT_SAVE_DELAY);
    _light_transition_time = getSetting("ltTime", LIGHT_TRANSITION_TIME);
    _light_transition_step = getSetting("ltStep", LIGHT_TRANSITION_STEP);

    _light_use_gamma = getSetting("useGamma", 1 == LIGHT_USE_GAMMA);
    for (size_t index = 0; index < lightChannels(); ++index) {
#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
        _light_my92xx_channel_map[index] = getSetting({"ltMy92xxCh", index}, Light::build::my92xxChannel(index));
#endif
        _light_channels[index].inverse = getSetting({"ltInv", index}, Light::build::inverse(index));
        _light_channels[index].gamma = (_light_has_color && _light_use_gamma) && _lightUseGamma(channels, index);
    }

}

#if RELAY_SUPPORT

void _lightRelaySupport() {
    if (!getSetting("ltRelay", 1 == LIGHT_RELAY_ENABLED)) {
        return;
    }

    if (_light_has_controls) {
        return;
    }

    auto next_id = relayCount();
    if (relayAdd(std::make_unique<LightGlobalProvider>())) {
        _light_state_listener = [next_id](bool state) {
            relayStatus(next_id, state);
        };
        _light_has_controls = true;
    }
}

#endif

void _lightBoot() {
    auto channels = _light_channels.size();
    if (channels) {
        DEBUG_MSG_P(PSTR("[LIGHT] Number of channels: %u\n"), channels);

        _lightUpdateMapping(channels);

        _lightConfigure();
        if (rtcmemStatus()) {
            _lightRestoreRtcmem();
        } else {
            _lightRestoreSettings();
        }

        _light_state_changed = true;
        lightUpdate(false);
    }
}

} // namespace

#if LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM

// Custom provider is expected to:
// - register a controller class via `lightSetProvider(...)`
// - use `lightAdd()` N times to create N channels that will be handled via the controller
// Once that's done, we 'boot' the provider and disable further calls to the `lightAdd()`

void lightSetProvider(std::unique_ptr<LightProvider>&& ptr) {
    _light_provider = std::move(ptr);
}

bool lightAdd() {
    enum class State {
        None,
        Scheduled,
        Done
    };

    static State state { State::None };
    if (State::Done == state) {
        return false;
    }

    if (_light_channels.size() < Light::ChannelsMax) {
        _light_channels.emplace_back(GPIO_NONE);
        if (State::Scheduled != state) {
            state = State::Scheduled;
            schedule_function([]() {
                _lightBoot();
                state = State::Done;
            });
        }

        return true;
    }

    return false;
}

#else

bool lightAdd() {
    return false;
}

#endif // LIGHT_PROVIDER_CUSTOM

void _lightProviderDebug() {
    DEBUG_MSG_P(PSTR("[LIGHT] Provider: "
#if LIGHT_PROVIDER == LIGHT_PROVIDER_NONE
        "NONE"
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
        "DIMMER"
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
        "MY92XX"
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_CUSTOM
        "CUSTOM"
#endif
    "\n"));
}

void _lightSettingsMigrate(int version) {
    if (!version || (version >= 5)) {
        return;
    }

    delSettingPrefix({
        "chGPIO",
        "chLogic",
        "myChips",
        "myDCKGPIO",
        "myDIGPIO"
    });
    delSetting("lightProvider");
    delSetting("useCSS");

    moveSetting("lightTime", "ltTime");
    moveSetting("lightColdMired", "ltColdMired");
    moveSetting("lightWarmMired", "ltWarmMired");
}

void lightSetup() {
    _lightSettingsMigrate(migrateVersion());

    const auto enable_pin = getSetting("ltEnableGPIO", Light::build::enablePin());
    if (enable_pin != GPIO_NONE) {
        pinMode(enable_pin, OUTPUT);
        digitalWrite(enable_pin, HIGH);
    }

    _light_channels.reserve(Light::ChannelsMax);

    _lightProviderDebug();

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
    {
        // TODO: library API specifies some hard-coded amount of channels, based off of the model and chips
        // we always map channel index 1-to-1, to simplify hw config, but most of the time there are less active channels
        // than the value generated by the lib (ref. `my92xx::getChannels()`)
        auto channels = getSetting("ltMy92xxChannels", Light::build::my92xxChannels());
        _my92xx = new my92xx(
            getSetting("ltMy92xxModel", Light::build::my92xxModel()),
            getSetting("ltMy92xxChips", Light::build::my92xxChips()),
            getSetting("ltMy92xxDiGPIO", Light::build::my92xxDiPin()),
            getSetting("ltMy92xxDckiGPIO", Light::build::my92xxDckiPin()),
            Light::build::my92xxCommand());
        for (size_t index = 0; index < channels; ++index) {
            _light_channels.emplace_back(GPIO_NONE);
        }
    }
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
    {
        // Initial duty value (will be passed to pwm_set_duty(...), OFF in this case)
        uint32_t pwm_duty_init[Light::ChannelsMax] = {0};

        // 3-tuples of MUX_REGISTER, MUX_VALUE and GPIO number
        uint32_t io_info[Light::ChannelsMax][3];

        for (size_t index = 0; index < Light::ChannelsMax; ++index) {

            // Load up until first GPIO_NONE. Allow settings to override, but not remove values
            const auto pin = getSetting({"ltDimmerGPIO", index}, Light::build::channelPin(index));
            if (!gpioValid(pin)) {
                break;
            }

            _light_channels.emplace_back(pin);

            io_info[index][0] = pgm_read_dword(&_light_iomux[pin]);
            io_info[index][1] = pgm_read_dword(&_light_iofunc[pin]);
            io_info[index][2] = pin;
            pinMode(pin, OUTPUT);

        }

        // with 0 channels this should not do anything at all and provider will never call pwm_set_duty(...)
        pwm_init(Light::PwmMax, pwm_duty_init, _light_channels.size(), io_info);
        pwm_start();
    }
#endif

    _lightBoot();

#if RELAY_SUPPORT
    _lightRelaySupport();
#endif

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_lightWebSocketOnVisible)
            .onConnected(_lightWebSocketOnConnected)
            .onData(_lightWebSocketStatus)
            .onAction(_lightWebSocketOnAction)
            .onKeyCheck(_lightWebSocketOnKeyCheck);
    #endif

    #if API_SUPPORT
        _lightApiSetup();
    #endif

    #if MQTT_SUPPORT
        _lightMqttSetup();
    #endif

    #if TERMINAL_SUPPORT
        _lightInitCommands();
    #endif

    espurnaRegisterReload(_lightConfigure);
    espurnaRegisterLoop([]() {
        _lightUpdate();
        _lightProviderUpdate();
    });

}

#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
