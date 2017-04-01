/*

LIGHT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#include <Ticker.h>
Ticker colorTicker;
bool _lightState = false;
unsigned int _lightColor[3] = {0};

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
#include <my9291.h>
my9291 * _my9291;
#endif

// -----------------------------------------------------------------------------
// UTILS
// -----------------------------------------------------------------------------

void color_string2array(const char * rgb, unsigned int * array) {

    char * p = (char *) rgb;
    if (strlen(p) == 0) return;

    // if color begins with a # then assume HEX RGB
    if (p[0] == '#') {

        ++p;
        unsigned long value = strtol(p, NULL, 16);
        array[0] = (value >> 16) & 0xFF;
        array[1] = (value >> 8) & 0xFF;
        array[2] = (value) & 0xFF;

    // it's a temperature
    } else if (p[strlen(p)-1] == 'K') {

        p[strlen(p)-1] = 0;
        unsigned int temperature = atoi(p);
        color_temperature2array(temperature, array);

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

void color_array2rgb(unsigned int * array, char * rgb) {
    unsigned long value = array[0];
    value = (value << 8) + array[1];
    value = (value << 8) + array[2];
    sprintf(rgb, "#%06X", value);
}

// Thanks to Sacha Telgenhof for sharing this code in his AiLight library
// https://github.com/stelgenhof/AiLight
void color_temperature2array(unsigned int temperature, unsigned int * array) {

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

// -----------------------------------------------------------------------------
// PROVIDER
// -----------------------------------------------------------------------------

void _lightProviderSet(bool state, unsigned int red, unsigned int green, unsigned int blue) {

    unsigned int white = 0;

    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192) || (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW)
		// If all set to the same value use white instead
		if ((red == green) && (green == blue)) {
		    white = red;
		    red = green = blue = 0;
		}
	#endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
        _my9291->setState(state);
        _my9291->setColor((my9291_color_t) { red, green, blue, white });
    #endif

    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_RGB) || (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW)

        // Check state
        if (!state) red = green = blue = white = 0;

        if (RGBW_INVERSE_LOGIC) {
            analogWrite(RGBW_RED_PIN, red);
            analogWrite(RGBW_GREEN_PIN, green);
            analogWrite(RGBW_BLUE_PIN, blue);
		    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW)
	            analogWrite(RGBW_WHITE_PIN, white);
			#endif
        } else {
            analogWrite(RGBW_RED_PIN, LIGHT_MAX_VALUE - red);
            analogWrite(RGBW_GREEN_PIN, LIGHT_MAX_VALUE - green);
            analogWrite(RGBW_BLUE_PIN, LIGHT_MAX_VALUE - blue);
		    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW)
	            analogWrite(RGBW_WHITE_PIN, LIGHT_MAX_VALUE - white);
			#endif
        }
    #endif

}

// -----------------------------------------------------------------------------
// LIGHT MANAGEMENT
// -----------------------------------------------------------------------------

void lightState(bool state) {
    _lightState = state;
    _lightProviderSet(_lightState, _lightColor[0], _lightColor[1], _lightColor[2]);
}

bool lightState() {
    return _lightState;
}

void lightColor(const char * color, bool save, bool forward) {

    color_string2array(color, _lightColor);
    _lightProviderSet(_lightState, _lightColor[0], _lightColor[1], _lightColor[2]);

    char rgb[8];
    color_array2rgb(_lightColor, rgb);

    // Delay saving to EEPROM 5 seconds to avoid wearing it out unnecessarily
    if (save) colorTicker.once(LIGHT_SAVE_DELAY, _lightColorSave);

    // Report color to MQTT broker
    if (forward) mqttSend(MQTT_TOPIC_COLOR, rgb);

    // Report color to WS clients
    char message[20];
    sprintf(message, "{\"color\": \"%s\"}", rgb);
    wsSend(message);

}

String lightColor() {
    char rgb[8];
    color_array2rgb(_lightColor, rgb);
    return String(rgb);
}

// -----------------------------------------------------------------------------
// PERSISTANCE
// -----------------------------------------------------------------------------

void _lightColorSave() {
    setSetting("color", lightColor());
    saveSettings();
}

void _lightColorRestore() {
    String color = getSetting("color", LIGHT_DEFAULT_COLOR);
    color_string2array(color.c_str(), _lightColor);
}

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

void lightMQTTCallback(unsigned int type, const char * topic, const char * payload) {


    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_COLOR);
    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttSubtopic((char *) topic);
        if (!t.equals(MQTT_TOPIC_COLOR)) return;

        lightColor(payload, true, mqttForward());

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
        analogWriteRange(LIGHT_MAX_VALUE);
        analogWriteFreq(1000);
        pinMode(RGBW_RED_PIN, OUTPUT);
        pinMode(RGBW_GREEN_PIN, OUTPUT);
        pinMode(RGBW_BLUE_PIN, OUTPUT);
		#if LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW
	        pinMode(RGBW_WHITE_PIN, OUTPUT);
		#endif
    #endif

    _lightColorRestore();

    // API entry points (protected with apikey)
    apiRegister(MQTT_TOPIC_COLOR, MQTT_TOPIC_COLOR,
        [](char * buffer, size_t len) {
			snprintf(buffer, len, "%s", lightColor().c_str());
        },
        [](const char * payload) {
            lightColor(payload, true, mqttForward());
        }
    );

    mqttRegister(lightMQTTCallback);

}

#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
