/*

LIGHT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#include <Ticker.h>
Ticker colorTicker;
bool _lightState = false;
float brightness = 1.0;
unsigned int _lightColor[3] = {0};

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
#include <my9291.h>
my9291 * _my9291;
#endif

#if ENABLE_GAMMA_CORRECTION

    #define GAMMA_TABLE_SIZE (256)
    #undef LIGHT_PWM_RANGE
    #define LIGHT_PWM_RANGE (4095)

    // Gamma Correction lookup table for gamma=2.8 and 12 bit (4095) full scale
    const unsigned short gamma_table[GAMMA_TABLE_SIZE] = {
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

#endif

#ifndef LIGHT_PWM_FREQUENCY
    #define LIGHT_PWM_FREQUENCY (1000)
#endif

#ifndef LIGHT_PWM_RANGE
    #define LIGHT_PWM_RANGE (255)
#endif

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

void _color_string2array(const char * rgb, unsigned int * array) {

    char * p = (char *) rgb;
    if (strlen(p) == 0) return;

    // if color begins with a # then assume HEX RGB
    if (p[0] == '#') {

        ++p;
        unsigned long value = strtol(p, NULL, 16);

        // RGBA values are interpreted like RGB + brightness
        if (strlen(p) > 7) {
            array[0] = (value >> 24) & 0xFF;
            array[1] = (value >> 16) & 0xFF;
            array[2] = (value >> 8) & 0xFF;
            brightness =float(value & 0xFF) / 255;
        } else {
            array[0] = (value >> 16) & 0xFF;
            array[1] = (value >> 8) & 0xFF;
            array[2] = (value) & 0xFF;
        }

    // it's a temperature
    } else if (p[strlen(p)-1] == 'K') {

        p[strlen(p)-1] = 0;
        unsigned int temperature = atoi(p);
        _color_temperature2array(temperature, array);

    // otherwise assume decimal values separated by commas
    } else {

        char * tok;
        tok = strtok(p, ",");
        array[0] = atoi(tok);
        tok = strtok(NULL, ",");

        // if there are more than one value assume R,G,B
        if (tok != NULL) {
            array[1] = atoi(tok);
            tok = strtok(NULL, ",");
            if (tok != NULL) {
                array[2] = atoi(tok);
            } else {
                array[2] = 0;
            }

        // only one value set red, green and blue to the same value
        } else {
            array[2] = array[1] = array[0];
        }

    }

}

void _color_array2rgb(unsigned int * array, float brightness, char * rgb) {
    unsigned long value = array[0] * brightness;
    value = (value << 8) + array[1] * brightness;
    value = (value << 8) + array[2] * brightness;
    sprintf(rgb, "#%06X", value);
}

// Thanks to Sacha Telgenhof for sharing this code in his AiLight library
// https://github.com/stelgenhof/AiLight
void _color_temperature2array(unsigned int temperature, unsigned int * array) {

    // Force boundaries and conversion
    temperature = constrain(temperature, 1000, 40000) / 100;

    // Calculate colors
    unsigned int red = (temperature <= 66)
        ? LIGHT_MAX_VALUE
        : 329.698727446 * pow((temperature - 60), -0.1332047592);
    unsigned int green = (temperature <= 66)
        ? 99.4708025861 * log(temperature) - 161.1195681661
        : 288.1221695283 * pow(temperature, -0.0755148492);
    unsigned int blue = (temperature >= 66)
        ? LIGHT_MAX_VALUE
        : ((temperature <= 19)
            ? 0
            : 138.5177312231 * log(temperature - 10) - 305.0447927307);

    // Save values
    array[0] = constrain(red, 0, LIGHT_MAX_VALUE);
    array[1] = constrain(green, 0, LIGHT_MAX_VALUE);
    array[2] = constrain(blue, 0, LIGHT_MAX_VALUE);

}

// Converts a color intensity value (0..255) to a pwm value
// This takes care of positive or negative logic and brightness
unsigned int _intensity2pwm(unsigned int intensity, float brightness) {

    intensity = brightness * intensity;

    #if ENABLE_GAMMA_CORRECTION
        unsigned int pwm = (intensity < GAMMA_TABLE_SIZE) ? gamma_table[intensity] : LIGHT_PWM_RANGE;
    #else
        unsigned int pwm = intensity;
    #endif

    #if RGBW_INVERSE_LOGIC != 1
        pwm = LIGHT_PWM_RANGE - pwm;
    #endif

    return pwm;

}

unsigned int _intensity2pwm(unsigned int intensity) {
    return _intensity2pwm(intensity, LIGHT_MAX_VALUE);
}


// -----------------------------------------------------------------------------
// PROVIDER
// -----------------------------------------------------------------------------

void _lightProviderSet(bool state, unsigned int red, unsigned int green, unsigned int blue, float brightness) {

    unsigned int white = 0;

    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192) || (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW) || (LIGHT_PROVIDER == LIGHT_PROVIDER_RGB2W)
		// If all set to the same value use white instead
		if ((red == green) && (green == blue)) {
		    white = red;
		    red = green = blue = 0;
		}
	#endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
        _my9291->setState(state);
        _my9291->setColor((my9291_color_t) { red * brightness, green * brightness, blue * brightness, white * brightness });
    #endif

    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_RGB) || (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW) || (LIGHT_PROVIDER == LIGHT_PROVIDER_RGB2W)

        // Check state
        if (!state) red = green = blue = white = 0;

        analogWrite(RGBW_RED_PIN, _intensity2pwm(red, brightness));
        analogWrite(RGBW_GREEN_PIN, _intensity2pwm(green, brightness));
        analogWrite(RGBW_BLUE_PIN, _intensity2pwm(blue, brightness));
        #if (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW)
            analogWrite(RGBW_WHITE_PIN, _intensity2pwm(white, brightness));
        #endif
        #if (LIGHT_PROVIDER == LIGHT_PROVIDER_RGB2W)
            analogWrite(RGBW_WHITE_PIN, _intensity2pwm(white, brightness));
            analogWrite(RGBW_WHITE2_PIN, _intensity2pwm(white,brightness));
        #endif

    #endif

}

// -----------------------------------------------------------------------------
// LIGHT MANAGEMENT
// -----------------------------------------------------------------------------

void lightState(bool state) {
    _lightState = state;
    _lightProviderSet(_lightState, _lightColor[0], _lightColor[1], _lightColor[2], brightness);
}

bool lightState() {
    return _lightState;
}

void parseColor(const char * color) {
    brightness = 1.0;
    _color_string2array(color, _lightColor);
}

void lightColor(bool save, bool forward) {

    _lightProviderSet(_lightState, _lightColor[0], _lightColor[1], _lightColor[2], brightness);

    char rgb[8];
    _color_array2rgb(_lightColor, brightness, rgb);

    // Delay saving to EEPROM 5 seconds to avoid wearing it out unnecessarily
    if (save) colorTicker.once(LIGHT_SAVE_DELAY, _lightColorSave);

    // Report color to MQTT broker
    if (forward) {
        mqttSend(MQTT_TOPIC_COLOR, rgb);
    }

    // Report color to WS clients
    char message[64];
    sprintf(message, "{\"color\": \"%s\", \"brightness\": %d}", rgb, (int) (brightness * LIGHT_MAX_BRIGHTNESS));
    wsSend(message);

}

String lightColor(float b) {
    char rgb[8];
    _color_array2rgb(_lightColor, b, rgb);
    return String(rgb);
}

String lightColor() {
    return lightColor(brightness);
}

// -----------------------------------------------------------------------------
// PERSISTANCE
// -----------------------------------------------------------------------------

void _lightColorSave() {
    setSetting("color", lightColor(1.0));
    setSetting("brightness", brightness * LIGHT_MAX_BRIGHTNESS);
    saveSettings();
}

void _lightColorRestore() {
    String color = getSetting("color", LIGHT_DEFAULT_COLOR);
    _color_string2array(color.c_str(), _lightColor);
    brightness = getSetting("brightness", 1).toFloat() / LIGHT_MAX_BRIGHTNESS;
}

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

void lightMQTTCallback(unsigned int type, const char * topic, const char * payload) {


    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_BRIGHTNESS);
        mqttSubscribe(MQTT_TOPIC_COLORTEMP);
        mqttSubscribe(MQTT_TOPIC_COLOR);
    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttSubtopic((char *) topic);

        // Color temperature
        if (t.equals(MQTT_TOPIC_COLORTEMP)) {
            char buffer[10];
            sprintf(buffer, "%sK", payload);
            parseColor(buffer);
            lightColor(true, mqttForward());
        }

        // Color
        if (t.equals(MQTT_TOPIC_COLOR)) {
            parseColor(payload);
            lightColor(true, mqttForward());
        }

        // Brightness
        if (t.equals(MQTT_TOPIC_BRIGHTNESS)) {
            brightness = (float) atoi(payload) / LIGHT_MAX_BRIGHTNESS;
            lightColor(true, mqttForward());
        }


    }

}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

void lightSetup() {

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
        _my9291 = new my9291(MY9291_DI_PIN, MY9291_DCKI_PIN, MY9291_COMMAND);
    #endif

    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_RGB) || (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW)
        analogWriteRange(LIGHT_PWM_RANGE);
        analogWriteFreq(LIGHT_PWM_FREQUENCY);
        pinMode(RGBW_RED_PIN, OUTPUT);
        pinMode(RGBW_GREEN_PIN, OUTPUT);
        pinMode(RGBW_BLUE_PIN, OUTPUT);
		#if LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW
	        pinMode(RGBW_WHITE_PIN, OUTPUT);
		#endif
        #if LIGHT_PROVIDER == LIGHT_PROVIDER_RGB2W
	        pinMode(RGBW_WHITE_PIN, OUTPUT);
            pinMode(RGBW_WHITE2_PIN, OUTPUT);
		#endif
    #endif

    _lightColorRestore();

    // API entry points (protected with apikey)
    apiRegister(MQTT_TOPIC_COLOR, MQTT_TOPIC_COLOR,
        [](char * buffer, size_t len) {
			snprintf(buffer, len, "%s", lightColor().c_str());
        },
        [](const char * payload) {
            parseColor(payload);
            lightColor(true, mqttForward());
        }
    );

    apiRegister(MQTT_TOPIC_BRIGHTNESS, MQTT_TOPIC_BRIGHTNESS,
        NULL,
        [](const char * payload) {
            lightColor(true, mqttForward());
        }
    );

    mqttRegister(lightMQTTCallback);

}

#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
