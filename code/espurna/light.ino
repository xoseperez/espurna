/*

LIGHT MODULE

Copyright (C) 2016-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#include <Ticker.h>
#include <ArduinoJson.h>
#include <vector>

#if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
#define PWM_CHANNEL_NUM_MAX LIGHT_CHANNELS
extern "C" {
    #include "libs/pwm.h"
}
#endif

// -----------------------------------------------------------------------------

Ticker _light_save_ticker;
Ticker _light_transition_ticker;

typedef struct {
    unsigned char pin;
    bool reverse;
    bool state;
    unsigned char inputValue;   // value that has been inputted
    unsigned char value;        // normalized value including brightness
    unsigned char shadow;       // represented value
    double current;             // transition value
} channel_t;
std::vector<channel_t> _light_channel;

bool _light_state = false;
bool _light_use_transitions = false;
unsigned int _light_transition_time = LIGHT_TRANSITION_TIME;
bool _light_has_color = false;
bool _light_use_white = false;
bool _light_use_gamma = false;
unsigned long _light_steps_left = 1;
unsigned char _light_brightness = LIGHT_MAX_BRIGHTNESS;
unsigned int _light_mireds = LIGHT_DEFAULT_MIREDS;

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
    _light_channel[0].inputValue = red;
    _light_channel[1].inputValue = green;
    _light_channel[2].inputValue = blue;
}

void _generateBrightness() {

    double brightness = (double) _light_brightness / LIGHT_MAX_BRIGHTNESS;

    // Convert RGB to RGBW
    if (_light_has_color && _light_use_white) {

        unsigned char white, max_in, max_out;
        double factor = 0;

        white = std::min(_light_channel[0].inputValue, std::min(_light_channel[1].inputValue, _light_channel[2].inputValue));
        max_in = std::max(_light_channel[0].inputValue, std::max(_light_channel[1].inputValue, _light_channel[2].inputValue));

        for (unsigned int i=0; i < 3; i++) {
            _light_channel[i].value = _light_channel[i].inputValue - white;
        }
        _light_channel[3].value = white;

        max_out = std::max(std::max(_light_channel[0].value, _light_channel[1].value), std::max(_light_channel[2].value, _light_channel[3].value));
        if (max_out > 0) {
            factor = (double) (max_in / max_out);
        }

        // Scale up to equal input values. So [250,150,50] -> [200,100,0,50] -> [250, 125, 0, 63]
        for (unsigned int i=0; i < 4; i++) {
            _light_channel[i].value = round((double) _light_channel[i].value * factor * brightness);
        }

        // Don't apply brightness, it is already in the inputValue:
        if (_light_channel.size() == 5) {
            _light_channel[4].value = _light_channel[4].inputValue;
        }

    } else {

        // Don't apply brightness, it is already in the inputValue:
        for (unsigned char i=0; i < _light_channel.size(); i++) {
            if (_light_has_color & (i<3)) {
                _light_channel[i].value = _light_channel[i].inputValue * brightness;
            } else {
                _light_channel[i].value = _light_channel[i].inputValue;
            }
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
        if (_light_has_color) {
            unsigned long mireds = atol(p + 1);
            _fromMireds(mireds);
        }
        break;
      case 'K': // Kelvin Value
        if (_light_has_color) {
            unsigned long kelvin = atol(p + 1);
            _fromKelvin(kelvin);
        }
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
void _fromKelvin(unsigned long kelvin, bool setMireds) {

    // Check we have RGB channels
    if (!_light_has_color) return;

    if (setMireds) {
      _light_mireds = round(1000000UL / kelvin);
    }

    // Calculate colors
    unsigned int red = (kelvin <= 66)
        ? LIGHT_MAX_VALUE
        : 329.698727446 * pow((kelvin - 60), -0.1332047592);
    unsigned int green = (kelvin <= 66)
        ? 99.4708025861 * log(kelvin) - 161.1195681661
        : 288.1221695283 * pow(kelvin, -0.0755148492);
    unsigned int blue = (kelvin >= 66)
        ? LIGHT_MAX_VALUE
        : ((kelvin <= 19)
            ? 0
            : 138.5177312231 * log(kelvin - 10) - 305.0447927307);

    _setRGBInputValue(
        constrain(red, 0, LIGHT_MAX_VALUE),
        constrain(green, 0, LIGHT_MAX_VALUE),
        constrain(blue, 0, LIGHT_MAX_VALUE)
    );
}

void _fromKelvin(unsigned long kelvin) {
  _fromKelvin(kelvin, true);
}

// Color temperature is measured in mireds (kelvin = 1e6/mired)
void _fromMireds(unsigned long mireds) {
    if (mireds == 0) mireds = 1;
    _light_mireds = mireds;
    unsigned long kelvin = constrain(1000000UL / mireds, 1000, 40000) / 100;
    _fromKelvin(kelvin, false);
}

// -----------------------------------------------------------------------------
// Output Values
// -----------------------------------------------------------------------------

void _toRGB(char * rgb, size_t len) {
    unsigned long value = 0;

    value += _light_channel[0].inputValue;
    value <<= 8;
    value += _light_channel[1].inputValue;
    value <<= 8;
    value += _light_channel[2].inputValue;

    snprintf_P(rgb, len, PSTR("#%06X"), value);
}

void _toHSV(char * hsv, size_t len) {
    double min, max, h, s, v;
    double brightness = (double) _light_brightness / LIGHT_MAX_BRIGHTNESS;

    double r = (double) (_light_channel[0].inputValue * brightness) / 255.0;
    double g = (double) (_light_channel[1].inputValue * brightness) / 255.0;
    double b = (double) (_light_channel[2].inputValue * brightness) / 255.0;

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

    // String
    snprintf_P(hsv, len, PSTR("%d,%d,%d"), round(h), round(s), round(v));
}

void _toLong(char * color, size_t len) {
    if (!_light_has_color) return;

    snprintf_P(color, len, PSTR("%d,%d,%d"),
        (int) _light_channel[0].inputValue,
        (int) _light_channel[1].inputValue,
        (int) _light_channel[2].inputValue
    );
}

void _toCSV(char * buffer, size_t len, bool applyBrightness) {
    char num[10];
    float b = applyBrightness ? (float) _light_brightness / LIGHT_MAX_BRIGHTNESS : 1;
    for (unsigned char i=0; i<_light_channel.size(); i++) {
        itoa(_light_channel[i].inputValue * b, num, 10);
        if (i>0) strncat(buffer, ",", len--);
        strncat(buffer, num, len);
        len = len - strlen(num);
    }
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
    return _toPWM(_light_channel[id].shadow, useGamma, _light_channel[id].reverse);
}

void _shadow() {
    // Update transition ticker
    _light_steps_left--;
    if (_light_steps_left == 0) _light_transition_ticker.detach();

    // Transitions
    unsigned char target;
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        target = _light_state ? _light_channel[i].value : 0;
        if (_light_steps_left == 0) {
            _light_channel[i].current = target;
        } else {
            double difference = (double) (target - _light_channel[i].current) / (_light_steps_left + 1);
            _light_channel[i].current = _light_channel[i].current + difference;
        }
        _light_channel[i].shadow = _light_channel[i].current;
    }
}

void _lightProviderUpdate() {

    _shadow();

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

void _lightColorSave() {
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        setSetting("ch", i, _light_channel[i].inputValue);
    }
    setSetting("brightness", _light_brightness);
    saveSettings();
}

void _lightColorRestore() {
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        _light_channel[i].inputValue = getSetting("ch", i, i==0 ? 255 : 0).toInt();
    }
    _light_brightness = getSetting("brightness", LIGHT_MAX_BRIGHTNESS).toInt();
    lightUpdate(false, false);
}

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

#if MQTT_SUPPORT
void _lightMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    String mqtt_group_color = getSetting("mqttGroupColor");

    if (type == MQTT_CONNECT_EVENT) {

        if (_light_has_color) {
            mqttSubscribe(MQTT_TOPIC_BRIGHTNESS);
            mqttSubscribe(MQTT_TOPIC_MIRED);
            mqttSubscribe(MQTT_TOPIC_KELVIN);
            mqttSubscribe(MQTT_TOPIC_COLOR_RGB);
            mqttSubscribe(MQTT_TOPIC_COLOR_HSV);
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
            _toRGB(buffer, sizeof(buffer));
        } else {
            _toLong(buffer, sizeof(buffer));
        }
        mqttSend(MQTT_TOPIC_COLOR_RGB, buffer);

        _toHSV(buffer, sizeof(buffer));
        mqttSend(MQTT_TOPIC_COLOR_HSV, buffer);

        // Brightness
        snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _light_brightness);
        mqttSend(MQTT_TOPIC_BRIGHTNESS, buffer);

        // Mireds
        snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _light_mireds);
        mqttSend(MQTT_TOPIC_MIRED, buffer);
    }

    // Channels
    for (unsigned int i=0; i < _light_channel.size(); i++) {
        itoa(_light_channel[i].inputValue, buffer, 10);
        mqttSend(MQTT_TOPIC_CHANNEL, i, buffer);
    }

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
        brokerPublish(MQTT_TOPIC_CHANNEL, i, buffer);
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

void lightUpdate(bool save, bool forward, bool group_forward) {

    _generateBrightness();

    // Configure color transition
    _light_steps_left = _light_use_transitions ? _light_transition_time / LIGHT_TRANSITION_STEP : 1;
    _light_transition_ticker.attach_ms(LIGHT_TRANSITION_STEP, _lightProviderUpdate);

    // Report channels to local broker
    #if BROKER_SUPPORT
        lightBroker();
    #endif

    // Report color & brightness to MQTT broker
    #if MQTT_SUPPORT
        if (forward) lightMQTT();
        if (group_forward) lightMQTTGroup();
    #endif

    // Report color to WS clients (using current brightness setting)
    #if WEB_SUPPORT
        wsSend(_lightWebSocketOnSend);
    #endif

    #if LIGHT_SAVE_ENABLED
        // Delay saving to EEPROM 5 seconds to avoid wearing it out unnecessarily
        if (save) _light_save_ticker.once(LIGHT_SAVE_DELAY, _lightColorSave);
    #endif

};

void lightUpdate(bool save, bool forward) {
    lightUpdate(save, forward, true);
}

#if LIGHT_SAVE_ENABLED == 0
void lightSave() {
    _lightColorSave();
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

void lightChannel(unsigned char id, unsigned int value) {
    if (id <= _light_channel.size()) {
        _light_channel[id].inputValue = constrain(value, 0, LIGHT_MAX_VALUE);
    }
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

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

bool _lightWebSocketOnReceive(const char * key, JsonVariant& value) {
    if (strncmp(key, "light", 5) == 0) return true;
    if (strncmp(key, "use", 3) == 0) return true;
    return false;
}

void _lightWebSocketOnSend(JsonObject& root) {
    root["colorVisible"] = 1;
    root["mqttGroupColor"] = getSetting("mqttGroupColor");
    root["useColor"] = _light_has_color;
    root["useWhite"] = _light_use_white;
    root["useGamma"] = _light_use_gamma;
    root["useTransitions"] = _light_use_transitions;
    root["lightTime"] = _light_transition_time;
    root["useCSS"] = getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1;
    bool useRGB = getSetting("useRGB", LIGHT_USE_RGB).toInt() == 1;
    root["useRGB"] = useRGB;
    if (_light_has_color) {
        if (useRGB) {
            root["rgb"] = lightColor(true);
            root["brightness"] = lightBrightness();
        } else {
            root["hsv"] = lightColor(false);
        }
    }
    JsonArray& channels = root.createNestedArray("channels");
    for (unsigned char id=0; id < _light_channel.size(); id++) {
        channels.add(lightChannel(id));
    }
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
            if (data.containsKey("brightness")) {
                lightBrightness(data["brightness"]);
                lightUpdate(true, true);
            }
        }
    }

    if (strcmp(action, "channel") == 0) {
        if (data.containsKey("id") && data.containsKey("value")) {
            lightChannel(data["id"], data["value"]);
            lightUpdate(true, true);
        }
    }
}

void _lightAPISetup() {
  // API entry points (protected with apikey)
  if (_light_has_color) {
    apiRegister(MQTT_TOPIC_COLOR_RGB,
        [](char * buffer, size_t len) {
            if (getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1) {
                _toRGB(buffer, len);
            } else {
                _toLong(buffer, len);
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

    apiRegister(MQTT_TOPIC_BRIGHTNESS,
        [](char * buffer, size_t len) {
			snprintf_P(buffer, len, PSTR("%d"), _light_brightness);
        },
        [](const char * payload) {
            lightBrightness(atoi(payload));
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
			snprintf_P(buffer, len, PSTR("%d"), lightChannel(id));
          },
          [id](const char * payload) {
              lightChannel(id, atoi(payload));
              lightUpdate(true, true);
          }
      );
  }
}

#endif // WEB_SUPPORT

#if TERMINAL_SUPPORT

void _lightInitCommands() {

    settingsRegisterCommand(F("BRIGHTNESS"), [](Embedis* e) {
        if (e->argc > 1) {
            lightBrightness(String(e->argv[1]).toInt());
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Brightness: %d\n"), lightBrightness());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("CHANNEL"), [](Embedis* e) {
        if (e->argc < 2) {
            DEBUG_MSG_P(PSTR("-ERROR: Wrong arguments\n"));
        }
        int id = String(e->argv[1]).toInt();
        if (e->argc > 2) {
            int value = String(e->argv[2]).toInt();
            lightChannel(id, value);
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Channel #%d: %d\n"), id, lightChannel(id));
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("COLOR"), [](Embedis* e) {
        if (e->argc > 1) {
            String color = String(e->argv[1]);
            lightColor(color.c_str());
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("KELVIN"), [](Embedis* e) {
        if (e->argc > 1) {
            String color = String("K") + String(e->argv[1]);
            lightColor(color.c_str());
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        DEBUG_MSG_P(PSTR("+OK\n"));
    });

    settingsRegisterCommand(F("MIRED"), [](Embedis* e) {
        if (e->argc > 1) {
            String color = String("M") + String(e->argv[1]);
            lightColor(color.c_str());
            lightUpdate(true, true);
        }
        DEBUG_MSG_P(PSTR("Color: %s\n"), lightColor().c_str());
        DEBUG_MSG_P(PSTR("+OK\n"));
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
    if (_light_use_white && (_light_channel.size() < 4)) {
        _light_use_white = false;
        setSetting("useWhite", _light_use_white);
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
    _lightColorRestore();

    #if WEB_SUPPORT
        _lightAPISetup();
        wsOnSendRegister(_lightWebSocketOnSend);
        wsOnActionRegister(_lightWebSocketOnAction);
        wsOnReceiveRegister(_lightWebSocketOnReceive);
        wsOnAfterParseRegister([]() {
            #if LIGHT_SAVE_ENABLED == 0
                lightSave();
            #endif
            _lightConfigure();
        });
    #endif

    #if MQTT_SUPPORT
        mqttRegister(_lightMQTTCallback);
    #endif

    #if TERMINAL_SUPPORT
        _lightInitCommands();
    #endif

}

#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
