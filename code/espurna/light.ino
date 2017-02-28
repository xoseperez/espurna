/*

LIGHT MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

#include <Ticker.h>

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
#include <my9291.h>
my9291 * _my9291;
Ticker colorTicker;
bool _mqttSkipColor = false;
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
// LIGHT MANAGEMENT
// -----------------------------------------------------------------------------

void lightColor(const char * rgb, bool save) {

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192

        unsigned int array[4] = {0};
        color_rgb2array(rgb, array);

        // If all set to the same value use white instead
        if ((array[0] == array[1]) && (array[1] == array[2])) {
            array[3] = array[0];
            array[0] = array[1] = array[2] = 0;
        }

        // Set new color (if light is open it will automatically change)
        _my9291->setColor((my9291_color_t) { array[0], array[1], array[2], array[3] });

    #endif

    // Delay saving to EEPROM 5 seconds to avoid wearing it out unnecessarily
    if (save) {
        colorTicker.once(LIGHT_SAVE_DELAY, lightColorSave);
    }

    // Report color to MQTT broker
    _mqttSkipColor = true;
    mqttSend(MQTT_COLOR_TOPIC, rgb);

    // Report color to WS clients
    char message[20];
    sprintf(message, "{\"color\": \"%s\"}", rgb);
    wsSend(message);


}

void lightColor(const char * rgb) {
    lightColor(rgb, true);
}

String lightColor() {

    String response;

    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
        my9291_color_t color = _my9291->getColor();
        unsigned int array[3];
        if (color.white > 0) {
            array[0] = array[1] = array[2] = color.white;
        } else {
            array[0] = color.red;
            array[1] = color.green;
            array[2] = color.blue;
        }
        char rgb[8];
        color_array2rgb(array, rgb);
        response = String(rgb);
    #endif

    return response;

}

void lightState(bool state) {
    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
        _my9291->setState(state);
    #endif
}

bool lightState() {
    bool response = false;
    #if LIGHT_PROVIDER == LIGHT_PROVIDER_MY9192
        response = _my9291->getState();
    #endif
    return response;
}

// -----------------------------------------------------------------------------
// PERSISTANCE
// -----------------------------------------------------------------------------

void lightColorSave() {
    setSetting("color", lightColor());
    saveSettings();
}

void lightColorRetrieve() {
    lightColor(getSetting("color", LIGHT_DEFAULT_COLOR).c_str(), false);
}

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

void lightMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    String mqttSetter = getSetting("mqttSetter", MQTT_USE_SETTER);
    String mqttGetter = getSetting("mqttGetter", MQTT_USE_GETTER);
    bool sameSetGet = mqttGetter.compareTo(mqttSetter) == 0;

    if (type == MQTT_CONNECT_EVENT) {
        char buffer[strlen(MQTT_COLOR_TOPIC) + mqttSetter.length() + 20];
        sprintf(buffer, "%s%s", MQTT_COLOR_TOPIC, mqttSetter.c_str());
        mqttSubscribe(buffer);
    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        char * t = mqttSubtopic((char *) topic);
        int len = mqttSetter.length();
        if (strncmp(t + strlen(t) - len, mqttSetter.c_str(), len) != 0) return;

        if (strncmp(t, MQTT_COLOR_TOPIC, strlen(MQTT_COLOR_TOPIC)) == 0) {
            if (_mqttSkipColor) {
                _mqttSkipColor = false;
            } else {
                lightColor(payload, true);
            }
            return;
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

    lightColorRetrieve();

    mqttRegister(lightMQTTCallback);

}

#endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
