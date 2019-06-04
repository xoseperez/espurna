/*

LIGHT MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#include <Ticker.h>
#include <ArduinoJson.h>
#include <vector>

extern "C" {
    #include "libs/fs_math.h"
}

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
#define PWM_CHANNEL_NUM_MAX LIGHT_CHANNELS
extern "C" {
    #include "libs/pwm.h"
}
#endif

// -----------------------------------------------------------------------------

Ticker _light_comms_ticker;
Ticker _light_save_ticker;
Ticker _light_transition_ticker;

typedef struct {
    unsigned char pin;
    bool reverse;
    bool state;
    unsigned char inputValue;   // value that has been inputted
    unsigned char value;        // normalized value including brightness
    unsigned char target;       // target value
    double current;             // transition value
} channel_t;
std::vector<channel_t> _light_channel;

bool _light_state = false;
bool _light_use_transitions = false;
unsigned int _light_transition_time = LIGHT_TRANSITION_TIME;
bool _light_has_color = false;
bool _light_use_white = false;
bool _light_use_cct = false;
bool _light_use_gamma = false;
unsigned long _light_steps_left = 1;
unsigned char _light_brightness = LIGHT_MAX_BRIGHTNESS;
unsigned int _light_mireds = round((LIGHT_COLDWHITE_MIRED+LIGHT_WARMWHITE_MIRED)/2);

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
#include <my92xx.h>
my92xx * _my92xx;
ARRAYINIT(unsigned char, _light_channel_map, MY92XX_MAPPING);
#endif

// Gamma Correction lookup table (8 bit)
// TODO: move to PROGMEM
const unsigned char _light_gamma_table[] = {
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

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

void _setRGBInputValue(unsigned char red, unsigned char green, unsigned char blue) {
    _light_channel[0].inputValue = constrain(red, 0, LIGHT_MAX_VALUE);
    _light_channel[1].inputValue = constrain(green, 0, LIGHT_MAX_VALUE);;
    _light_channel[2].inputValue = constrain(blue, 0, LIGHT_MAX_VALUE);;
}

void _setCCTInputValue(unsigned char warm, unsigned char cold) {
    _light_channel[0].inputValue = constrain(warm, 0, LIGHT_MAX_VALUE);
    _light_channel[1].inputValue = constrain(cold, 0, LIGHT_MAX_VALUE);
}

void _generateBrightness() {

    double brightness = (double) _light_brightness / LIGHT_MAX_BRIGHTNESS;

    // Convert RGB to RGBW(W)
    if (_light_has_color && _light_use_white) {

        // Substract the common part from RGB channels and add it to white channel. So [250,150,50] -> [200,100,0,50]
        unsigned char white = std::min(_light_channel[0].inputValue, std::min(_light_channel[1].inputValue, _light_channel[2].inputValue));
        for (unsigned int i=0; i < 3; i++) {
            _light_channel[i].value = _light_channel[i].inputValue - white;
        }

        // Split the White Value across 2 White LED Strips.
        if (_light_use_cct) {

          // This change the range from 153-500 to 0-347 so we get a value between 0 and 1 in the end.
          double miredFactor = ((double) _light_mireds - (double) LIGHT_COLDWHITE_MIRED)/((double) LIGHT_WARMWHITE_MIRED - (double) LIGHT_COLDWHITE_MIRED);

          // set cold white
          _light_channel[3].inputValue = 0;
          _light_channel[3].value = round(((double) 1.0 - miredFactor) * white);

          // set warm white
          _light_channel[4].inputValue = 0;
          _light_channel[4].value = round(miredFactor * white);
        } else {
          _light_channel[3].inputValue = 0;
          _light_channel[3].value = white;
        }

        // Scale up to equal input values. So [250,150,50] -> [200,100,0,50] -> [250, 125, 0, 63]
        unsigned char max_in = std::max(_light_channel[0].inputValue, std::max(_light_channel[1].inputValue, _light_channel[2].inputValue));
        unsigned char max_out = std::max(std::max(_light_channel[0].value, _light_channel[1].value), std::max(_light_channel[2].value, _light_channel[3].value));
        unsigned char channelSize = _light_use_cct ? 5 : 4;

        if (_light_use_cct) {
          max_out = std::max(max_out, _light_channel[4].value);
        }

        double factor = (max_out > 0) ? (double) (max_in / max_out) : 0;
        for (unsigned char i=0; i < channelSize; i++) {
            _light_channel[i].value = round((double) _light_channel[i].value * factor * brightness);
        }

        // Scale white channel to match brightness
        for (unsigned char i=3; i < channelSize; i++) {
            _light_channel[i].value = constrain(_light_channel[i].value * LIGHT_WHITE_FACTOR, 0, LIGHT_MAX_BRIGHTNESS);
        }

        // For the rest of channels, don't apply brightness, it is already in the inputValue
        // i should be 4 when RGBW and 5 when RGBWW
        for (unsigned char i=channelSize; i < _light_channel.size(); i++) {
            _light_channel[i].value = _light_channel[i].inputValue;
        }

    } else {

        // Apply brightness equally to all channels
        for (unsigned char i=0; i < _light_channel.size(); i++) {
            _light_channel[i].value = _light_channel[i].inputValue * brightness;
        }

    }

}

// -----------------------------------------------------------------------------
// Input Values
// -----------------------------------------------------------------------------

void _fromLong(unsigned long value, bool brightness) {
    if (brightness) {
        _setRGBInputValue((value >> 24) & 0xFF, (value >> 16) & 0xFF, (value >> 8) & 0xFF);
        _light_brightness = (value & 0xFF) * LIGHT_MAX_BRIGHTNESS / 255;
    } else {
        _setRGBInputValue((value >> 16) & 0xFF, (value >> 8) & 0xFF, (value) & 0xFF);
    }
}

void _fromRGB(const char * rgb) {
    char * p = (char *) rgb;
    if (strlen(p) == 0) return;

    switch (p[0]) {
      case '#': // HEX Value
        if (_light_has_color) {
            ++p;
            unsigned long value = strtoul(p, NULL, 16);
            // RGBA values are interpreted like RGB + brightness
            _fromLong(value, strlen(p) > 7);
        }
        break;
      case 'M': // Mired Value
          _fromMireds(atol(p + 1));
        break;
      case 'K': // Kelvin Value
          _fromKelvin(atol(p + 1));
        break;
      default: // assume decimal values separated by commas
        char * tok;
        unsigned char count = 0;
        unsigned char channels = _light_channel.size();

        tok = strtok(p, ",");
        while (tok != NULL) {
            _light_channel[count].inputValue = atoi(tok);
            if (++count == channels) break;
            tok = strtok(NULL, ",");
        }

        // RGB but less than 3 values received, assume it is 0
        if (_light_has_color && (count < 3)) {
          // check channel 1 and 2:
          for (int i = 1; i <= 2; i++) {
            if (count < (i+1)) {
              _light_channel[i].inputValue = 0;
            }
          }
        }
        break;
    }
}

// HSV string is expected to be "H,S,V", where:
//   0 <= H <= 360
//   0 <= S <= 100
//   0 <= V <= 100
void _fromHSV(const char * hsv) {

    char * ptr = (char *) hsv;
    if (strlen(ptr) == 0) return;
    if (!_light_has_color) return;

    char * tok;
    unsigned char count = 0;
    unsigned int value[3] = {0};

    tok = strtok(ptr, ",");
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

    double h = (value[0] == 360) ? 0 : (double) value[0] / 60.0;
    double f = (h - floor(h));
    double s = (double) value[1] / 100.0;

    _light_brightness = round((double) value[2] * 2.55); // (255/100)
    unsigned char p = round(255 * (1.0 - s));
    unsigned char q = round(255 * (1.0 - s * f));
    unsigned char t = round(255 * (1.0 - s * (1.0 - f)));

    switch (int(h)) {
        case 0:
            _setRGBInputValue(255, t, p);
            break;
        case 1:
            _setRGBInputValue(q, 255, p);
            break;
        case 2:
            _setRGBInputValue(p, 255, t);
            break;
        case 3:
            _setRGBInputValue(p, q, 255);
            break;
        case 4:
            _setRGBInputValue(t, p, 255);
            break;
        case 5:
            _setRGBInputValue(255, p, q);
            break;
        default:
            _setRGBInputValue(0, 0, 0);
            break;
    }
}

// Thanks to Sacha Telgenhof for sharing this code in his AiLight library
// https://github.com/stelgenhof/AiLight
void _fromKelvin(unsigned long kelvin) {

    if (!_light_has_color) {

      if(!_light_use_cct) return;
      
      _light_mireds = constrain(round(1000000UL / kelvin), LIGHT_MIN_MIREDS, LIGHT_MAX_MIREDS);
      
      // This change the range from 153-500 to 0-347 so we get a value between 0 and 1 in the end.
      double factor = ((double) _light_mireds - (double) LIGHT_COLDWHITE_MIRED)/((double) LIGHT_WARMWHITE_MIRED - (double) LIGHT_COLDWHITE_MIRED);
      unsigned char warm = round(factor * LIGHT_MAX_VALUE);
      unsigned char cold = round(((double) 1.0 - factor) * LIGHT_MAX_VALUE);

      _setCCTInputValue(warm, cold);
      
      return;
    }

    _light_mireds = constrain(round(1000000UL / kelvin), LIGHT_MIN_MIREDS, LIGHT_MAX_MIREDS);

    if (_light_use_cct) {
      _setRGBInputValue(LIGHT_MAX_VALUE, LIGHT_MAX_VALUE, LIGHT_MAX_VALUE);
      return;
    }

    // Calculate colors
    kelvin /= 100;
    unsigned int red = (kelvin <= 66)
        ? LIGHT_MAX_VALUE
        : 329.698727446 * fs_pow((double) (kelvin - 60), -0.1332047592);
    unsigned int green = (kelvin <= 66)
        ? 99.4708025861 * fs_log(kelvin) - 161.1195681661
        : 288.1221695283 * fs_pow((double) kelvin, -0.0755148492);
    unsigned int blue = (kelvin >= 66)
        ? LIGHT_MAX_VALUE
        : ((kelvin <= 19)
            ? 0
            : 138.5177312231 * fs_log(kelvin - 10) - 305.0447927307);

    _setRGBInputValue(red, green, blue);

}

// Color temperature is measured in mireds (kelvin = 1e6/mired)
void _fromMireds(unsigned long mireds) {
    unsigned long kelvin = constrain(1000000UL / mireds, 1000, 40000);
    _fromKelvin(kelvin);
}

// -----------------------------------------------------------------------------
// Output Values
// -----------------------------------------------------------------------------

void _toRGB(char * rgb, size_t len, bool target) {
    unsigned long value = 0;

    value += target ? _light_channel[0].target : _light_channel[0].inputValue;
    value <<= 8;
    value += target ? _light_channel[1].target : _light_channel[1].inputValue;
    value <<= 8;
    value += target ? _light_channel[2].target : _light_channel[2].inputValue;

    snprintf_P(rgb, len, PSTR("#%06X"), value);
}

void _toRGB(char * rgb, size_t len) {
    _toRGB(rgb, len, false);
}

void _toHSV(char * hsv, size_t len, bool target) {
    double h, s, v;
    double brightness = (double) _light_brightness / LIGHT_MAX_BRIGHTNESS;

    double r = (double) ((target ? _light_channel[0].target : _light_channel[0].inputValue) * brightness) / 255.0;
    double g = (double) ((target ? _light_channel[1].target : _light_channel[1].inputValue) * brightness) / 255.0;
    double b = (double) ((target ? _light_channel[2].target : _light_channel[2].inputValue) * brightness) / 255.0;

    double min = std::min(r, std::min(g, b));
    double max = std::max(r, std::max(g, b));

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

    // String
    snprintf_P(hsv, len, PSTR("%d,%d,%d"), round(h), round(s), round(v));
}

void _toHSV(char * hsv, size_t len) {
    _toHSV(hsv, len, false);
}

void _toLong(char * color, size_t len, bool target) {
    
    if (!_light_has_color) return;

    snprintf_P(color, len, PSTR("%d,%d,%d"),
        (int) (target ? _light_channel[0].target : _light_channel[0].inputValue),
        (int) (target ? _light_channel[1].target : _light_channel[1].inputValue),
        (int) (target ? _light_channel[2].target : _light_channel[2].inputValue)
    );

}

void _toLong(char * color, size_t len) {
    _toLong(color, len, false);
}

void _toCSV(char * buffer, size_t len, bool applyBrightness, bool target) {
    char num[10];
    float b = applyBrightness ? (float) _light_brightness / LIGHT_MAX_BRIGHTNESS : 1;
    for (unsigned char i=0; i<_light_channel.size(); i++) {
        itoa((target ? _light_channel[i].target : _light_channel[i].inputValue) * b, num, 10);
        if (i>0) strncat(buffer, ",", len--);
        strncat(buffer, num, len);
        len = len - strlen(num);
    }
}

void _toCSV(char * buffer, size_t len, bool applyBrightness) {
    _toCSV(buffer, len, applyBrightness, false);
}

// -----------------------------------------------------------------------------
// PROVIDER
// -----------------------------------------------------------------------------

unsigned int _toPWM(unsigned long value, bool gamma, bool reverse) {
    value = constrain(value, 0, LIGHT_MAX_VALUE);
    if (gamma) value = _light_gamma_table[value];
    if (LIGHT_MAX_VALUE != LIGHT_LIMIT_PWM) value = map(value, 0, LIGHT_MAX_VALUE, 0, LIGHT_LIMIT_PWM);
    if (reverse) value = LIGHT_LIMIT_PWM - value;
    return value;
}

// Returns a PWM value for the given channel ID
unsigned int _toPWM(unsigned char id) {
    bool useGamma = _light_use_gamma && _light_has_color && (id < 3);
    return _toPWM(_light_channel[id].current, useGamma, _light_channel[id].reverse);
}

void _transition() {

    // Update transition ticker
    _light_steps_left--;
    if (_light_steps_left == 0) _light_transition_ticker.detach();

    // Transitions
    for (unsigned int i=0; i < _light_channel.size(); i++) {

        if (_light_steps_left == 0) {
            _light_channel[i].current = _light_channel[i].target;
        } else {
            double difference = (double) (_light_channel[i].target - _light_channel[i].current) / (_light_steps_left + 1);
            _light_channel[i].current = _light_channel[i].current + difference;
        }

    }

}

void _lightProviderUpdate() {

    _transition();

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX

        for (unsigned char i=0; i<_light_channel.size(); i++) {
            _my92xx->setChannel(_light_channel_map[i], _toPWM(i));
        }
        _my92xx->setState(true);
        _my92xx->update();

    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

        for (unsigned int i=0; i < _light_channel.size(); i++) {
            pwm_set_duty(_toPWM(i), i);
        }
        pwm_start();

    #endif

}

// -----------------------------------------------------------------------------
// PERSISTANCE
// -----------------------------------------------------------------------------

union light_rtcmem_t {
    struct {
        uint8_t channels[5];
        uint8_t brightness;
        uint16_t mired;
    } packed;
    uint64_t value;
};

#define LIGHT_RTCMEM_CHANNELS_MAX sizeof(light_rtcmem_t().packed.channels)

void _lightSaveRtcmem() {
    if (lightChannels() > LIGHT_RTCMEM_CHANNELS_MAX) return;

    light_rtcmem_t light;

    for (unsigned int i=0; i < lightChannels(); i++) {
        light.packed.channels[i] = _light_channel[i].inputValue;
    }

    light.packed.brightness = _light_brightness;
    light.packed.mired = _light_mireds;

    Rtcmem->light = light.value;
}

void _lightRestoreRtcmem() {
    if (lightChannels() > LIGHT_RTCMEM_CHANNELS_MAX) return;

    light_rtcmem_t light;
    light.value = Rtcmem->light;

    for (unsigned int i=0; i < lightChannels(); i++) {
        _light_channel[i].inputValue = light.packed.channels[i];
    }

    _light_brightness = light.packed.brightness;
    _light_mireds = light.packed.mired;
}

void _lightSaveSettings() {
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        setSetting("ch", i, _light_channel[i].inputValue);
    }
    setSetting("brightness", _light_brightness);
    setSetting("mireds", _light_mireds);
    saveSettings();
}

void _lightRestoreSettings() {
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        _light_channel[i].inputValue = getSetting("ch", i, i==0 ? 255 : 0).toInt();
    }
    _light_brightness = getSetting("brightness", LIGHT_MAX_BRIGHTNESS).toInt();
    _light_mireds = getSetting("mireds", _light_mireds).toInt();
    lightUpdate(false, false);
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
        if ((mqtt_group_color.length() > 0) & (mqtt_group_color.equals(topic))) {
            lightColor(payload, true);
            lightUpdate(true, mqttForward(), false);
            return;
        }

        // Match topic
        String t = mqttMagnitude((char *) topic);

        // Color temperature in mireds
        if (t.equals(MQTT_TOPIC_MIRED)) {
            _fromMireds(atol(payload));
            lightUpdate(true, mqttForward());
            return;
        }

        // Color temperature in kelvins
        if (t.equals(MQTT_TOPIC_KELVIN)) {
            _fromKelvin(atol(payload));
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
            _light_brightness = constrain(atoi(payload), 0, LIGHT_MAX_BRIGHTNESS);
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
            if (channelID >= _light_channel.size()) {
                DEBUG_MSG_P(PSTR("[LIGHT] Wrong channelID (%d)\n"), channelID);
                return;
            }
            lightChannel(channelID, atoi(payload));
            lightUpdate(true, mqttForward());
            return;
        }

    }

}

void lightMQTT() {

    char buffer[20];

    if (_light_has_color) {

        // Color
        if (getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1) {
            _toRGB(buffer, sizeof(buffer), true);
        } else {
            _toLong(buffer, sizeof(buffer), true);
        }
        mqttSend(MQTT_TOPIC_COLOR_RGB, buffer);

        _toHSV(buffer, sizeof(buffer), true);
        mqttSend(MQTT_TOPIC_COLOR_HSV, buffer);

    }
    
    if (_light_has_color || _light_use_cct) {
      
      // Mireds
      snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _light_mireds);
      mqttSend(MQTT_TOPIC_MIRED, buffer);
    
    }

    // Channels
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        itoa(_light_channel[i].target, buffer, 10);
        mqttSend(MQTT_TOPIC_CHANNEL, i, buffer);
    }

    // Brightness
    snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _light_brightness);
    mqttSend(MQTT_TOPIC_BRIGHTNESS, buffer);

}

void lightMQTTGroup() {
    String mqtt_group_color = getSetting("mqttGroupColor");
    if (mqtt_group_color.length()>0) {
        char buffer[20];
        _toCSV(buffer, sizeof(buffer), true);
        mqttSendRaw(mqtt_group_color.c_str(), buffer);
    }
}

#endif

// -----------------------------------------------------------------------------
// Broker
// -----------------------------------------------------------------------------

#if BROKER_SUPPORT

void lightBroker() {
    char buffer[10];
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        itoa(_light_channel[i].inputValue, buffer, 10);
        brokerPublish(BROKER_MSG_TYPE_STATUS, MQTT_TOPIC_CHANNEL, i, buffer);
    }
}

#endif

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

unsigned char lightChannels() {
    return _light_channel.size();
}

bool lightHasColor() {
    return _light_has_color;
}

bool lightUseCCT() {
    return _light_use_cct;
}

void _lightComms(unsigned char mask) {

    // Report color & brightness to MQTT broker
    #if MQTT_SUPPORT
        if (mask & 0x01) lightMQTT();
        if (mask & 0x02) lightMQTTGroup();
    #endif

    // Report color to WS clients (using current brightness setting)
    #if WEB_SUPPORT
        wsSend(_lightWebSocketStatus);
    #endif

    // Report channels to local broker
    #if BROKER_SUPPORT
        lightBroker();
    #endif

}

void lightUpdate(bool save, bool forward, bool group_forward) {

    _generateBrightness();

    // Update channels
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        _light_channel[i].target = _light_state && _light_channel[i].state ? _light_channel[i].value : 0;
        //DEBUG_MSG_P("[LIGHT] Channel #%u target value: %u\n", i, _light_channel[i].target);
    }

    // Configure color transition
    _light_steps_left = _light_use_transitions ? _light_transition_time / LIGHT_TRANSITION_STEP : 1;
    _light_transition_ticker.attach_ms(LIGHT_TRANSITION_STEP, _lightProviderUpdate);

    // Delay every communication 100ms to avoid jamming
    unsigned char mask = 0;
    if (forward) mask += 1;
    if (group_forward) mask += 2;
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

void lightState(unsigned char i, bool state) {
    _light_channel[i].state = state;
}

bool lightState(unsigned char i) {
    return _light_channel[i].state;
}

void lightState(bool state) {
    _light_state = state;
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

unsigned int lightChannel(unsigned char id) {
    if (id <= _light_channel.size()) {
        return _light_channel[id].inputValue;
    }
    return 0;
}

void lightChannel(unsigned char id, int value) {
    if (id <= _light_channel.size()) {
        _light_channel[id].inputValue = constrain(value, 0, LIGHT_MAX_VALUE);
    }
}

void lightChannelStep(unsigned char id, int steps) {
    lightChannel(id, lightChannel(id) + steps * LIGHT_STEP);
}

unsigned int lightBrightness() {
    return _light_brightness;
}

void lightBrightness(int b) {
    _light_brightness = constrain(b, 0, LIGHT_MAX_BRIGHTNESS);
}

void lightBrightnessStep(int steps) {
    lightBrightness(_light_brightness + steps * LIGHT_STEP);
}

unsigned long lightTransitionTime() {
    if (_light_use_transitions) {
        return _light_transition_time;
    } else {
        return 0;
    }
}

void lightTransitionTime(unsigned long m) {
    if (0 == m) {
        _light_use_transitions = false;
    } else {
        _light_use_transitions = true;
        _light_transition_time = m;
    }
    setSetting("useTransitions", _light_use_transitions);
    setSetting("lightTime", _light_transition_time);
    saveSettings();
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _lightWebSocketOnReceive(const char * key, JsonVariant& value) {
    if (strncmp(key, "light", 5) == 0) return true;
    if (strncmp(key, "use", 3) == 0) return true;
    return false;
}

void _lightWebSocketStatus(JsonObject& root) {
    if (_light_has_color) {
        if (getSetting("useRGB", LIGHT_USE_RGB).toInt() == 1) {
            root["rgb"] = lightColor(true);
        } else {
            root["hsv"] = lightColor(false);
        }
    }
    if (_light_use_cct) {
        root["useCCT"] = _light_use_cct;
        root["mireds"] = _light_mireds;
    }
    JsonArray& channels = root.createNestedArray("channels");
    for (unsigned char id=0; id < _light_channel.size(); id++) {
        channels.add(lightChannel(id));
    }
    root["brightness"] = lightBrightness();
}

void _lightWebSocketOnSend(JsonObject& root) {
    root["colorVisible"] = 1;
    root["mqttGroupColor"] = getSetting("mqttGroupColor");
    root["useColor"] = _light_has_color;
    root["useWhite"] = _light_use_white;
    root["useGamma"] = _light_use_gamma;
    root["useTransitions"] = _light_use_transitions;
    root["useCSS"] = getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1;
    root["useRGB"] = getSetting("useRGB", LIGHT_USE_RGB).toInt() == 1;
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
            lightChannel(data["id"], data["value"]);
            lightUpdate(true, true);
        }
    }

    if (strcmp(action, "brightness") == 0) {
        if (data.containsKey("value")) {
            lightBrightness(data["value"]);
            lightUpdate(true, true);
        }
    }

}

#endif

#if API_SUPPORT

void _lightAPISetup() {

    if (_light_has_color) {

        apiRegister(MQTT_TOPIC_COLOR_RGB,
            [](char * buffer, size_t len) {
                if (getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1) {
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
                _toHSV(buffer, len, true);
            },
            [](const char * payload) {
                lightColor(payload, false);
                lightUpdate(true, true);
            }
        );

        apiRegister(MQTT_TOPIC_KELVIN,
            [](char * buffer, size_t len) {},
            [](const char * payload) {
                _fromKelvin(atol(payload));
                lightUpdate(true, true);
            }
        );

        apiRegister(MQTT_TOPIC_MIRED,
            [](char * buffer, size_t len) {},
            [](const char * payload) {
                _fromMireds(atol(payload));
                lightUpdate(true, true);
            }
        );

    }

    for (unsigned int id=0; id<_light_channel.size(); id++) {

        char key[15];
        snprintf_P(key, sizeof(key), PSTR("%s/%d"), MQTT_TOPIC_CHANNEL, id);
        apiRegister(key,
            [id](char * buffer, size_t len) {
                snprintf_P(buffer, len, PSTR("%d"), _light_channel[id].target);
            },
            [id](const char * payload) {
                lightChannel(id, atoi(payload));
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
            lightBrightness(atoi(payload));
            lightUpdate(true, true);
        }
    );

}

#endif // API_SUPPORT

#if TERMINAL_SUPPORT

void _lightInitCommands() {

    terminalRegisterCommand(F("BRIGHTNESS"), [](Embedis* e) {
        if (e->argc > 1) {
            const String value(e->argv[1]);
            if( value.length() > 0 ) {
                if( value[0] == '+' || value[0] == '-' ) {
                    lightBrightness(lightBrightness()+String(e->argv[1]).toInt());
                } else {
                    lightBrightness(String(e->argv[1]).toInt());
                }
                lightUpdate(true, true);
            }
        }
        DEBUG_MSG_P(PSTR("Brightness: %d\n"), lightBrightness());
        terminalOK();
    });

    terminalRegisterCommand(F("CHANNEL"), [](Embedis* e) {
        if (e->argc < 2) {
            terminalError(F("Wrong arguments"));
        }
        int id = String(e->argv[1]).toInt();
        if (e->argc > 2) {
            int value = String(e->argv[2]).toInt();
            lightChannel(id, value);
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Channel #%d: %d\n"), id, lightChannel(id));
        terminalOK();
    });

    terminalRegisterCommand(F("COLOR"), [](Embedis* e) {
        if (e->argc > 1) {
            String color = String(e->argv[1]);
            lightColor(color.c_str());
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        terminalOK();
    });

    terminalRegisterCommand(F("KELVIN"), [](Embedis* e) {
        if (e->argc > 1) {
            String color = String("K") + String(e->argv[1]);
            lightColor(color.c_str());
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        terminalOK();
    });

    terminalRegisterCommand(F("MIRED"), [](Embedis* e) {
        if (e->argc > 1) {
            const String value(e->argv[1]);
            String color = String("M");
            if( value.length() > 0 ) {
                if( value[0] == '+' || value[0] == '-' ) {
                    color += String(_light_mireds + String(e->argv[1]).toInt());
                } else {
                    color += String(e->argv[1]);
                }
                lightColor(color.c_str());
                lightUpdate(true, true);
            }
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        terminalOK();
    });

}

#endif // TERMINAL_SUPPORT

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

unsigned long getIOMux(unsigned long gpio) {
    unsigned long muxes[16] = {
        PERIPHS_IO_MUX_GPIO0_U, PERIPHS_IO_MUX_U0TXD_U, PERIPHS_IO_MUX_GPIO2_U, PERIPHS_IO_MUX_U0RXD_U,
        PERIPHS_IO_MUX_GPIO4_U, PERIPHS_IO_MUX_GPIO5_U, PERIPHS_IO_MUX_SD_CLK_U, PERIPHS_IO_MUX_SD_DATA0_U,
        PERIPHS_IO_MUX_SD_DATA1_U, PERIPHS_IO_MUX_SD_DATA2_U, PERIPHS_IO_MUX_SD_DATA3_U, PERIPHS_IO_MUX_SD_CMD_U,
        PERIPHS_IO_MUX_MTDI_U, PERIPHS_IO_MUX_MTCK_U, PERIPHS_IO_MUX_MTMS_U, PERIPHS_IO_MUX_MTDO_U
    };
    return muxes[gpio];
}

unsigned long getIOFunc(unsigned long gpio) {
    unsigned long funcs[16] = {
        FUNC_GPIO0, FUNC_GPIO1, FUNC_GPIO2, FUNC_GPIO3,
        FUNC_GPIO4, FUNC_GPIO5, FUNC_GPIO6, FUNC_GPIO7,
        FUNC_GPIO8, FUNC_GPIO9, FUNC_GPIO10, FUNC_GPIO11,
        FUNC_GPIO12, FUNC_GPIO13, FUNC_GPIO14, FUNC_GPIO15
    };
    return funcs[gpio];
}

#endif

void _lightConfigure() {

    _light_has_color = getSetting("useColor", LIGHT_USE_COLOR).toInt() == 1;
    if (_light_has_color && (_light_channel.size() < 3)) {
        _light_has_color = false;
        setSetting("useColor", _light_has_color);
    }

    _light_use_white = getSetting("useWhite", LIGHT_USE_WHITE).toInt() == 1;
    if (_light_use_white && (_light_channel.size() < 4) && (_light_channel.size() != 2)) {
        _light_use_white = false;
        setSetting("useWhite", _light_use_white);
    }

    _light_use_cct = getSetting("useCCT", LIGHT_USE_CCT).toInt() == 1;
    if (_light_use_cct && (((_light_channel.size() < 5) && (_light_channel.size() != 2)) || !_light_use_white)) {
        _light_use_cct = false;
        setSetting("useCCT", _light_use_cct);
    }

    _light_use_gamma = getSetting("useGamma", LIGHT_USE_GAMMA).toInt() == 1;
    _light_use_transitions = getSetting("useTransitions", LIGHT_USE_TRANSITIONS).toInt() == 1;
    _light_transition_time = getSetting("lightTime", LIGHT_TRANSITION_TIME).toInt();

}

void lightSetup() {

    #ifdef LIGHT_ENABLE_PIN
        pinMode(LIGHT_ENABLE_PIN, OUTPUT);
        digitalWrite(LIGHT_ENABLE_PIN, HIGH);
    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX

        _my92xx = new my92xx(MY92XX_MODEL, MY92XX_CHIPS, MY92XX_DI_PIN, MY92XX_DCKI_PIN, MY92XX_COMMAND);
        for (unsigned char i=0; i<LIGHT_CHANNELS; i++) {
            _light_channel.push_back((channel_t) {0, false, true, 0, 0, 0});
        }

    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

        #ifdef LIGHT_CH1_PIN
            _light_channel.push_back((channel_t) {LIGHT_CH1_PIN, LIGHT_CH1_INVERSE, true, 0, 0, 0});
        #endif

        #ifdef LIGHT_CH2_PIN
            _light_channel.push_back((channel_t) {LIGHT_CH2_PIN, LIGHT_CH2_INVERSE, true, 0, 0, 0});
        #endif

        #ifdef LIGHT_CH3_PIN
            _light_channel.push_back((channel_t) {LIGHT_CH3_PIN, LIGHT_CH3_INVERSE, true, 0, 0, 0});
        #endif

        #ifdef LIGHT_CH4_PIN
            _light_channel.push_back((channel_t) {LIGHT_CH4_PIN, LIGHT_CH4_INVERSE, true, 0, 0, 0});
        #endif

        #ifdef LIGHT_CH5_PIN
            _light_channel.push_back((channel_t) {LIGHT_CH5_PIN, LIGHT_CH5_INVERSE, true, 0, 0, 0});
        #endif

        uint32 pwm_duty_init[PWM_CHANNEL_NUM_MAX];
        uint32 io_info[PWM_CHANNEL_NUM_MAX][3];
        for (unsigned int i=0; i < _light_channel.size(); i++) {
            pwm_duty_init[i] = 0;
            io_info[i][0] = getIOMux(_light_channel[i].pin);
            io_info[i][1] = getIOFunc(_light_channel[i].pin);
            io_info[i][2] = _light_channel[i].pin;
            pinMode(_light_channel[i].pin, OUTPUT);
        }
        pwm_init(LIGHT_MAX_PWM, pwm_duty_init, PWM_CHANNEL_NUM_MAX, io_info);
        pwm_start();


    #endif

    DEBUG_MSG_P(PSTR("[LIGHT] LIGHT_PROVIDER = %d\n"), LIGHT_PROVIDER);
    DEBUG_MSG_P(PSTR("[LIGHT] Number of channels: %d\n"), _light_channel.size());

    _lightConfigure();
    if (rtcmemStatus()) {
        _lightRestoreRtcmem();
    } else {
        _lightRestoreSettings();
    }

    #if WEB_SUPPORT
        wsOnSendRegister(_lightWebSocketOnSend);
        wsOnActionRegister(_lightWebSocketOnAction);
        wsOnReceiveRegister(_lightWebSocketOnReceive);
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
