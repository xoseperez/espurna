/*

LIGHT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
#ifndef LIGHT_PROVIDER_EXPERIMENTAL_RGB_ONLY_HSV_IR

#include <Ticker.h>
#include <ArduinoJson.h>
#include <vector>

Ticker colorTicker;
typedef struct {
    unsigned char pin;
    bool reverse;
    unsigned char value;
    unsigned char shadow;
} channel_t;
std::vector<channel_t> _channels;
bool _lightState = false;
unsigned int _brightness = LIGHT_MAX_BRIGHTNESS;

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
#include <my9291.h>
my9291 * _my9291;
#endif

// Gamma Correction lookup table for gamma=2.8 and 12 bit (4095) full scale
// TODO: move to PROGMEM
const unsigned short gamma_table[LIGHT_MAX_VALUE+1] = {
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,
   2,   2,   2,   3,   3,   4,   4,   5,   5,   6,   7,   8,   8,   9,  10,  11,
  12,  13,  15,  16,  17,  18,  20,  21,  23,  25,  26,  28,  30,  32,  34,  36,
  38,  40,  43,  45,  48,  50,  53,  56,  59,  62,  65,  68,  71,  75,  78,  82,
  85,  89,  93,  97, 101, 105, 110, 114, 119, 123, 128, 133, 138, 143, 149, 154,
 159, 165, 171, 177, 183, 189, 195, 202, 208, 215, 222, 229, 236, 243, 250, 258,
 266, 273, 281, 290, 298, 306, 315, 324, 332, 341, 351, 360, 369, 379, 389, 399,
 409, 419, 430, 440, 451, 462, 473, 485, 496, 508, 520, 532, 544, 556, 569, 582,
 594, 608, 621, 634, 648, 662, 676, 690, 704, 719, 734, 749, 764, 779, 795, 811,
 827, 843, 859, 876, 893, 910, 927, 944, 962, 980, 998,1016,1034,1053,1072,1091,
1110,1130,1150,1170,1190,1210,1231,1252,1273,1294,1316,1338,1360,1382,1404,1427,
1450,1473,1497,1520,1544,1568,1593,1617,1642,1667,1693,1718,1744,1770,1797,1823,
1850,1877,1905,1932,1960,1988,2017,2045,2074,2103,2133,2162,2192,2223,2253,2284,
2315,2346,2378,2410,2442,2474,2507,2540,2573,2606,2640,2674,2708,2743,2778,2813,
2849,2884,2920,2957,2993,3030,3067,3105,3143,3181,3219,3258,3297,3336,3376,3416,
3456,3496,3537,3578,3619,3661,3703,3745,3788,3831,3874,3918,3962,4006,4050,4095 };

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

void _fromLong(unsigned long value, bool brightness) {

    if (brightness) {
        _channels[0].value = (value >> 24) & 0xFF;
        _channels[1].value = (value >> 16) & 0xFF;
        _channels[2].value = (value >> 8) & 0xFF;
        _brightness = (value & 0xFF) * LIGHT_MAX_BRIGHTNESS / 255;
    } else {
        _channels[0].value = (value >> 16) & 0xFF;
        _channels[1].value = (value >> 8) & 0xFF;
        _channels[2].value = (value) & 0xFF;
    }

}

void _fromRGB(const char * rgb) {

    char * p = (char *) rgb;
    if (strlen(p) == 0) return;

    // if color begins with a # then assume HEX RGB
    if (p[0] == '#') {

        if (lightHasColor()) {

            ++p;
            unsigned long value = strtoul(p, NULL, 16);

            // RGBA values are interpreted like RGB + brightness
            _fromLong(value, strlen(p) > 7);

        }

    // it's a temperature in mireds
    } else if (p[0] == 'M') {

        if (lightHasColor()) {
            unsigned long mireds = atol(p + 1);
            _fromMireds(mireds);
        }

    // it's a temperature in kelvin
    } else if (p[0] == 'K') {

        if (lightHasColor()) {
            unsigned long kelvin = atol(p + 1);
            _fromKelvin(kelvin);
        }

    // otherwise assume decimal values separated by commas
    } else {

        char * tok;
        unsigned char count = 0;
        unsigned char channels = _channels.size();

        tok = strtok(p, ",");
        while (tok != NULL) {
            _channels[count].value = atoi(tok);
            if (++count == channels) break;
            tok = strtok(NULL, ",");
        }

        // RGB but less than 3 values received
        if (lightHasColor() && (count < 3)) {
            _channels[1].value = _channels[0].value;
            _channels[2].value = _channels[0].value;
        }

    }

}

void _toRGB(char * rgb, size_t len, bool applyBrightness) {

    if (!lightHasColor()) return;

    float b = applyBrightness ? (float) _brightness / LIGHT_MAX_BRIGHTNESS : 1;

    unsigned long value = 0;

    value += _channels[0].value * b;
    value <<= 8;
    value += _channels[1].value * b;
    value <<= 8;
    value += _channels[2].value * b;

    snprintf_P(rgb, len, PSTR("#%06X"), value);

}

void _toRGB(char * rgb, size_t len) {
    _toRGB(rgb, len, false);
}

void _toLong(char * color, size_t len, bool applyBrightness) {

    if (!lightHasColor()) return;

    float b = applyBrightness ? (float) _brightness / LIGHT_MAX_BRIGHTNESS : 1;

    snprintf_P(color, len, PSTR("%d,%d,%d"),
        (int) (_channels[0].value * b),
        (int) (_channels[1].value * b),
        (int) (_channels[2].value * b)
    );

}

void _toLong(char * color, size_t len) {
    _toLong(color, len, false);
}

// Thanks to Sacha Telgenhof for sharing this code in his AiLight library
// https://github.com/stelgenhof/AiLight
void _fromKelvin(unsigned long kelvin) {

    // Check we have RGB channels
    if (!lightHasColor()) return;

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

    // Save values
    _channels[0].value = constrain(red, 0, LIGHT_MAX_VALUE);
    _channels[1].value = constrain(green, 0, LIGHT_MAX_VALUE);
    _channels[2].value = constrain(blue, 0, LIGHT_MAX_VALUE);

}

// Color temperature is measured in mireds (kelvin = 1e6/mired)
void _fromMireds(unsigned long mireds) {
    if (mireds == 0) mireds = 1;
    unsigned long kelvin = constrain(1000000UL / mireds, 1000, 40000) / 100;
    _fromKelvin(kelvin);
}

unsigned int _toPWM(unsigned long value, bool bright, bool gamma, bool reverse) {
    value = constrain(value, 0, LIGHT_MAX_VALUE);
    if (bright) value *= ((float) _brightness / LIGHT_MAX_BRIGHTNESS);
    unsigned int pwm = gamma ? gamma_table[value] : map(value, 0, LIGHT_MAX_VALUE, 0, LIGHT_LIMIT_PWM);
    if (reverse) pwm = LIGHT_LIMIT_PWM - pwm;
    return pwm;
}

// Returns a PWM valule for the given channel ID
unsigned int _toPWM(unsigned char id) {
    if (id < _channels.size()) {
        bool isColor = lightHasColor() && (id < 3);
        bool bright = isColor;
        bool gamma = isColor & (getSetting("useGamma", LIGHT_USE_GAMMA).toInt() == 1);
        return _toPWM(_channels[id].shadow, bright, gamma, _channels[id].reverse);
    }
    return 0;
}

// -----------------------------------------------------------------------------
// PROVIDER
// -----------------------------------------------------------------------------

void _shadow() {

    for (unsigned int i=0; i < _channels.size(); i++) {
        _channels[i].shadow = _lightState ? _channels[i].value : 0;
    }

    if (lightHasColor()) {

        bool useWhite = getSetting("useWhite", LIGHT_USE_WHITE).toInt() == 1;

        if (_lightState && useWhite && (_channels.size() > 3)) {
            if (_channels[0].shadow == _channels[1].shadow  && _channels[1].shadow == _channels[2].shadow ) {
                _channels[3].shadow = _channels[0].shadow * ((float) _brightness / LIGHT_MAX_BRIGHTNESS);
                _channels[2].shadow = 0;
                _channels[1].shadow = 0;
                _channels[0].shadow = 0;
            }
        }

    }

}

void _lightProviderUpdate() {

    _shadow();

    #ifdef LIGHT_ENABLE_PIN
        digitalWrite(LIGHT_ENABLE_PIN, _lightState);
    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192

        if (_lightState) {

            float ratio = (float) LIGHT_MAX_VALUE / LIGHT_MAX_PWM;

            unsigned int red = _toPWM(0) * ratio;
            unsigned int green = _toPWM(1) * ratio;
            unsigned int blue = _toPWM(2) * ratio;
            unsigned int white = _toPWM(3) * ratio;
            unsigned int warm = _toPWM(4) * ratio;
            _my9291->setColor((my9291_color_t) { red, green, blue, white, warm });
            _my9291->setState(true);

        } else {

            _my9291->setColor((my9291_color_t) { 0, 0, 0, 0, 0 });
            _my9291->setState(false);

        }

    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

        for (unsigned int i=0; i < _channels.size(); i++) {
            analogWrite(_channels[i].pin, _toPWM(i));
        }

    #endif

}

// -----------------------------------------------------------------------------
// PERSISTANCE
// -----------------------------------------------------------------------------

void _lightColorSave() {
    for (unsigned int i=0; i < _channels.size(); i++) {
        setSetting("ch", i, _channels[i].value);
    }
    setSetting("brightness", _brightness);
    saveSettings();
}

void _lightColorRestore() {
    for (unsigned int i=0; i < _channels.size(); i++) {
        _channels[i].value = getSetting("ch", i, i==0 ? 255 : 0).toInt();
    }
    _brightness = getSetting("brightness", LIGHT_MAX_BRIGHTNESS).toInt();
    lightUpdate(false, false);
}

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

void _lightMQTTCallback(unsigned int type, const char * topic, const char * payload) {


    if (type == MQTT_CONNECT_EVENT) {

        if (lightHasColor()) {
            mqttSubscribe(MQTT_TOPIC_BRIGHTNESS);
            mqttSubscribe(MQTT_TOPIC_MIRED);
            mqttSubscribe(MQTT_TOPIC_KELVIN);
            mqttSubscribe(MQTT_TOPIC_COLOR);
        }

        char buffer[strlen(MQTT_TOPIC_CHANNEL) + 3];
        snprintf_P(buffer, sizeof(buffer), PSTR("%s/+"), MQTT_TOPIC_CHANNEL);
        mqttSubscribe(buffer);

    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttSubtopic((char *) topic);

        // Color temperature in mireds
        if (t.equals(MQTT_TOPIC_MIRED)) {
            _fromMireds(atol(payload));
            lightUpdate(true, mqttForward());
        }

        // Color temperature in kelvins
        if (t.equals(MQTT_TOPIC_KELVIN)) {
            _fromKelvin(atol(payload));
            lightUpdate(true, mqttForward());
        }

        // Color
        if (t.equals(MQTT_TOPIC_COLOR)) {
            lightColor(payload);
            lightUpdate(true, mqttForward());
        }

        // Brightness
        if (t.equals(MQTT_TOPIC_BRIGHTNESS)) {
            _brightness = constrain(atoi(payload), 0, LIGHT_MAX_BRIGHTNESS);
            lightUpdate(true, mqttForward());
        }

        // Channel
        if (t.startsWith(MQTT_TOPIC_CHANNEL)) {
            unsigned int channelID = t.substring(strlen(MQTT_TOPIC_CHANNEL)+1).toInt();
            if (channelID >= _channels.size()) {
                DEBUG_MSG_P(PSTR("[LIGHT] Wrong channelID (%d)\n"), channelID);
                return;
            }
            lightChannel(channelID, atoi(payload));
            lightUpdate(true, mqttForward());
        }

    }

}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

unsigned char lightChannels() {
    return _channels.size();
}

bool lightHasColor() {
    bool useColor = getSetting("useColor", LIGHT_USE_COLOR).toInt() == 1;
    return useColor && (_channels.size() > 2);
}

unsigned char lightWhiteChannels() {
    return _channels.size() % 3;
}

void lightMQTT() {

    char buffer[12];

    if (lightHasColor()) {

        // Color
        if (getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1) {
            _toRGB(buffer, 12, false);
        } else {
            _toLong(buffer, 12, false);
        }
        mqttSend(MQTT_TOPIC_COLOR, buffer);

        // Brightness
        snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _brightness);
        mqttSend(MQTT_TOPIC_BRIGHTNESS, buffer);

    }

    // Channels
    for (unsigned int i=0; i < _channels.size(); i++) {
        snprintf_P(buffer, sizeof(buffer), PSTR("%d"), _channels[i].value);
        mqttSend(MQTT_TOPIC_CHANNEL, i, buffer);
    }

}

void lightUpdate(bool save, bool forward) {

    _lightProviderUpdate();

    // Report color & brightness to MQTT broker
    if (forward) lightMQTT();

    // Report color to WS clients (using current brightness setting)
    #if WEB_SUPPORT
    {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        root["colorVisible"] = 1;
        root["useColor"] = getSetting("useColor", LIGHT_USE_COLOR).toInt() == 1;
        root["useWhite"] = getSetting("useWhite", LIGHT_USE_WHITE).toInt() == 1;
        root["useGamma"] = getSetting("useGamma", LIGHT_USE_GAMMA).toInt() == 1;
        if (lightHasColor()) {
            root["color"] = lightColor();
            root["brightness"] = lightBrightness();
        }
        JsonArray& channels = root.createNestedArray("channels");
        for (unsigned char id=0; id < lightChannels(); id++) {
            channels.add(lightChannel(id));
        }
        String output;
        root.printTo(output);
        wsSend(output.c_str());
    }
    #endif

    #if LIGHT_SAVE_ENABLED
        // Delay saving to EEPROM 5 seconds to avoid wearing it out unnecessarily
        if (save) colorTicker.once(LIGHT_SAVE_DELAY, _lightColorSave);
    #endif

};

#if LIGHT_SAVE_ENABLED == 0
void lightSave() {
    _lightColorSave();
}
#endif

void lightState(bool state) {
    _lightState = state;
}

bool lightState() {
    return _lightState;
}

void lightColor(const char * color) {
    _fromRGB(color);
}

void lightColor(unsigned long color) {
    _fromLong(color, false);
}

String lightColor() {
    char rgb[8];
    _toRGB(rgb, 8, false);
    return String(rgb);
}

unsigned int lightChannel(unsigned char id) {
    if (id <= _channels.size()) {
        return _channels[id].value;
    }
    return 0;
}

void lightChannel(unsigned char id, unsigned int value) {
    if (id <= _channels.size()) {
        _channels[id].value = constrain(value, 0, LIGHT_MAX_VALUE);
    }
}

unsigned int lightBrightness() {
    return _brightness;
}

void lightBrightness(unsigned int b) {
    _brightness = constrain(b, 0, LIGHT_MAX_BRIGHTNESS);
}

void lightBrightnessStep(int steps) {
    lightBrightness(_brightness + steps * LIGHT_STEP);
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

void _lightAPISetup() {

    #if WEB_SUPPORT

        // API entry points (protected with apikey)
        if (lightHasColor()) {

            apiRegister(MQTT_TOPIC_COLOR, MQTT_TOPIC_COLOR,
                [](char * buffer, size_t len) {
                    if (getSetting("useCSS", LIGHT_USE_CSS).toInt() == 1) {
                        _toRGB(buffer, len, false);
                    } else {
                        _toLong(buffer, len, false);
                    }
                },
                [](const char * payload) {
                    lightColor(payload);
                    lightUpdate(true, true);
                }
            );

            apiRegister(MQTT_TOPIC_BRIGHTNESS, MQTT_TOPIC_BRIGHTNESS,
                [](char * buffer, size_t len) {
        			snprintf_P(buffer, len, PSTR("%d"), _brightness);
                },
                [](const char * payload) {
                    lightBrightness(atoi(payload));
                    lightUpdate(true, true);
                }
            );

            apiRegister(MQTT_TOPIC_KELVIN, MQTT_TOPIC_KELVIN,
                [](char * buffer, size_t len) {},
                [](const char * payload) {
                    _fromKelvin(atol(payload));
                    lightUpdate(true, true);
                }
            );

            apiRegister(MQTT_TOPIC_MIRED, MQTT_TOPIC_MIRED,
                [](char * buffer, size_t len) {},
                [](const char * payload) {
                    _fromMireds(atol(payload));
                    lightUpdate(true, true);
                }
            );

        }

        for (unsigned int id=0; id<lightChannels(); id++) {

            char url[15];
            snprintf_P(url, sizeof(url), PSTR("%s/%d"), MQTT_TOPIC_CHANNEL, id);

            char key[10];
            snprintf_P(key, sizeof(key), PSTR("%s%d"), MQTT_TOPIC_CHANNEL, id);

            apiRegister(url, key,
                [id](char * buffer, size_t len) {
    				snprintf_P(buffer, len, PSTR("%d"), lightChannel(id));
                },
                [id](const char * payload) {
                    lightChannel(id, atoi(payload));
                    lightUpdate(true, true);
                }
            );

        }

    #endif // WEB_SUPPORT

}

void lightSetup() {

    #ifdef LIGHT_ENABLE_PIN
        pinMode(LIGHT_ENABLE_PIN, OUTPUT);
    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192

        _my9291 = new my9291(MY9291_DI_PIN, MY9291_DCKI_PIN, MY9291_COMMAND, MY9291_CHANNELS);
        for (unsigned char i=0; i<MY9291_CHANNELS; i++) {
            _channels.push_back((channel_t) {0, false, 0});
        }

    #endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER

        #ifdef LIGHT_CH1_PIN
            _channels.push_back((channel_t) {LIGHT_CH1_PIN, LIGHT_CH1_INVERSE, 0});
        #endif

        #ifdef LIGHT_CH2_PIN
            _channels.push_back((channel_t) {LIGHT_CH2_PIN, LIGHT_CH2_INVERSE, 0});
        #endif

        #ifdef LIGHT_CH3_PIN
            _channels.push_back((channel_t) {LIGHT_CH3_PIN, LIGHT_CH3_INVERSE, 0});
        #endif

        #ifdef LIGHT_CH4_PIN
            _channels.push_back((channel_t) {LIGHT_CH4_PIN, LIGHT_CH4_INVERSE, 0});
        #endif

        #ifdef LIGHT_CH5_PIN
            _channels.push_back((channel_t) {LIGHT_CH5_PIN, LIGHT_CH5_INVERSE, 0});
        #endif

        analogWriteRange(LIGHT_MAX_PWM+1);
        analogWriteFreq(LIGHT_PWM_FREQUENCY);
        for (unsigned int i=0; i < _channels.size(); i++) {
            pinMode(_channels[i].pin, OUTPUT);
        }


    #endif

    DEBUG_MSG_P(PSTR("[LIGHT] LIGHT_PROVIDER = %d\n"), LIGHT_PROVIDER);
    DEBUG_MSG_P(PSTR("[LIGHT] Number of channels: %d\n"), _channels.size());

    _lightColorRestore();
    _lightAPISetup();
    mqttRegister(_lightMQTTCallback);

}

void lightLoop(){
}


#endif // LIGHT_PROVIDER_EXPERIMENTAL_RGB_ONLY_HSV_IR
#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
