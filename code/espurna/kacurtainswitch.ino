/*
 * KA_CURTAIN MODULE
  wifi_kingartcurtainswitch.ino - KingArt Cover/Shutter/Blind/Curtain support for ESPURNA
  Based on xdrv_19_ps16dz.dimmer.ino - PS_16_DZ dimmer support for Tasmota
  Copyright (C) 2019 by Albert Weterings
*/
/*
Buttons on the device will move Cover/Shutter/Blind/Curtain up/open or down/close On the end of
every movement the unit reports the last action and posiston over MQTT topic {hostname}/curtainin
report format looks like:
"AT+UPDATE="switch":"on","setclose":13"
"AT+UPDATE="switch":"off","setclose":38"
"AT+UPDATE="switch":"pause","setclose":75"

The device is listning to MQTT topic {hostname}/curtainout/set
you can send the position 0-100 as string or pause to stop the movement
The device its self will detairmain the requested direction.

- Set the Cover/Shutter/Blind/Curtain run time
Tthe factory default Open and Close run time for the switch is 50 seconds, it must be set to 
the accurate run time for smooth working. Some of the Cover/Shutter/Blind/Curtain motor do not
have the resistance stop function, so when the Cover/Shutter/Blind/Curtain track open or close
to the max. length, the Cover/Shutter/Blind/Curtain motor keeps on running if not manual
paused/stoped. This might cause damage on the motor and switch, it also waste energy. In order
to protect the Cover/Shutter/Blind/Curtain motor and save energy, this switch designed with a
time setting function. After set up the run time, the switch will automaticly stop when the
Cover/Shutter/Blind/Curtain open/close to the maximum length. The run time setting is also
helpful for the accurate persentage when operate by drag a progress bar.

After installed and connect the switch with the motor. For the first time power up the switch,
it will automatically close the Cover/Shutter/Blind/Curtain to the maximum.
 - Press and hold the touch interface pause button for around 4 seconds till the red background
   led come up and blinking, then press the open touch button, the Cover/Shutter/Blind/Curtain
   start to open, the switch start to calculate the run time.
 - When the Cover/Shutter/Blind/Curtain is fully open, press the Open or Close button to stop
   the time calculating. The switch record the run time.

 - Press up/down for 5 seconds to bring device into AP mode. Press up/down again device will
   restart in normal mode.
*/
#include <TimeLib.h> //we need this library to make now() working.
#if KA_CURTAIN_SUPPORT

char _KACurtainBuffer[KA_CURTAIN_BUFFER_SIZE];
bool _KACurtainNewData = false;
#define KA_CURTAIN_PORT  KA_CURTAIN_HW_PORT

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

void _KACurtainReceiveUART() {
    static unsigned char ndx = 0;
    while (KA_CURTAIN_PORT.available() > 0 && !_KACurtainNewData) {
        char rc = KA_CURTAIN_PORT.read();
        if (rc != KA_CURTAIN_TERMINATION) {
            _KACurtainBuffer[ndx] = rc;
            if (ndx < KA_CURTAIN_BUFFER_SIZE - 1) ndx++;
        } else {
            _KACurtainBuffer[ndx] = '\0';
            _KACurtainNewData = true;
            ndx = 0;
        }
    }
}

void _kacurtainSendMQTT() {
    if (_KACurtainNewData == true && MQTT_SUPPORT) {
        mqttSend(MQTT_TOPIC_CURTAININ, _KACurtainBuffer);
        _KACurtainNewData = false;
        if (String(_KACurtainBuffer).indexOf("enterESPTOUCH") > 0 ) {
          wifiStartAP();
        } else if (String(_KACurtainBuffer).indexOf("exitESPTOUCH") > 0 ) {
          deferredReset(100, CUSTOM_RESET_HARDWARE);
        }
    }
}

void _KACurtainActionSelect(const char * message) {
  if (String(message) == "pause") {
    _KACurtainPause(message);
  } else {
    _KACurtainSetclose(message);
  }
}

void _KACurtainPause(const char * message) {
  // Tell N76E003AT20 to stop moving and report current position
  char tx_buffer[80];
  snprintf_P(tx_buffer, sizeof(tx_buffer), PSTR("AT+UPDATE=\"sequence\":\"%d%03d\",\"switch\":\"%s\""),
   now(), millis()%1000, message);
  _KACurtainSend(tx_buffer);
}

void _KACurtainSetclose(const char * message) {
  // Tell N76E003AT20 to go to position X (based on X N76E003AT20 decides to go up or down)
  char tx_buffer[80];
  // %d   = long / uint8_t
  // %03d = long / uint8_t - last 3 digits
  // %s   = char
  snprintf_P(tx_buffer, sizeof(tx_buffer), PSTR("AT+UPDATE=\"sequence\":\"%d%03d\",\"switch\":\"%s\",\"setclose\":%s"),
   now(), millis()%1000, "off", message);
  _KACurtainSend(tx_buffer);
}

void _KACurtainSendOk() {
  // Confirm N76E003AT20 message received and stop repeating
  if (_KACurtainNewData == true) {
    KA_CURTAIN_PORT.print("AT+SEND=ok");
    KA_CURTAIN_PORT.write(0x1B);
    KA_CURTAIN_PORT.flush();
  }
}

void _KACurtainSend(const char * tx_buffer) {
  KA_CURTAIN_PORT.print(tx_buffer);
  KA_CURTAIN_PORT.write(0x1B);
  KA_CURTAIN_PORT.flush();
}

void _KACurtainCallback(unsigned int type, const char * topic, const char * payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_CURTAINOUT);
    }
    if (type == MQTT_MESSAGE_EVENT) {
        // Match topic
        String t = mqttMagnitude((char *) topic);
        if (t.equals(MQTT_TOPIC_CURTAINOUT)) {
          _KACurtainActionSelect(payload);
        }
    }
}

// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

void _kacurtainLoop() {
    _KACurtainReceiveUART();
    _KACurtainSendOk();
    _kacurtainSendMQTT();
}

void kacurtainSetup() {

    // Init port
    KA_CURTAIN_PORT.begin(KA_CURTAIN_BAUDRATE);

    // Register MQTT callbackj
    mqttRegister(_KACurtainCallback);

    // Register loop
    espurnaRegisterLoop(_kacurtainLoop);
}

#endif // KA_CURTAIN_SUPPORT
