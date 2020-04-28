/*

LIGHT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "light.h"

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#include "api.h"
#include "broker.h"
#include "mqtt.h"
#include "rtcmem.h"
#include "tuya.h"
#include "ws.h"

#include "light_config.h"

#include <Ticker.h>
#include <Schedule.h>
#include <ArduinoJson.h>
#include <vector>

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

// -----------------------------------------------------------------------------

struct channel_t {

    channel_t();
    channel_t(unsigned char pin, bool inverse);

    unsigned char pin;           // real GPIO pin
    bool inverse;                // whether we should invert the value before using it
    bool state;                  // is the channel ON
    unsigned char inputValue;    // raw value, without the brightness
    unsigned char value;         // normalized value, including brightness
    unsigned char target;        // target value
    double current;              // transition value

};

Ticker _light_comms_ticker;
Ticker _light_save_ticker;
Ticker _light_transition_ticker;

std::vector<channel_t> _light_channels;

bool _light_has_color = false;
bool _light_use_white = false;
bool _light_use_cct = false;
bool _light_use_gamma = false;

bool _light_provider_update = false;

bool _light_use_transitions = false;
unsigned int _light_transition_time = LIGHT_TRANSITION_TIME;

bool _light_dirty = false;
bool _light_state = false;
unsigned char _light_brightness = Light::BRIGHTNESS_MAX;

// Default to the Philips Hue value that HA also use.
// https://developers.meethue.com/documentation/core-concepts
long _light_cold_mireds = LIGHT_COLDWHITE_MIRED;
long _light_warm_mireds = LIGHT_WARMWHITE_MIRED;

long _light_cold_kelvin = (1000000L / _light_cold_mireds);
long _light_warm_kelvin = (1000000L / _light_warm_mireds);

long _light_mireds = lround((_light_cold_mireds + _light_warm_mireds) / 2L);

using light_brightness_func_t = void();
light_brightness_func_t* _light_brightness_func = nullptr;

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
#include <my92xx.h>
my92xx * _my92xx;
unsigned char _light_channel_map[] {
    MY92XX_MAPPING
};
#endif

// UI hint about channel distribution
const char _light_channel_desc[5][5] PROGMEM = {
    {'W',   0,   0,   0,   0},
    {'W', 'C',   0,   0,   0},
    {'R', 'G', 'B',   0,   0},
    {'R', 'G', 'B', 'W',   0},
    {'R', 'G', 'B', 'W', 'C'}
};
static_assert((LIGHT_CHANNELS * LIGHT_CHANNELS) <= (sizeof(_light_channel_desc)), "Out-of-bounds array access");

// Gamma Correction lookup table (8 bit)
const unsigned char _light_gamma_table[] PROGMEM = {
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
static_assert(Light::VALUE_MAX <= sizeof(_light_gamma_table), "Out-of-bounds array access");

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

channel_t::channel_t() :
    pin(GPIO_NONE),
    inverse(false),
    state(true),
    inputValue(0),
    value(0),
    target(0),
    current(0.0)
{}

channel_t::channel_t(unsigned char pin, bool inverse) :
    pin(pin),
    inverse(inverse),
    state(true),
    inputValue(0),
    value(0),
    target(0),
    current(0.0)
{
    pinMode(pin, OUTPUT);
}

void _setValue(const unsigned char id, const unsigned int value) {
    if (_light_channels[id].value != value) {
        _light_channels[id].value = value;
        _light_dirty = true;
    }
}

void _setInputValue(const unsigned char id, const unsigned int value) {
    _light_channels[id].inputValue = value;
}

void _setRGBInputValue(unsigned char red, unsigned char green, unsigned char blue) {
    _setInputValue(0, constrain(red, Light::VALUE_MIN, Light::VALUE_MAX));
    _setInputValue(1, constrain(green, Light::VALUE_MIN, Light::VALUE_MAX));
    _setInputValue(2, constrain(blue, Light::VALUE_MIN, Light::VALUE_MAX));
}

void _setCCTInputValue(unsigned char warm, unsigned char cold) {
    _setInputValue(0, constrain(warm, Light::VALUE_MIN, Light::VALUE_MAX));
    _setInputValue(1, constrain(cold, Light::VALUE_MIN, Light::VALUE_MAX));
}

void _lightApplyBrightness(size_t channels = lightChannels()) {

    double brightness = static_cast<double>(_light_brightness) / static_cast<double>(Light::BRIGHTNESS_MAX);

    channels = std::min(channels, lightChannels());

    for (unsigned char i=0; i < lightChannels(); i++) {
        if (i >= channels) brightness = 1;
        _setValue(i, _light_channels[i].inputValue * brightness);
    }

}

void _lightApplyBrightnessColor() {

    double brightness = static_cast<double>(_light_brightness) / static_cast<double>(Light::BRIGHTNESS_MAX);

    // Substract the common part from RGB channels and add it to white channel. So [250,150,50] -> [200,100,0,50]
    unsigned char white = std::min(_light_channels[0].inputValue, std::min(_light_channels[1].inputValue, _light_channels[2].inputValue));
    for (unsigned int i=0; i < 3; i++) {
        _setValue(i, _light_channels[i].inputValue - white);
    }

    // Split the White Value across 2 White LED Strips.
    if (_light_use_cct) {

        // This change the range from 153-500 to 0-347 so we get a value between 0 and 1 in the end.
        double miredFactor = ((double) _light_mireds - (double) _light_cold_mireds)/((double) _light_warm_mireds - (double) _light_cold_mireds);

        // set cold white
        _light_channels[3].inputValue = 0;
        _setValue(3, lround(((double) 1.0 - miredFactor) * white));

        // set warm white
        _light_channels[4].inputValue = 0;
        _setValue(4, lround(miredFactor * white));
    } else {
        _light_channels[3].inputValue = 0;
        _setValue(3, white);
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
        _setValue(i, lround((double) _light_channels[i].value * factor * brightness));
    }

    // Scale white channel to match brightness
    for (unsigned char i=3; i < channelSize; i++) {
        _setValue(i, constrain(static_cast<unsigned int>(_light_channels[i].value * LIGHT_WHITE_FACTOR), Light::BRIGHTNESS_MIN, Light::BRIGHTNESS_MAX));
    }

    // For the rest of channels, don't apply brightness, it is already in the inputValue
    // i should be 4 when RGBW and 5 when RGBWW
    for (unsigned char i=channelSize; i < _light_channels.size(); i++) {
        _setValue(i, _light_channels[i].inputValue);
    }

}

String lightDesc(unsigned char id) {
    if (id >= _light_channels.size()) return FPSTR(pstr_unknown);

    const char tag = pgm_read_byte(&_light_channel_desc[_light_channels.size() - 1][id]);
    switch (tag) {
        case 'W': return F("WARM WHITE");
        case 'C': return F("COLD WHITE");
        case 'R': return F("RED");
        case 'G': return F("GREEN");
        case 'B': return F("BLUE");
        default: break;
    }

    return FPSTR(pstr_unknown);
}

// -----------------------------------------------------------------------------
// Input Values
// -----------------------------------------------------------------------------

void _fromLong(unsigned long value, bool brightness) {
    if (brightness) {
        _setRGBInputValue((value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF);
        lightBrightness((value & 0xFF) * Light::BRIGHTNESS_MAX / 255);
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

    _light_brightness = lround((double) value[2] * (static_cast<double>(Light::BRIGHTNESS_MAX) / 100.0)); // (default 255/100)
    const unsigned char p = lround(Light::VALUE_MAX * (1.0 - s));
    const unsigned char q = lround(Light::VALUE_MAX * (1.0 - s * f));
    const unsigned char t = lround(Light::VALUE_MAX * (1.0 - s * (1.0 - f)));

    switch (int(h)) {
        case 0:
            _setRGBInputValue(Light::VALUE_MAX, t, p);
            break;
        case 1:
            _setRGBInputValue(q, Light::VALUE_MAX, p);
            break;
        case 2:
            _setRGBInputValue(p, Light::VALUE_MAX, t);
            break;
        case 3:
            _setRGBInputValue(p, q, Light::VALUE_MAX);
            break;
        case 4:
            _setRGBInputValue(t, p, Light::VALUE_MAX);
            break;
        case 5:
            _setRGBInputValue(Light::VALUE_MAX, p, q);
            break;
        default:
            _setRGBInputValue(Light::VALUE_MIN, Light::VALUE_MIN, Light::VALUE_MIN);
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
        lround(factor * Light::VALUE_MAX),
        lround(((double) 1.0 - factor) * Light::VALUE_MAX)
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
      _setRGBInputValue(Light::VALUE_MAX, Light::VALUE_MAX, Light::VALUE_MAX);
      return;
    }

    // Calculate colors
    kelvin /= 100;
    const unsigned int red = (kelvin <= 66)
        ? Light::VALUE_MAX
        : 329.698727446 * fs_pow((double) (kelvin - 60), -0.1332047592);
    const unsigned int green = (kelvin <= 66)
        ? 99.4708025861 * fs_log(kelvin) - 161.1195681661
        : 288.1221695283 * fs_pow((double) kelvin, -0.0755148492);
    const unsigned int blue = (kelvin >= 66)
        ? Light::VALUE_MAX
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

void _toHSV(char * hsv, size_t len) {
    double h {0.}, s {0.}, v {0.};
    double r {0.}, g {0.}, b {0.};
    double min {0.}, max {0.};

    r = static_cast<double>(_light_channels[0].target) / Light::VALUE_MAX;
    g = static_cast<double>(_light_channels[1].target) / Light::VALUE_MAX;
    b = static_cast<double>(_light_channels[2].target) / Light::VALUE_MAX;

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

// See cores/esp8266/WMath.cpp::map
// Redefining as local method here to avoid breaking in unexpected ways in inputs like (0, 0, 0, 0, 1)
template <typename T, typename Tin, typename Tout> T _lightMap(T x, Tin in_min, Tin in_max, Tout out_min, Tout out_max) {
    auto divisor = (in_max - in_min);
    if (divisor == 0){
        return -1; //AVR returns -1, SAM returns 0
    }
    return (x - in_min) * (out_max - out_min) / divisor + out_min;
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

void _lightAdjustBrightness(const char *payload) {
    lightBrightness(_lightAdjustValue(lightBrightness(), payload));
}

void _lightAdjustChannel(unsigned char id, const char *payload) {
    lightChannel(id, _lightAdjustValue(lightChannel(id), payload));
}

void _lightAdjustKelvin(const char *payload) {
    _fromKelvin(_lightAdjustValue(_toKelvin(_light_mireds), payload));
}

void _lightAdjustMireds(const char *payload) {
    _fromMireds(_lightAdjustValue(_light_mireds, payload));
}

// -----------------------------------------------------------------------------
// PROVIDER
// -----------------------------------------------------------------------------

unsigned int _toPWM(unsigned int value, bool gamma, bool inverse) {
    value = constrain(value, Light::VALUE_MIN, Light::VALUE_MAX);
    if (gamma) value = pgm_read_byte(_light_gamma_table + value);
    if (Light::VALUE_MAX != Light::PWM_LIMIT) value = _lightMap(value, Light::VALUE_MIN, Light::VALUE_MAX, Light::PWM_MIN, Light::PWM_LIMIT);
    if (inverse) value = LIGHT_LIMIT_PWM - value;
    return value;
}

// Returns a PWM value for the given channel ID
unsigned int _toPWM(unsigned char id) {
    bool useGamma = _light_use_gamma && _light_has_color && (id < 3);
    return _toPWM(_light_channels[id].current, useGamma, _light_channels[id].inverse);
}

void _lightTransition(unsigned long step) {

    // Transitions based on current step. If step == 0, then it is the last transition
    for (auto& channel : _light_channels) {
        if (!step) {
            channel.current = channel.target;
        } else {
            channel.current += (double) (channel.target - channel.current) / (step + 1);
        }
    }

}

void _lightProviderScheduleUpdate(unsigned long steps);

void _lightProviderUpdate(unsigned long steps) {

    if (_light_provider_update) return;
    _light_provider_update = true;

    _lightTransition(--steps);

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX

        for (unsigned char i=0; i<_light_channels.size(); i++) {
            _my92xx->setChannel(_light_channel_map[i], _toPWM(i));
        }
        _my92xx->setState(true);
        _my92xx->update();

    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

        for (unsigned int i=0; i < _light_channels.size(); i++) {
            pwm_set_duty(_toPWM(i), i);
        }
        pwm_start();

    #endif

    // This is not the final value, update again
    if (steps) _light_transition_ticker.once_ms(LIGHT_TRANSITION_STEP, _lightProviderScheduleUpdate, steps);

    _light_provider_update = false;

}

void _lightProviderScheduleUpdate(unsigned long steps) {
    schedule_function(std::bind(_lightProviderUpdate, steps));
}

// -----------------------------------------------------------------------------
// PERSISTANCE
// -----------------------------------------------------------------------------

union light_rtcmem_t {
    struct {
        uint8_t channels[Light::ChannelsMax];
        uint8_t brightness;
        uint16_t mired;
    } __attribute__((packed)) packed;
    uint64_t value;
};

void _lightSaveRtcmem() {
    if (lightChannels() > Light::ChannelsMax) return;

    light_rtcmem_t light;

    for (unsigned int i=0; i < lightChannels(); i++) {
        light.packed.channels[i] = _light_channels[i].inputValue;
    }

    light.packed.brightness = _light_brightness;
    light.packed.mired = _light_mireds;

    Rtcmem->light = light.value;
}

void _lightRestoreRtcmem() {
    if (lightChannels() > Light::ChannelsMax) return;

    light_rtcmem_t light;
    light.value = Rtcmem->light;

    for (unsigned int i=0; i < lightChannels(); i++) {
        _light_channels[i].inputValue = light.packed.channels[i];
    }

    _light_brightness = light.packed.brightness;
    _light_mireds = light.packed.mired;
}

void _lightSaveSettings() {
    for (unsigned char i=0; i < _light_channels.size(); ++i) {
        setSetting({"ch", i}, _light_channels[i].inputValue);
    }
    setSetting("brightness", _light_brightness);
    setSetting("mireds", _light_mireds);
    saveSettings();
}

void _lightRestoreSettings() {
    for (unsigned char i=0; i < _light_channels.size(); ++i) {
        _light_channels[i].inputValue = getSetting({"ch", i}, (i == 0) ? Light::VALUE_MAX : 0);
    }
    _light_brightness = getSetting("brightness", Light::BRIGHTNESS_MAX);
    _light_mireds = getSetting("mireds", _light_mireds);
}

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

#if MQTT_SUPPORT
void _lightMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    String mqtt_group_color = getSetting("mqttGroupColor");

    if (type == MQTT_CONNECT_EVENT) {

        mqttSubscribe(MQTT_TOPIC_BRIGHTNESS);

        if (_light_has_color) {
            mqttSubscribe(MQTT_TOPIC_COLOR_RGB);
            mqttSubscribe(MQTT_TOPIC_COLOR_HSV);
            mqttSubscribe(MQTT_TOPIC_TRANSITION);
        }

        if (_light_has_color || _light_use_cct) {
            mqttSubscribe(MQTT_TOPIC_MIRED);
            mqttSubscribe(MQTT_TOPIC_KELVIN);
        }

        // Group color
        if (mqtt_group_color.length() > 0) mqttSubscribeRaw(mqtt_group_color.c_str());

        // Channels
        char buffer[strlen(MQTT_TOPIC_CHANNEL) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_CHANNEL);
        mqttSubscribe(buffer);

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Group color
        if ((mqtt_group_color.length() > 0) && (mqtt_group_color.equals(topic))) {
            lightColor(payload, true);
            lightUpdate(true, mqttForward(), false);
            return;
        }

        // Match topic
        String t = mqttMagnitude((char *) topic);

        // Color temperature in mireds
        if (t.equals(MQTT_TOPIC_MIRED)) {
            _lightAdjustMireds(payload);
            lightUpdate(true, mqttForward());
            return;
        }

        // Color temperature in kelvins
        if (t.equals(MQTT_TOPIC_KELVIN)) {
            _lightAdjustKelvin(payload);
            lightUpdate(true, mqttForward());
            return;
        }

        // Color
        if (t.equals(MQTT_TOPIC_COLOR_RGB)) {
            lightColor(payload, true);
            lightUpdate(true, mqttForward());
            return;
        }
        if (t.equals(MQTT_TOPIC_COLOR_HSV)) {
            lightColor(payload, false);
            lightUpdate(true, mqttForward());
            return;
        }

        // Brightness
        if (t.equals(MQTT_TOPIC_BRIGHTNESS)) {
            _lightAdjustBrightness(payload);
            lightUpdate(true, mqttForward());
            return;
        }

        // Transitions
        if (t.equals(MQTT_TOPIC_TRANSITION)) {
            lightTransitionTime(atol(payload));
            return;
        }

        // Channel
        if (t.startsWith(MQTT_TOPIC_CHANNEL)) {
            unsigned int channelID = t.substring(strlen(MQTT_TOPIC_CHANNEL)+1).toInt();
            if (channelID >= _light_channels.size()) {
                DEBUG_MSG_P(PSTR("[LIGHT] Wrong channelID (%d)\n"), channelID);
                return;
            }
            _lightAdjustChannel(channelID, payload);
            lightUpdate(true, mqttForward());
            return;
        }

    }

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
      
      // Mireds
      snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _light_mireds);
      mqttSend(MQTT_TOPIC_MIRED, buffer);
    
    }

    // Channels
    for (unsigned int i=0; i < _light_channels.size(); i++) {
        itoa(_light_channels[i].target, buffer, 10);
        mqttSend(MQTT_TOPIC_CHANNEL, i, buffer);
    }

    // Brightness
    snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _light_brightness);
    mqttSend(MQTT_TOPIC_BRIGHTNESS, buffer);

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

void _lightAPISetup() {

    if (_light_has_color) {

        apiRegister(MQTT_TOPIC_COLOR_RGB,
            [](char * buffer, size_t len) {
                if (getSetting("useCSS", 1 == LIGHT_USE_CSS)) {
                    _toRGB(buffer, len, true);
                } else {
                    _toLong(buffer, len, true);
                }
            },
            [](const char * payload) {
                lightColor(payload, true);
                lightUpdate(true, true);
            }
        );

        apiRegister(MQTT_TOPIC_COLOR_HSV,
            [](char * buffer, size_t len) {
                _toHSV(buffer, len);
            },
            [](const char * payload) {
                lightColor(payload, false);
                lightUpdate(true, true);
            }
        );

        apiRegister(MQTT_TOPIC_KELVIN,
            [](char * buffer, size_t len) {},
            [](const char * payload) {
                _lightAdjustKelvin(payload);
                lightUpdate(true, true);
            }
        );

        apiRegister(MQTT_TOPIC_MIRED,
            [](char * buffer, size_t len) {},
            [](const char * payload) {
                _lightAdjustMireds(payload);
                lightUpdate(true, true);
            }
        );

    }

    for (unsigned int id=0; id<_light_channels.size(); id++) {

        char key[15];
        snprintf_P(key, sizeof(key), PSTR("%s/%d"), MQTT_TOPIC_CHANNEL, id);
        apiRegister(key,
            [id](char * buffer, size_t len) {
                snprintf_P(buffer, len, PSTR("%d"), _light_channels[id].target);
            },
            [id](const char * payload) {
                _lightAdjustChannel(id, payload);
                lightUpdate(true, true);
            }
        );

    }

    apiRegister(MQTT_TOPIC_TRANSITION,
        [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), lightTransitionTime());
        },
        [](const char * payload) {
            lightTransitionTime(atol(payload));
        }
    );

    apiRegister(MQTT_TOPIC_BRIGHTNESS,
        [](char * buffer, size_t len) {
            snprintf_P(buffer, len, PSTR("%d"), _light_brightness);
        },
        [](const char * payload) {
            _lightAdjustBrightness(payload);
            lightUpdate(true, true);
        }
    );

}

#endif // API_SUPPORT


#if WEB_SUPPORT

bool _lightWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    if (strncmp(key, "light", 5) == 0) return true;
    if (strncmp(key, "use", 3) == 0) return true;
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
    root["lightTime"] = _light_transition_time;

    _lightWebSocketStatus(root);
}

void _lightWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {

    if (_light_has_color) {
        if (strcmp(action, "color") == 0) {
            if (data.containsKey("rgb")) {
                lightColor(data["rgb"], true);
                lightUpdate(true, true);
            }
            if (data.containsKey("hsv")) {
                lightColor(data["hsv"], false);
                lightUpdate(true, true);
            }
        }
    }

    if (_light_use_cct) {
      if (strcmp(action, "mireds") == 0) {
          _fromMireds(data["mireds"]);
          lightUpdate(true, true);
      }
    }


    if (strcmp(action, "channel") == 0) {
        if (data.containsKey("id") && data.containsKey("value")) {
            lightChannel(data["id"].as<unsigned char>(), data["value"].as<int>());
            lightUpdate(true, true);
        }
    }

    if (strcmp(action, "brightness") == 0) {
        if (data.containsKey("value")) {
            lightBrightness(data["value"].as<int>());
            lightUpdate(true, true);
        }
    }

}

#endif

#if TERMINAL_SUPPORT

void _lightChannelDebug(unsigned char id) {
    DEBUG_MSG_P(PSTR("Channel #%u (%s): %d\n"), id, lightDesc(id).c_str(), lightChannel(id));
}

void _lightInitCommands() {

    terminalRegisterCommand(F("BRIGHTNESS"), [](Embedis* e) {
        if (e->argc > 1) {
            _lightAdjustBrightness(e->argv[1]);
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Brightness: %u\n"), lightBrightness());
        terminalOK();
    });

    terminalRegisterCommand(F("CHANNEL"), [](Embedis* e) {
        if (!lightChannels()) return;

        auto id = -1;
        if (e->argc > 1) {
            id = String(e->argv[1]).toInt();
        }

        if (id < 0 || id >= static_cast<decltype(id)>(lightChannels())) {
            for (unsigned char index = 0; index < lightChannels(); ++index) {
                _lightChannelDebug(index);
            }
            return;
        }

        if (e->argc > 2) {
            _lightAdjustChannel(id, e->argv[2]);
            lightUpdate(true, true);
        }

        _lightChannelDebug(id);

        terminalOK();
    });

    terminalRegisterCommand(F("COLOR"), [](Embedis* e) {
        if (e->argc > 1) {
            lightColor(e->argv[1]);
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        terminalOK();
    });

    terminalRegisterCommand(F("KELVIN"), [](Embedis* e) {
        if (e->argc > 1) {
            _lightAdjustKelvin(e->argv[1]);
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        terminalOK();
    });

    terminalRegisterCommand(F("MIRED"), [](Embedis* e) {
        if (e->argc > 1) {
            _lightAdjustMireds(e->argv[1]);
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        terminalOK();
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

void _lightComms(unsigned char mask) {

    // Report color and brightness to MQTT broker
    #if MQTT_SUPPORT
        if (mask & Light::COMMS_NORMAL) lightMQTT();
        if (mask & Light::COMMS_GROUP) lightMQTTGroup();
    #endif

    // Report color to WS clients (using current brightness setting)
    #if WEB_SUPPORT
        wsPost(_lightWebSocketStatus);
    #endif

    // Report channels to local broker
    #if BROKER_SUPPORT
        lightBroker();
    #endif

}

void lightUpdate(bool save, bool forward, bool group_forward) {

    // Calculate values based on inputs and brightness
    _light_brightness_func();

    // Only update if a channel has changed
    if (!_light_dirty) return;
    _light_dirty = false;

    // Update channels
    for (unsigned int i=0; i < _light_channels.size(); i++) {
        _light_channels[i].target = _light_state && _light_channels[i].state ? _light_channels[i].value : 0;
        //DEBUG_MSG_P("[LIGHT] Channel #%u target value: %u\n", i, _light_channels[i].target);
    }

    // Channel transition will be handled by the provider function
    // User can configure total transition time, step time is a fixed value
    const unsigned long steps = _light_use_transitions ? _light_transition_time / LIGHT_TRANSITION_STEP : 1;
    _light_transition_ticker.once_ms(LIGHT_TRANSITION_STEP, _lightProviderScheduleUpdate, steps);

    // Delay every communication 100ms to avoid jamming
    const unsigned char mask =
        ((forward) ? Light::COMMS_NORMAL : Light::COMMS_NONE) |
        ((group_forward) ? Light::COMMS_GROUP : Light::COMMS_NONE);
    _light_comms_ticker.once_ms(LIGHT_COMMS_DELAY, _lightComms, mask);

    _lightSaveRtcmem();

    #if LIGHT_SAVE_ENABLED
        // Delay saving to EEPROM 5 seconds to avoid wearing it out unnecessarily
        if (save) _light_save_ticker.once(LIGHT_SAVE_DELAY, _lightSaveSettings);
    #endif

};

void lightUpdate(bool save, bool forward) {
    lightUpdate(save, forward, true);
}

#if LIGHT_SAVE_ENABLED == 0
void lightSave() {
    _lightSaveSettings();
}
#endif

void lightState(unsigned char id, bool state) {
    if (id >= _light_channels.size()) return;
    if (_light_channels[id].state != state) {
        _light_channels[id].state = state;
        _light_dirty = true;
    }
}

bool lightState(unsigned char id) {
    if (id >= _light_channels.size()) return false;
    return _light_channels[id].state;
}

void lightState(bool state) {
    if (_light_state != state) {
        _light_state = state;
        _light_dirty = true;
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

void lightColor(const char * color) {
    lightColor(color, true);
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
    _setInputValue(id, constrain(value, Light::VALUE_MIN, Light::VALUE_MAX));
}

void lightChannelStep(unsigned char id, long steps, long multiplier) {
    lightChannel(id, static_cast<int>(lightChannel(id)) + (steps * multiplier));
}

long lightBrightness() {
    return _light_brightness;
}

void lightBrightness(long brightness) {
    _light_brightness = constrain(brightness, Light::BRIGHTNESS_MIN, Light::BRIGHTNESS_MAX);
}

void lightBrightnessStep(long steps, long multiplier) {
    lightBrightness(static_cast<int>(_light_brightness) + (steps * multiplier));
}

unsigned int lightTransitionTime() {
    if (_light_use_transitions) {
        return _light_transition_time;
    } else {
        return 0;
    }
}

void lightTransitionTime(unsigned long ms) {
    if (0 == ms) {
        _light_use_transitions = false;
    } else {
        _light_use_transitions = true;
        _light_transition_time = ms;
    }
    setSetting("useTransitions", _light_use_transitions);
    setSetting("lightTime", _light_transition_time);
    saveSettings();
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

void _lightConfigure() {

    _light_has_color = getSetting("useColor", 1 == LIGHT_USE_COLOR);
    if (_light_has_color && (_light_channels.size() < 3)) {
        _light_has_color = false;
        setSetting("useColor", _light_has_color);
    }

    _light_use_white = getSetting("useWhite", 1 == LIGHT_USE_WHITE);
    if (_light_use_white && (_light_channels.size() < 4) && (_light_channels.size() != 2)) {
        _light_use_white = false;
        setSetting("useWhite", _light_use_white);
    }

    if (_light_has_color) {
        if (_light_use_white) {
            _light_brightness_func = _lightApplyBrightnessColor;
        } else {
            _light_brightness_func = []() { _lightApplyBrightness(3); };
        }
    } else {
        _light_brightness_func = []() { _lightApplyBrightness(); };
    }

    _light_use_cct = getSetting("useCCT", 1 == LIGHT_USE_CCT);
    if (_light_use_cct && (((_light_channels.size() < 5) && (_light_channels.size() != 2)) || !_light_use_white)) {
        _light_use_cct = false;
        setSetting("useCCT", _light_use_cct);
    }

    _light_cold_mireds = getSetting("lightColdMired", LIGHT_COLDWHITE_MIRED);
    _light_warm_mireds = getSetting("lightWarmMired", LIGHT_WARMWHITE_MIRED);
    _light_cold_kelvin = (1000000L / _light_cold_mireds);
    _light_warm_kelvin = (1000000L / _light_warm_mireds);

    _light_use_gamma = getSetting("useGamma", 1 == LIGHT_USE_GAMMA);
    _light_use_transitions = getSetting("useTransitions", 1 == LIGHT_USE_TRANSITIONS);
    _light_transition_time = getSetting("lightTime", LIGHT_TRANSITION_TIME);

}

// Dummy channel setup for light providers without real GPIO
void lightSetupChannels(unsigned char size) {

    size = constrain(size, 0, Light::ChannelsMax);
    if (size == _light_channels.size()) return;
    _light_channels.resize(size);

}

void lightSetup() {

    const auto enable_pin = getSetting("ltEnableGPIO", _lightEnablePin());
    if (enable_pin != GPIO_NONE) {
        pinMode(enable_pin, OUTPUT);
        digitalWrite(enable_pin, HIGH);
    }

    _light_channels.reserve(Light::ChannelsMax);

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX

        _my92xx = new my92xx(MY92XX_MODEL, MY92XX_CHIPS, MY92XX_DI_PIN, MY92XX_DCKI_PIN, MY92XX_COMMAND);
        lightSetupChannels(LIGHT_CHANNELS);

    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

        // Initial duty value (will be passed to pwm_set_duty(...), OFF in this case)
        uint32_t pwm_duty_init[Light::ChannelsMax] = {0};

        // 3-tuples of MUX_REGISTER, MUX_VALUE and GPIO number
        uint32_t io_info[Light::ChannelsMax][3];

        for (unsigned char index = 0; index < Light::ChannelsMax; ++index) {

            // Load up until first GPIO_NONE. Allow settings to override, but not remove values
            const auto pin = getSetting({"ltDimmerGPIO", index}, _lightChannelPin(index));
            if (!gpioValid(pin)) {
                break;
            }

            _light_channels.emplace_back(pin, getSetting({"ltDimmerInv", index}, _lightInverse(index)));

            io_info[index][0] = pgm_read_dword(&_light_iomux[pin]);
            io_info[index][1] = pgm_read_dword(&_light_iofunc[pin]);
            io_info[index][2] = pin;
            pinMode(pin, OUTPUT);

        }

        // with 0 channels this should not do anything at all and provider will never call pwm_set_duty(...)
        pwm_init(Light::PWM_MAX, pwm_duty_init, _light_channels.size(), io_info);
        pwm_start();

    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
        Tuya::tuyaSetupLight();
    #endif

    DEBUG_MSG_P(PSTR("[LIGHT] LIGHT_PROVIDER = %d\n"), LIGHT_PROVIDER);
    DEBUG_MSG_P(PSTR("[LIGHT] Number of channels: %d\n"), _light_channels.size());

    _lightConfigure();
    if (rtcmemStatus()) {
        _lightRestoreRtcmem();
    } else {
        _lightRestoreSettings();
    }
    lightUpdate(false, false);

    #if WEB_SUPPORT
        wsRegister()
            .onVisible(_lightWebSocketOnVisible)
            .onConnected(_lightWebSocketOnConnected)
            .onAction(_lightWebSocketOnAction)
            .onKeyCheck(_lightWebSocketOnKeyCheck);
    #endif

    #if API_SUPPORT
        _lightAPISetup();
    #endif

    #if MQTT_SUPPORT
        mqttRegister(_lightMQTTCallback);
    #endif

    #if TERMINAL_SUPPORT
        _lightInitCommands();
    #endif

    // Main callbacks
    espurnaRegisterReload([]() {
        #if LIGHT_SAVE_ENABLED == 0
            lightSave();
        #endif
        _lightConfigure();
    });

}

#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
