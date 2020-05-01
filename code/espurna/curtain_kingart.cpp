/*

KingArt Cover/Shutter/Blind/Curtain support for ESPURNA

Based on xdrv_19_ps16dz.dimmer.ino, PS_16_DZ dimmer support for Tasmota
Copyright (C) 2019 by Albert Weterings

*/

#include "curtain_kingart.h"

#if KINGART_CURTAIN_SUPPORT

#include "ntp.h"
#include "mqtt.h"

#ifndef KINGART_CURTAIN_PORT
#define KINGART_CURTAIN_PORT         Serial      // Hardware serial port by default
#endif

#ifndef KINGART_CURTAIN_BUFFER_SIZE
#define KINGART_CURTAIN_BUFFER_SIZE  100         // Local UART buffer size
#endif

#define KINGART_CURTAIN_TERMINATION  '\e'        // Termination character after each message
#define KINGART_CURTAIN_BAUDRATE     19200       // Serial speed is fixed for the device
  
char _KACurtainBuffer[KINGART_CURTAIN_BUFFER_SIZE];
bool _KACurtainNewData = false;
  
// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

void _KACurtainSend(const char * tx_buffer) {
    KINGART_CURTAIN_PORT.print(tx_buffer);
    KINGART_CURTAIN_PORT.print(KINGART_CURTAIN_TERMINATION);
    KINGART_CURTAIN_PORT.flush();
}
  
void _KACurtainReceiveUART() {
    static unsigned char ndx = 0;
    while (KINGART_CURTAIN_PORT.available() > 0 && !_KACurtainNewData) {
        char rc = KINGART_CURTAIN_PORT.read();
        if (rc != KINGART_CURTAIN_TERMINATION) {
            _KACurtainBuffer[ndx] = rc;
            if (ndx < KINGART_CURTAIN_BUFFER_SIZE - 1) ndx++;
        } else {
            _KACurtainBuffer[ndx] = '\0';
            _KACurtainNewData = true;
            ndx = 0;
        }
    }
}

/*
Buttons on the device will move Cover/Shutter/Blind/Curtain up/open or down/close On the end of
every movement the unit reports the last action and posiston over MQTT topic {hostname}/curtain

RAW paylod format looks like:
AT+UPDATE="switch":"on","setclose":13
AT+UPDATE="switch":"off","setclose":38
AT+UPDATE="switch":"pause","setclose":75

The device is listening to MQTT topic {hostname}/curtain/set, to which you can send:
- position value, in range from 0 to 100
- "pause", to stop the movement.
The device will determine the direction all by itself.

# Set the Cover / Shutter / Blind / Curtain run time

The factory default Open and Close run time for the switch is 50 seconds, and it must be set to 
an accurate run time for smooth working. Some motors do not have the resistance stop function,
so when the Cover/Shutter/Blind/Curtain track open or close to the maximum length, but the motor keeps on running.
This might cause damage on the motor and the switch, it also wastes a lot of energy. In order
to protect the motor, this switch designed with a time setting function. After setting up the run time,
the switch will automaticly stop when the track reaches its limits. The run time setting is also helpful
for the accurate control when manually controlling the device via the touch panel.

After installing the switch and connecting the switch for the very first time:
- First, it will automatically close the Cover/Shutter/Blind/Curtain to the maximum.
- Press and hold the touch interface pause button for around 4 seconds until the red background
  led lights up and starts blinking. Then, press the open touch button so start the opening process.
- When cover is fully open, press the Open or Close button to stop the timer and save the calculated run time.

To configure the device:
- Press up/down for 5 seconds to bring device into AP mode. After pressing up/down again, device will restart in normal mode.
*/

void _KACurtainResult() {
    if (_KACurtainNewData) {

        // Need to send confiramtion to the N76E003AT20 that message is received
        _KACurtainSend("AT+SEND=ok");

        // We don't handle "setclose" any other way, simply redirect payload value
        const String buffer(_KACurtainBuffer);
        #if MQTT_SUPPORT
            int setclose_idx = buffer.indexOf("setclose");
            if (setclose_idx > 0) {
                auto position = buffer.substring(setclose_idx + strlen("setclose") + 2, buffer.length());
                int leftovers = position.indexOf(',');
                if (leftovers > 0) {
                    position = position.substring(0, leftovers);
                }
                mqttSend(MQTT_TOPIC_CURTAIN, position.c_str());
            }
        #endif // MQTT_SUPPORT

        // Handle configuration button presses
        if (buffer.indexOf("enterESPTOUCH") > 0) {
            wifiStartAP();
        } else if (buffer.indexOf("exitESPTOUCH") > 0) {
            deferredReset(100, CUSTOM_RESET_HARDWARE);
        }

        _KACurtainNewData = false; 
    }
}
  
// %d   = now() / time_t / NTP timestamp in seconds
// %03u = millis() / uint32_t / we need last 3 digits
// %s   = char strings for various actions
  
// Tell N76E003AT20 to stop moving and report current position
void _KACurtainPause(const char * message) {
    char tx_buffer[80] = {0};
    snprintf_P(
        tx_buffer, sizeof(tx_buffer),
        PSTR("AT+UPDATE=\"sequence\":\"%d%03u\",\"switch\":\"%s\""),
        now(), millis() % 1000,
        message
    );
    _KACurtainSend(tx_buffer);
}
  
// Tell N76E003AT20 to go to position X (based on X N76E003AT20 decides to go up or down)
void _KACurtainSetclose(const char * message) {
    char tx_buffer[80] = {0};
    snprintf_P(
        tx_buffer, sizeof(tx_buffer),
        PSTR("AT+UPDATE=\"sequence\":\"%d%03u\",\"switch\":\"%s\",\"setclose\":%s"),
        now(), millis() % 1000,
        "off", message
    );
    _KACurtainSend(tx_buffer);
}
  
#if MQTT_SUPPORT

void _KACurtainActionSelect(const char * message) {
    if (strcmp(message, "pause") == 0) {
        _KACurtainPause(message);
    } else {
        _KACurtainSetclose(message);
    }
}

void _KACurtainCallback(unsigned int type, const char * topic, char * payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_CURTAIN);
    }
    if (type == MQTT_MESSAGE_EVENT) {
        // Match topic
        const String t = mqttMagnitude(const_cast<char*>(topic));
        if (t.equals(MQTT_TOPIC_CURTAIN)) {
            _KACurtainActionSelect(payload);
        }
    }
}

#endif // MQTT_SUPPORT
  
// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------
  
void _KACurtainLoop() {
    _KACurtainReceiveUART();
    _KACurtainResult();
}

void kingartCurtainSetup() {

    // Init port to receive and send messages
    KINGART_CURTAIN_PORT.begin(KINGART_CURTAIN_BAUDRATE);

    // Register MQTT callback only when supported
    #if MQTT_SUPPORT
        mqttRegister(_KACurtainCallback);
    #endif // MQTT_SUPPORT

    // Register loop to poll the UART for new messages
    espurnaRegisterLoop(_KACurtainLoop);

}
  
#endif // KINGART_CURTAIN_SUPPORT
