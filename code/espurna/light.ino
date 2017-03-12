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

void color_rgb2array(const char * rgb, unsigned int * array) {

    char * p = (char *) rgb;
    if (p[0] == '#') ++p;

    unsigned long value = strtol(p, NULL, 16);
    array[0] = (value >> 16) & 0xFF;
    array[1] = (value >> 8) & 0xFF;
    array[2] = (value) & 0xFF;

}

void color_array2rgb(unsigned int * array, char * rgb) {
    unsigned long value = array[0];
    value = (value << 8) + array[1];
    value = (value << 8) + array[2];
    sprintf(rgb, "#%06X", value);
}

// -----------------------------------------------------------------------------
// PROVIDER
// -----------------------------------------------------------------------------

void lightColorProvider(unsigned int red, unsigned int green, unsigned int blue) {

    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192) || (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW)
		unsigned int white = 0;

		// If all set to the same value use white instead
		if ((red == green) && (green == blue)) {
		    white = red;
		    red = green = blue = 0;
		}
	#endif

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
        _my9291->setColor((my9291_color_t) { red, green, blue, white });
    #endif

    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_RGB) || (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW)
        if (RGBW_INVERSE_LOGIC) {
            analogWrite(RGBW_RED_PIN, red);
            analogWrite(RGBW_GREEN_PIN, green);
            analogWrite(RGBW_BLUE_PIN, blue);
		    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW)
	            analogWrite(RGBW_WHITE_PIN, white);
			#endif
        } else {
            analogWrite(RGBW_RED_PIN, 255 - red);
            analogWrite(RGBW_GREEN_PIN, 255 - green);
            analogWrite(RGBW_BLUE_PIN, 255 - blue);
		    #if (LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW)
	            analogWrite(RGBW_WHITE_PIN, 255 - white);
			#endif
        }
    #endif

}

// -----------------------------------------------------------------------------
// LIGHT MANAGEMENT
// -----------------------------------------------------------------------------

void lightColor(const char * rgb, bool save, bool forward) {

    color_rgb2array(rgb, _lightColor);
    lightColorProvider(_lightColor[0], _lightColor[1], _lightColor[2]);

    // Delay saving to EEPROM 5 seconds to avoid wearing it out unnecessarily
    if (save) colorTicker.once(LIGHT_SAVE_DELAY, lightColorSave);

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

void lightState(bool state) {
    if (state) {
        lightColorProvider(_lightColor[0], _lightColor[1], _lightColor[2]);
    } else {
        lightColorProvider(0, 0, 0);
    }
    _lightState = state;
}

bool lightState() {
    return _lightState;
}

// -----------------------------------------------------------------------------
// PERSISTANCE
// -----------------------------------------------------------------------------

void lightColorSave() {
    setSetting("color", lightColor());
    saveSettings();
}

void lightColorRetrieve() {
    lightColor(getSetting("color", LIGHT_DEFAULT_COLOR).c_str(), false, true);
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
        pinMode(RGBW_RED_PIN, OUTPUT);
        pinMode(RGBW_GREEN_PIN, OUTPUT);
        pinMode(RGBW_BLUE_PIN, OUTPUT);
		#if LIGHT_PROVIDER == LIGHT_PROVIDER_RGBW
	        pinMode(RGBW_WHITE_PIN, OUTPUT);
		#endif
        lightColorProvider(0, 0, 0);
    #endif

    lightColorRetrieve();

    mqttRegister(lightMQTTCallback);

}

#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
