/*

UART_MQTT MODULE

Copyright (C) 2018 by Albert Weterings
Adapted by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "uartmqtt.h"

#if UART_MQTT_SUPPORT

#include "mqtt.h"

char _uartmqttBuffer[UART_MQTT_BUFFER_SIZE];
bool _uartmqttNewData = false;

#if UART_MQTT_USE_SOFT
    SoftwareSerial _uart_mqtt_serial(UART_MQTT_RX_PIN, UART_MQTT_TX_PIN, false, UART_MQTT_BUFFER_SIZE);
    #define UART_MQTT_PORT  _uart_mqtt_serial
#else
    #define UART_MQTT_PORT  UART_MQTT_HW_PORT
#endif

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

void _uartmqttReceiveUART() {

    static unsigned char ndx = 0;

    while (UART_MQTT_PORT.available() > 0 && _uartmqttNewData == false) {

        char rc = UART_MQTT_PORT.read();

        if (rc != UART_MQTT_TERMINATION) {

            _uartmqttBuffer[ndx] = rc;
            if (ndx < UART_MQTT_BUFFER_SIZE - 1) ndx++;

        } else {

            _uartmqttBuffer[ndx] = '\0';
            _uartmqttNewData = true;
            ndx = 0;

        }

    }

}

void _uartmqttSendMQTT() {
    if (_uartmqttNewData == true && MQTT_SUPPORT) {
        DEBUG_MSG_P(PSTR("[UART_MQTT] Send data over MQTT: %s\n"), _uartmqttBuffer);
        mqttSend(MQTT_TOPIC_UARTIN, _uartmqttBuffer);
        _uartmqttNewData = false;
    }
}

void _uartmqttSendUART(const char * message) {
    DEBUG_MSG_P(PSTR("[UART_MQTT] Send data over UART: %s\n"), message);
    UART_MQTT_PORT.println(message);
}

void _uartmqttMQTTCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_UARTOUT);
    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttMagnitude((char *) topic);
        if (t.equals(MQTT_TOPIC_UARTOUT)) {
            _uartmqttSendUART(payload);
        }

    }

}

// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

void _uartmqttLoop() {
    _uartmqttReceiveUART();
    _uartmqttSendMQTT();
}

void uartmqttSetup() {

    // Init port
    UART_MQTT_PORT.begin(UART_MQTT_BAUDRATE);

    // Register MQTT callbackj
    mqttRegister(_uartmqttMQTTCallback);

    // Register loop
    espurnaRegisterLoop(_uartmqttLoop);

}

#endif // UART_MQTT_SUPPORT
