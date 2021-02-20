/*

LIGHT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "light.h"

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#include "api.h"
#include "broker.h"
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

#if RELAY_SUPPORT

// Setup virtual relays contolling the light's state
// TODO: only do per-channel setup optionally

class LightChannelProvider : public RelayProviderBase {
public:
    LightChannelProvider() = delete;
    explicit LightChannelProvider(unsigned char id) :
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
    unsigned char _id { RELAY_NONE };
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

bool _light_save = LIGHT_SAVE_ENABLED;
unsigned long _light_save_delay = LIGHT_SAVE_DELAY;
Ticker _light_save_ticker;

unsigned long _light_report_delay = LIGHT_REPORT_DELAY;
Ticker _light_report_ticker;
LightReportListener _light_report;

bool _light_has_controls = false;
bool _light_has_color = false;
bool _light_use_white = false;
bool _light_use_cct = false;
bool _light_use_gamma = false;

bool _light_state = false;
long _light_brightness = Light::BrightnessMax;

// Default to the Philips Hue value that HA also use.
// https://developers.meethue.com/documentation/core-concepts
long _light_cold_mireds = Light::MiredsCold;
long _light_warm_mireds = Light::MiredsWarm;

long _light_cold_kelvin = (1000000L / _light_cold_mireds);
long _light_warm_kelvin = (1000000L / _light_warm_mireds);

long _light_mireds = (Light::MiredsCold + Light::MiredsWarm) / 2L;

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

bool _setValue(unsigned char, unsigned int) __attribute__((warn_unused_result));
bool _setValue(unsigned char id, unsigned int value) {
    if (_light_channels[id].value != value) {
        _light_channels[id].value = value;
        return true;
    }

    return false;
}

void _setInputValue(unsigned char id, unsigned int value) {
    _light_channels[id].inputValue = value;
}

void _setRGBInputValue(unsigned char red, unsigned char green, unsigned char blue) {
    _setInputValue(0, constrain(red, Light::ValueMin, Light::ValueMax));
    _setInputValue(1, constrain(green, Light::ValueMin, Light::ValueMax));
    _setInputValue(2, constrain(blue, Light::ValueMin, Light::ValueMax));
}

void _setCCTInputValue(unsigned char warm, unsigned char cold) {
    _setInputValue(0, constrain(warm, Light::ValueMin, Light::ValueMax));
    _setInputValue(1, constrain(cold, Light::ValueMin, Light::ValueMax));
}

bool _lightApplyBrightnessChannels(size_t channels) {
    auto scale = static_cast<float>(_light_brightness) / static_cast<float>(Light::BrightnessMax);

    channels = std::min(channels, lightChannels());
    OnceFlag changed;

    for (unsigned char channel = 0; channel < lightChannels(); ++channel) {
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

bool _lightApplyBrightnessColor() {
    OnceFlag changed;

    double brightness = static_cast<double>(_light_brightness) / static_cast<double>(Light::BrightnessMax);

    // Substract the common part from RGB channels and add it to white channel. So [250,150,50] -> [200,100,0,50]
    unsigned char white = std::min(_light_channels[0].inputValue, std::min(_light_channels[1].inputValue, _light_channels[2].inputValue));
    for (unsigned int i=0; i < 3; i++) {
        changed = _setValue(i, _light_channels[i].inputValue - white);
    }

    // Split the White Value across 2 White LED Strips.
    if (_light_use_cct) {

        // This change the range from 153-500 to 0-347 so we get a value between 0 and 1 in the end.
        double miredFactor = ((double) _light_mireds - (double) _light_cold_mireds)/((double) _light_warm_mireds - (double) _light_cold_mireds);

        // set cold white
        _light_channels[3].inputValue = 0;
        changed = _setValue(3, lround(((double) 1.0 - miredFactor) * white));

        // set warm white
        _light_channels[4].inputValue = 0;
        changed = _setValue(4, lround(miredFactor * white));
    } else {
        _light_channels[3].inputValue = 0;
        changed = _setValue(3, white);
    }

    // Scale up to equal input values. So [250,150,50] -> [200,100,0,50] -> [250, 125, 0, 63]
    unsigned char max_in = std::max(_light_channels[0].inputValue, std::max(_light_channels[1].inputValue, _light_channels[2].inputValue));
    unsigned char max_out = std::max(std::max(_light_channels[0].value, _light_channels[1].value), std::max(_light_channels[2].value, _light_channels[3].value));
    unsigned char channelSize = _light_use_cct ? 5 : 4;

    if (_light_use_cct) {
        max_out = std::max(max_out, _light_channels[4].value);
    }

    double factor = (max_out > 0) ? (double) (max_in / max_out) : 0;
    for (unsigned char i=0; i < channelSize; i++) {
        changed = _setValue(i, lround((double) _light_channels[i].value * factor * brightness));
    }

    // Scale white channel to match brightness
    for (unsigned char i=3; i < channelSize; i++) {
        changed = _setValue(i, constrain(static_cast<unsigned int>(_light_channels[i].value * LIGHT_WHITE_FACTOR), Light::BrightnessMin, Light::BrightnessMax));
    }

    // For the rest of channels, don't apply brightness, it is already in the inputValue
    // i should be 4 when RGBW and 5 when RGBWW
    for (unsigned char i=channelSize; i < _light_channels.size(); i++) {
        changed = _setValue(i, _light_channels[i].inputValue);
    }

    return changed.get();
}

char _lightTag(size_t channels, unsigned char index) {
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

char _lightTag(unsigned char index) {
    return _lightTag(_light_channels.size(), index);
}

// UI hint about channel distribution
const char* _lightDesc(size_t channels, unsigned char index) {
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

const char* _lightDesc(unsigned char index) {
    return _lightDesc(_light_channels.size(), index);
}

// -----------------------------------------------------------------------------
// Input Values
// -----------------------------------------------------------------------------

void _fromLong(unsigned long value, bool brightness) {
    if (brightness) {
        _setRGBInputValue((value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF);
        lightBrightness((value & 0xFF) * Light::BrightnessMax / 255);
    } else {
        _setRGBInputValue((value >> 16) & 0xFF, (value >> 8) & 0xFF, (value) & 0xFF);
    }
}

void _fromRGB(const char * rgb) {
    // 9 char #........ , 11 char ...,...,...
    if (!_light_has_color) return;
    if (!rgb || (strlen(rgb) == 0)) return;

    // HEX value is always prefixed, like CSS
    // values are interpreted like RGB + optional brightness
    if (rgb[0] == '#') {
        _fromLong(strtoul(rgb + 1, nullptr, 16), strlen(rgb + 1) > 7);
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
void _fromHSV(const char * hsv) {
    if (!_light_has_color) return;
    if (strlen(hsv) == 0) return;

    char buf[16] = {0};
    strncpy(buf, hsv, sizeof(buf) - 1);

    unsigned char count = 0;
    unsigned int value[3] = {0};

    char * tok = strtok(buf, ",");
    while (tok != NULL) {
        value[count] = atoi(tok);
        if (++count == 3) break;
        tok = strtok(NULL, ",");
    }
    if (count != 3) return;

    // HSV to RGB transformation -----------------------------------------------

    //INPUT: [0,100,57]
    //IS: [145,0,0]
    //SHOULD: [255,0,0]

    const double h = (value[0] == 360) ? 0 : (double) value[0] / 60.0;
    const double f = (h - floor(h));
    const double s = (double) value[1] / 100.0;

    _light_brightness = lround((double) value[2] * (static_cast<double>(Light::BrightnessMax) / 100.0)); // (default 255/100)
    const unsigned char p = lround(Light::ValueMax * (1.0 - s));
    const unsigned char q = lround(Light::ValueMax * (1.0 - s * f));
    const unsigned char t = lround(Light::ValueMax * (1.0 - s * (1.0 - f)));

    switch (int(h)) {
        case 0:
            _setRGBInputValue(Light::ValueMax, t, p);
            break;
        case 1:
            _setRGBInputValue(q, Light::ValueMax, p);
            break;
        case 2:
            _setRGBInputValue(p, Light::ValueMax, t);
            break;
        case 3:
            _setRGBInputValue(p, q, Light::ValueMax);
            break;
        case 4:
            _setRGBInputValue(t, p, Light::ValueMax);
            break;
        case 5:
            _setRGBInputValue(Light::ValueMax, p, q);
            break;
        default:
            _setRGBInputValue(Light::ValueMin, Light::ValueMin, Light::ValueMin);
            break;
    }
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

    // This change the range from 153-500 to 0-347 so we get a value between 0 and 1 in the end.
    const double factor = ((double) _light_mireds - (double) _light_cold_mireds)/((double) _light_warm_mireds - (double) _light_cold_mireds);
    _setCCTInputValue(
        lround(factor * Light::ValueMax),
        lround(((double) 1.0 - factor) * Light::ValueMax)
    );
}

void _fromKelvin(long kelvin) {

    if (!_light_has_color) {
        if (!_light_use_cct) return;
        _lightMiredsCCT(kelvin);
        return;
    }

    _lightMireds(kelvin);

    if (_light_use_cct) {
      _setRGBInputValue(Light::ValueMax, Light::ValueMax, Light::ValueMax);
      return;
    }

    // Calculate colors
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

void _toRGB(char * rgb, size_t len, bool target = false) {
    unsigned long value = 0;

    value += target ? _light_channels[0].target : _light_channels[0].inputValue;
    value <<= 8;
    value += target ? _light_channels[1].target : _light_channels[1].inputValue;
    value <<= 8;
    value += target ? _light_channels[2].target : _light_channels[2].inputValue;

    snprintf_P(rgb, len, PSTR("#%06X"), value);
}

String _toRGB(bool target) {
    char buffer[64] { 0 };
    _toRGB(buffer, sizeof(buffer), target);
    return buffer;
}

void _toHSV(char * hsv, size_t len) {
    double h {0.}, s {0.}, v {0.};
    double r {0.}, g {0.}, b {0.};
    double min {0.}, max {0.};

    r = static_cast<double>(_light_channels[0].target) / Light::ValueMax;
    g = static_cast<double>(_light_channels[1].target) / Light::ValueMax;
    b = static_cast<double>(_light_channels[2].target) / Light::ValueMax;

    min = std::min(r, std::min(g, b));
    max = std::max(r, std::max(g, b));

    v = 100.0 * max;
    if (v == 0) {
        h = s = 0;
    } else {
        s = 100.0 * (max - min) / max;
        if (s == 0) {
            h = 0;
        } else {
            if (max == r) {
                if (g >= b) {
                    h = 0.0 + 60.0 * (g - b) / (max - min);
                } else {
                    h = 360.0 + 60.0 * (g - b) / (max - min);
                }
            } else if (max == g) {
                h = 120.0 + 60.0 * (b - r) / (max - min);
            } else {
                h = 240.0 + 60.0 * (r - g) / (max - min);
            }
        }
    }

    // Convert to string. Using lround, since we can't (yet) printf floats
    snprintf(hsv, len, "%d,%d,%d",
        static_cast<int>(lround(h)),
        static_cast<int>(lround(s)),
        static_cast<int>(lround(v))
    );
}

String _toHSV() {
    char buffer[64] { 0 };
    _toHSV(buffer, sizeof(buffer));
    return buffer;
}

void _toLong(char * color, size_t len, bool target) {

    if (!_light_has_color) return;

    snprintf_P(color, len, PSTR("%u,%u,%u"),
        (target ? _light_channels[0].target : _light_channels[0].inputValue),
        (target ? _light_channels[1].target : _light_channels[1].inputValue),
        (target ? _light_channels[2].target : _light_channels[2].inputValue)
    );

}

void _toLong(char * color, size_t len) {
    _toLong(color, len, false);
}

String _toLong(bool target = false) {
    char buffer[64] { 0 };
    _toLong(buffer, sizeof(buffer), target);
    return buffer;
}

String _toCSV(bool target) {
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

void _lightAdjustChannel(unsigned char id, const char* payload) {
    lightChannel(id, _lightAdjustValue(lightChannel(id), payload));
}

void _lightAdjustChannel(unsigned char id, const String& payload) {
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

    return pgm_read_byte(&gamma[value]);
}

class LightTransitionHandler {
public:
    using Channels = std::vector<channel_t>;

    struct Transition {
        float& value;
        unsigned char target;
        float step;
        size_t count;

        void debug() const {
            DEBUG_MSG_P(PSTR("[LIGHT] Transition from %s to %u (step %s, %u times)\n"),
                String(value, 2).c_str(), target, String(step, 2).c_str(), count);
        }
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

        channel.target = target_state ? channel.value : Light::ValueMin;
        if (channel.gamma) {
            channel.target = _lightGammaMap(channel.target);
        }

        if (channel.inverse) {
            channel.target = Light::ValueMax - channel.target;
        }

        float diff = static_cast<float>(channel.target) - channel.current;
        if (isImmediate(target_state, diff)) {
            Transition transition { channel.current, channel.target, diff, 1};
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

        Transition transition { channel.current, channel.target, step, count };
        transition.debug();

        _transitions.push_back(std::move(transition));

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

        for (unsigned char index = 0; index < _transitions.size(); ++index) {
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
void _lightProviderHandleValue(unsigned char channel, float value) {
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

void _lightProviderHandleValue(unsigned char channel, float value) {
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
        _brightness = (value >> (8ull * 5ull));

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
        return (static_cast<uint64_t>(_mireds) << (8ull * 6ull))
            | (static_cast<uint64_t>(_brightness) << (8ull * 5ull))
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
    long _mireds { (Light::MiredsWarm + Light::MiredsCold) / 2L };
};

bool lightSave() {
    return _light_save;
}

void lightSave(bool save) {
    _light_save = save;
}

void _lightSaveRtcmem() {
    auto channels = LightRtcmem::defaultChannels();
    for (unsigned char channel = 0; channel < lightChannels(); ++channel) {
        channels[channel] = _light_channels[channel].inputValue;
    }

    LightRtcmem light(channels, _light_brightness, _light_mireds);
    Rtcmem->light = light.serialize();
}

void _lightRestoreRtcmem() {
    uint64_t value = Rtcmem->light;
    LightRtcmem light(value);

    auto& channels = light.channels();
    for (unsigned char channel = 0; channel < lightChannels(); ++channel) {
        _light_channels[channel].inputValue = channels[channel];
    }

    _light_brightness = light.brightness();
    _light_mireds = light.mireds();
}

void _lightSaveSettings() {
    if (!_light_save) {
        return;
    }

    for (unsigned char channel = 0; channel < lightChannels(); ++channel) {
        setSetting({"ch", channel}, _light_channels[channel].inputValue);
    }

    setSetting("brightness", _light_brightness);
    setSetting("mireds", _light_mireds);

    saveSettings();
}

void _lightRestoreSettings() {
    for (unsigned char channel = 0; channel < lightChannels(); ++channel) {
        auto value = getSetting({"ch", channel}, (channel == 0) ? Light::ValueMax : Light::ValueMin);
        _light_channels[channel].inputValue = value;
    }

    _light_brightness = getSetting("brightness", Light::BrightnessMax);
    _light_mireds = getSetting("mireds", _light_mireds);
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

bool _lightTryParseChannel(const char* p, unsigned char& id) {
    char* endp { nullptr };
    const unsigned long result { strtoul(p, &endp, 10) };
    if ((endp == p) || (*endp != '\0') || (result >= lightChannels())) {
        DEBUG_MSG_P(PSTR("[LIGHT] Invalid channelID (%s)\n"), p);
        return false;
    }

    id = result;
    return true;
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
            lightColor(payload, true);
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
        if (t.equals(MQTT_TOPIC_COLOR_RGB)) {
            lightColor(payload, true);
            _lightUpdateFromMqtt();
            return;
        }
        if (t.equals(MQTT_TOPIC_COLOR_HSV)) {
            lightColor(payload, false);
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
            unsigned char id;
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

        // Color
        if (getSetting("useCSS", 1 == LIGHT_USE_CSS)) {
            _toRGB(buffer, sizeof(buffer), true);
        } else {
            _toLong(buffer, sizeof(buffer), true);
        }
        mqttSend(MQTT_TOPIC_COLOR_RGB, buffer);

        _toHSV(buffer, sizeof(buffer));
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
        mqttSendRaw(mqtt_group_color.c_str(), _toCSV(false).c_str());
    }
}

#endif

// -----------------------------------------------------------------------------
// Broker
// -----------------------------------------------------------------------------

#if BROKER_SUPPORT

void lightBroker() {
    for (unsigned int id = 0; id < _light_channels.size(); ++id) {
        StatusBroker::Publish(MQTT_TOPIC_CHANNEL, id, _light_channels[id].value);
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
    unsigned char id;
    if (!_lightTryParseChannel(id_param.c_str(), id)) {
        return false;
    }

    return callback(id);
}

void _lightApiSetup() {

    if (_light_has_color) {

        apiRegister(F(MQTT_TOPIC_COLOR_RGB),
            [](ApiRequest& request) {
                auto result = getSetting("useCSS", 1 == LIGHT_USE_CSS)
                    ? _toRGB(true) : _toLong(true);
                request.send(result);
                return true;
            },
            [](ApiRequest& request) {
                lightColor(request.param(F("value")), true);
                lightUpdate();
                return true;
            }
        );

        apiRegister(F(MQTT_TOPIC_COLOR_HSV),
            [](ApiRequest& request) {
                request.send(_toHSV());
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
            return _lightApiTryHandle(request, [&](unsigned char id) {
                request.send(String(static_cast<int>(_light_channels[id].target)));
                return true;
            });
        },
        [](ApiRequest& request) {
            return _lightApiTryHandle(request, [&](unsigned char id) {
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
        if (getSetting("useRGB", 1 == LIGHT_USE_RGB)) {
            root["rgb"] = lightColor(true);
        } else {
            root["hsv"] = lightColor(false);
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
    for (unsigned char id=0; id < _light_channels.size(); id++) {
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
    root["useCSS"] = getSetting("useCSS", 1 == LIGHT_USE_CSS);
    root["useRGB"] = getSetting("useRGB", 1 == LIGHT_USE_RGB);
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
                lightColor(data["rgb"].as<const char*>(), true);
                lightUpdate();
            }
            if (data.containsKey("hsv")) {
                lightColor(data["hsv"].as<const char*>(), false);
                lightUpdate();
            }
        }
    }

    if (_light_use_cct) {
      if (strcmp(action, "mireds") == 0) {
          _fromMireds(data["mireds"]);
          lightUpdate();
      }
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

        auto id = -1;
        if (ctx.argc > 1) {
            id = ctx.argv[1].toInt();
        }

        auto description = [&](unsigned char channel) {
            ctx.output.printf("#%hhu (%s): %hhu\n", channel, _lightDesc(channels, channel), _light_channels[channel].inputValue);
        };

        if (id < 0 || id >= static_cast<decltype(id)>(channels)) {
            for (unsigned char index = 0; index < channels; ++index) {
                description(index);
            }
            terminalOK(ctx);
            return;
        }

        if (ctx.argc > 2) {
            _lightAdjustChannel(id, ctx.argv[2].c_str());
            lightUpdate();
        }

        description(id);
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("COLOR"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc > 1) {
            lightColor(ctx.argv[1].c_str());
            lightUpdate();
        }
        ctx.output.printf("%s\n", lightColor().c_str());
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("KELVIN"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc > 1) {
            _lightAdjustKelvin(ctx.argv[1].c_str());
            lightUpdate();
        }
        ctx.output.printf("%ld\n", _toKelvin(_light_mireds));
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("MIRED"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc > 1) {
            _lightAdjustMireds(ctx.argv[1]);
            lightUpdate();
        }
        ctx.output.printf("%ld\n", _light_mireds);
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

// -----------------------------------------------------------------------------

void lightSetReportListener(LightReportListener func) {
    _light_report = func;
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

#if BROKER_SUPPORT
    if (report & Light::Report::Broker) {
        lightBroker();
    }
#endif

    if (_light_report) {
        _light_report();
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

void lightState(unsigned char id, bool state) {
    if (id >= _light_channels.size()) return;
    if (_light_channels[id].state != state) {
        _light_channels[id].state = state;
        _light_state_changed = true;
    }
}

bool lightState(unsigned char id) {
    if (id >= _light_channels.size()) return false;
    return _light_channels[id].state;
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

void lightColor(const char * color, bool rgb) {
    DEBUG_MSG_P(PSTR("[LIGHT] %s: %s\n"), rgb ? "RGB" : "HSV", color);
    if (rgb) {
        _fromRGB(color);
    } else {
        _fromHSV(color);
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
    _fromLong(color, false);
}

String lightColor(bool rgb) {
    char str[12];
    if (rgb) {
        _toRGB(str, sizeof(str));
    } else {
        _toHSV(str, sizeof(str));
    }
    return String(str);
}

String lightColor() {
    return lightColor(true);
}

long lightChannel(unsigned char id) {
    if (id >= _light_channels.size()) return 0;
    return _light_channels[id].inputValue;
}

void lightChannel(unsigned char id, long value) {
    if (id >= _light_channels.size()) return;
    _setInputValue(id, constrain(value, Light::ValueMin, Light::ValueMax));
}

void lightChannelStep(unsigned char id, long steps, long multiplier) {
    lightChannel(id, static_cast<int>(lightChannel(id)) + (steps * multiplier));
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

inline bool _lightUseGamma(size_t channels, unsigned char index) {
    switch (_lightTag(channels, index)) {
    case 'R':
    case 'G':
    case 'B':
        return true;
    }

    return false;
}

inline bool _lightUseGamma(unsigned char index) {
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
    for (unsigned char index = 0; index < lightChannels(); ++index) {
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
        for (unsigned char index = 0; index < channels; ++index) {
            _light_channels.emplace_back(GPIO_NONE);
        }
    }
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
    {
        // Initial duty value (will be passed to pwm_set_duty(...), OFF in this case)
        uint32_t pwm_duty_init[Light::ChannelsMax] = {0};

        // 3-tuples of MUX_REGISTER, MUX_VALUE and GPIO number
        uint32_t io_info[Light::ChannelsMax][3];

        for (unsigned char index = 0; index < Light::ChannelsMax; ++index) {

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
