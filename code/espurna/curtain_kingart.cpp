/*

KingArt Cover/Shutter/Blind/Curtain support for ESPURNA

Based on xdrv_19_ps16dz.dimmer.ino, PS_16_DZ dimmer support for Tasmota
Copyright (C) 2019 by Albert Weterings

Based on curtain_kingart.ino Albert Weterings
Copyright (C) 2020 - Eric Chauvet

*/

#include "espurna.h"

#if KINGART_CURTAIN_SUPPORT

#include "curtain_kingart.h"
#include "mqtt.h"
#include "ntp.h"
#include "ntp_timelib.h"
#include "settings.h"
#include "ws.h"

#ifndef KINGART_CURTAIN_PORT
#define KINGART_CURTAIN_PORT         Serial      // Hardware serial port by default
#endif

#ifndef KINGART_CURTAIN_BUFFER_SIZE
#define KINGART_CURTAIN_BUFFER_SIZE  100         // Local UART buffer size
#endif

#define KINGART_CURTAIN_TERMINATION  '\e'        // Termination character after each message
#define KINGART_CURTAIN_BAUDRATE     19200       // Serial speed is fixed for the device

// --> Let see after if we define a curtain generic switch, use these for now
#define CURTAIN_BUTTON_UNKNOWN       -1
#define CURTAIN_BUTTON_PAUSE         0
#define CURTAIN_BUTTON_OPEN          1
#define CURTAIN_BUTTON_CLOSE         2


#define CURTAIN_INIT_CLOSE          1
#define CURTAIN_INIT_OPEN           2
#define CURTAIN_INIT_POSITION       3

#define CURTAIN_POSITION_UNKNOWN     -1
// <--

#define KINGART_DEBUG_MSG_P(...) do { if (_curtain_debug_flag) { DEBUG_MSG_P(__VA_ARGS__); } } while(0)

char _KACurtainBuffer[KINGART_CURTAIN_BUFFER_SIZE];
bool _KACurtainNewData = false;

// Status vars - for curtain move detection :
int _curtain_position               = CURTAIN_POSITION_UNKNOWN;
int _curtain_last_position          = CURTAIN_POSITION_UNKNOWN;
int _curtain_button                 = CURTAIN_BUTTON_UNKNOWN;
int _curtain_last_button            = CURTAIN_BUTTON_UNKNOWN;
unsigned long last_uptime           = 0;

int _curtain_position_set           = CURTAIN_POSITION_UNKNOWN; //Last position asked to be set (not the real position, the real query - updated when the curtain stops moving)
bool _curtain_waiting_ack           = false; //Avoid too fast MQTT commands
bool _curtain_ignore_next_position  = false; //Avoid a bug (see (*1)
bool _curtain_initial_position_set  = false; //Flag to detect if we manage to set the curtain back to its position before power off or reboot

// Calculated behaviour depending on KA switch, MQTT and UI actions
bool _curtain_moving                = true;

//Enable more traces, true as a default and stopped when curtain is setup.
bool _curtain_debug_flag = true;

#if WEB_SUPPORT
bool _curtain_report_ws = true; //This will init curtain control and flag the web ui update
#endif // WEB_SUPPORT


//------------------------------------------------------------------------------
void curtainUpdateUI() {
#if WEB_SUPPORT
    _curtain_report_ws = true;
#endif // WEB_SUPPORT
}

//------------------------------------------------------------------------------
int setButtonFromSwitchText(String & text) {
    if(text == "on")
        return CURTAIN_BUTTON_OPEN;
    else if(text == "off")
        return CURTAIN_BUTTON_CLOSE;
    else if(text == "pause")
        return CURTAIN_BUTTON_PAUSE;
    else
        return CURTAIN_BUTTON_UNKNOWN;
}

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

//------------------------------------------------------------------------------
//This check that wa got latest and new stats from the AT+RESULT message
bool _KAValidStatus() {
   return   _curtain_button != CURTAIN_BUTTON_UNKNOWN &&
            _curtain_last_button != CURTAIN_BUTTON_UNKNOWN &&
            _curtain_position != CURTAIN_POSITION_UNKNOWN &&
            _curtain_last_position != CURTAIN_POSITION_UNKNOWN;
}


//------------------------------------------------------------------------------
//We consider that the curtain is moving. A timer is set to get the position of the curtain sending AT+START messages in the loop()
void _KASetMoving() {
    last_uptime = millis() + 1000; //Let the returned curtain position to be refreshed to know of the curtain is still moving
     _curtain_moving = true;
}

//------------------------------------------------------------------------------
//Send a buffer to serial
void _KACurtainSend(const char * tx_buffer) {
    KINGART_CURTAIN_PORT.print(tx_buffer);
    KINGART_CURTAIN_PORT.print(KINGART_CURTAIN_TERMINATION);
    KINGART_CURTAIN_PORT.flush();
    KINGART_DEBUG_MSG_P(PSTR("[KA] UART OUT %s\n"), tx_buffer);
}

//------------------------------------------------------------------------------
//Send a formatted message to MCU
void _KACurtainSet(int button, int position = CURTAIN_POSITION_UNKNOWN) {

   if(_curtain_waiting_ack) {
        KINGART_DEBUG_MSG_P(PSTR("[KA] UART ACK not received : Request ignored!\n"));
        return;
   }

    char tx_buffer[80] = {0};
    if(button != CURTAIN_BUTTON_UNKNOWN && position != CURTAIN_POSITION_UNKNOWN) {
        snprintf_P(
            tx_buffer, sizeof(tx_buffer),
            PSTR("AT+UPDATE=\"sequence\":\"%d%03u\",\"switch\":\"%s\",\"setclose\":%d"),
            now(), millis() % 1000,
            (button == CURTAIN_BUTTON_PAUSE ? "pause" : (button == CURTAIN_BUTTON_OPEN ? "on" : "off")), position
        );
    } else if(button == CURTAIN_BUTTON_UNKNOWN) {
        snprintf_P(
            tx_buffer, sizeof(tx_buffer),
            PSTR("AT+UPDATE=\"sequence\":\"%d%03u\",\"setclose\":%d"),
            now(), millis() % 1000,
           position
        );
    } else {
            snprintf_P(
            tx_buffer, sizeof(tx_buffer),
            PSTR("AT+UPDATE=\"sequence\":\"%d%03u\",\"switch\":\"%s\""),
            now(), millis() % 1000,
            (button == CURTAIN_BUTTON_PAUSE ? "pause" : (button == CURTAIN_BUTTON_OPEN ? "on" : "off"))
        );
    }
    _KACurtainSend(tx_buffer);
    _curtain_waiting_ack = true;
}

//------------------------------------------------------------------------------
//Stop moving will set the real curtain position to the GUI/MQTT
void _KAStopMoving() {
     _curtain_moving = false;
     if( _curtain_position != CURTAIN_POSITION_UNKNOWN)
        _curtain_position_set = _curtain_position;
    else if( _curtain_last_position != CURTAIN_POSITION_UNKNOWN)
        _curtain_position_set = _curtain_last_position;

    if (!_curtain_initial_position_set) { //The curtain stopped moving for the first time, set the position back to
        int init_position = getSetting("curtainBoot", 0);
        KINGART_DEBUG_MSG_P(PSTR("[KA] curtainBoot : %d, curtainBootPos : %d\n"), init_position, getSetting("curtainBootPos", 100));
        if (init_position == CURTAIN_INIT_CLOSE) {
             _KACurtainSet(CURTAIN_BUTTON_CLOSE);
        } else if (init_position == CURTAIN_INIT_OPEN) {
             _KACurtainSet(CURTAIN_BUTTON_OPEN);
        } else if (init_position == CURTAIN_INIT_POSITION) {
            int pos = getSetting("curtainBootPos", 100); //Set closed if we do not have initial position set.
            if (_curtain_position_set != pos) {
                _KACurtainSet(CURTAIN_BUTTON_UNKNOWN, pos);
            }
        }
        _curtain_initial_position_set = true;
        _curtain_debug_flag = false; //Disable debug - user has could ask for it
    } else if(_curtain_position_set != CURTAIN_POSITION_UNKNOWN && _curtain_position_set != getSetting("curtainBootPos", _curtain_position_set)) {
           setSetting("curtainBootPos", _curtain_last_position); //Remeber last position in case of power loss
    }
}

//------------------------------------------------------------------------------
//Receive a buffer from serial
bool _KACurtainReceiveUART() {
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
    if(_KACurtainNewData) {
        KINGART_DEBUG_MSG_P(PSTR("[KA] Serial received : %s\n"), _KACurtainBuffer);
        _KACurtainNewData = false;
        return true;
    }
    return false;
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

//------------------------------------------------------------------------------
void _KACurtainResult() {

    // Need to send confirmation to the N76E003AT20 that message is received
    // ECH : TODO Check this is the case every time
    _KACurtainSend("AT+SEND=ok");

    //Init receive stats : The buffer which may contain : "setclose":INT(0-100) or "switch":["on","off","pause"]
    const String buffer(_KACurtainBuffer);
    _curtain_button = CURTAIN_BUTTON_UNKNOWN;
    _curtain_position = CURTAIN_POSITION_UNKNOWN;


    if(buffer.indexOf("AT+RESULT") == 0) { //AT+RESULT is an acquitment of our command (MQTT or GUI)
        //Set the status on what we kown
        if( ( _curtain_last_button == CURTAIN_BUTTON_OPEN && _curtain_last_position == 0 ) ||
            ( _curtain_last_button == CURTAIN_BUTTON_CLOSE && _curtain_last_position == 100 ) ||
            _curtain_last_button == CURTAIN_BUTTON_PAUSE) { //The curtain is max opened, closed or pause
            _KAStopMoving();
        } else { //Else it is probably moving
            _KASetMoving();
            /*
                (*1) ATTENTION THERE :
                Send immediatly a AT+START - we need to purge the first response.
                It will return us the right direction of the switch but the position
                we set instead of the real on. We take care of the switch response but
                we ignore the position.
            */
            _KACurtainSend("AT+START");
            _curtain_ignore_next_position = true;
        }
        //Time to update UI
        curtainUpdateUI();
        _curtain_waiting_ack = false;
        return;
    } else if(buffer.indexOf("AT+UPDATE") == 0) { //AT+UPDATE is a response from the switch itself or AT+SEND query
        // Get switch status from MCU
        int switch_idx = buffer.indexOf("switch");
        if (switch_idx > 0) {
            String switch_text = buffer.substring(switch_idx + strlen("switch") + 3, buffer.length());
            int leftovers = switch_text.indexOf('"');
            if (leftovers > 0) { //We must find leftover as it is text
                switch_text = switch_text.substring(0, leftovers);
                _curtain_button = setButtonFromSwitchText(switch_text);
            }
        }
        // Get position from MCU
        int setclose_idx = buffer.indexOf("setclose");
        if (setclose_idx > 0) {
            String position = buffer.substring(setclose_idx + strlen("setclose") + 2, buffer.length());
            int leftovers = position.indexOf(',');
            if (leftovers > 0) { // Not found if finishing by setclose
                position = position.substring(0, leftovers);
            }
            if(_curtain_ignore_next_position) { // (*1)
                _curtain_ignore_next_position = false;
            } else {
                _curtain_position = position.toInt();
            }
        }
    } else {
        KINGART_DEBUG_MSG_P(PSTR("[KA] ERROR : Serial unknown message : %s\n"), _KACurtainBuffer);
    }

    //Check if curtain is moving or not
    if( _curtain_button == CURTAIN_BUTTON_PAUSE ) { //This is returned from MCU and tells us than last status is pause or full opened or closed
       _KAStopMoving();
    } else if(_curtain_moving ) {
        if(_KAValidStatus()) {
            if(_curtain_last_button != _curtain_button) //Direction change? Reset the timer to know
                _KASetMoving();
            else if(_curtain_last_position == _curtain_position) //Same direction, same position - curtain is not moving anymore
                _KAStopMoving();
       }
    } else { //Not paused, not moving, and we received an AT+UPDATE -> This means that we are moving
        _KASetMoving();
    }

    //Update last position and transmit to MQTT (GUI is at the end)
    if(_curtain_position != CURTAIN_POSITION_UNKNOWN && _curtain_last_position != _curtain_position) {
        _curtain_last_position = _curtain_position;

        #if MQTT_SUPPORT
        const String pos = String(_curtain_last_position);
        mqttSend(MQTT_TOPIC_CURTAIN, pos.c_str());
        #endif // MQTT_SUPPORT
    }

    //Reset last button to make the algorithm work and set last button state
    if(!_curtain_moving) {
        _curtain_last_button = CURTAIN_BUTTON_UNKNOWN;
    } else if (_curtain_button != CURTAIN_BUTTON_UNKNOWN) {
        _curtain_last_button = _curtain_button;
    }

    // Handle configuration button presses
    if (buffer.indexOf("enterESPTOUCH") > 0) {
        wifiStartAp();
    } else if (buffer.indexOf("exitESPTOUCH") > 0) {
        prepareReset(CustomResetReason::Hardware);
    } else { //In any other case, update as it could be a move action
        curtainUpdateUI();
    }
}

// -----------------------------------------------------------------------------
// MQTT Support
// -----------------------------------------------------------------------------

#if MQTT_SUPPORT

//------------------------------------------------------------------------------
void _curtainMQTTCallback(unsigned int type, const char* topic, char* payload) {
    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_CURTAIN);
    } else if (type == MQTT_MESSAGE_EVENT) {
        // Match topic
        const String t = mqttMagnitude(topic);
        if (t.equals(MQTT_TOPIC_CURTAIN)) {
            if (strcmp(payload, "pause") == 0) {
                _KACurtainSet(CURTAIN_BUTTON_PAUSE);
            } else  if (strcmp(payload, "on") == 0) {
                _KACurtainSet(CURTAIN_BUTTON_OPEN);
            } else  if (strcmp(payload, "off") == 0) {
                _KACurtainSet(CURTAIN_BUTTON_CLOSE);
            } else {
                _curtain_position_set = String(payload).toInt();
                _KACurtainSet(CURTAIN_BUTTON_UNKNOWN, _curtain_position_set);
            }
        }
    }
}

#endif // MQTT_SUPPORT

// -----------------------------------------------------------------------------
// WEB Support
// -----------------------------------------------------------------------------

#if WEB_SUPPORT

//------------------------------------------------------------------------------
void _curtainWebSocketOnConnected(JsonObject& root) {
    root["curtainType"] = getSetting("curtainType", "0");
    root["curtainBoot"] = getSetting("curtainBoot", "0");
}

//------------------------------------------------------------------------------
bool _curtainWebSocketOnKeyCheck(const char * key, JsonVariant& value) {
    if (strncmp(key, "curtain", strlen("curtain")) == 0) return true;
    return false;
}

//------------------------------------------------------------------------------
void _curtainWebSocketUpdate(JsonObject& root) {
    JsonObject& state = root.createNestedObject("curtainState");
    state["get"] = _curtain_last_position;
    if(_curtain_position_set == CURTAIN_POSITION_UNKNOWN) {
        _curtain_position_set = _curtain_last_position;
    }
    state["set"] = _curtain_position_set;
    state["button"] = _curtain_last_button;
    state["moving"] = _curtain_moving;
    state["type"] = getSetting("curtainType", "0");
}

//------------------------------------------------------------------------------
void _curtainWebSocketStatus(JsonObject& root) {
    _curtainWebSocketUpdate(root);
}

//------------------------------------------------------------------------------
void _curtainWebSocketOnAction(uint32_t client_id, const char * action, JsonObject& data) {

    if (strcmp(action, "curtainAction") == 0) {
        if (data.containsKey("position")) {
            _curtain_position_set = data["position"].as<int>();
            _KACurtainSet(CURTAIN_BUTTON_UNKNOWN, _curtain_position_set);
        } else if(data.containsKey("button")){
            _curtain_last_button = data["button"].as<int>();
             _KACurtainSet(_curtain_last_button);
        }
    }
}

void _curtainWebSocketOnVisible(JsonObject& root) {
    wsPayloadModule(root, "curtain");
}

#endif //WEB_SUPPORT

// -----------------------------------------------------------------------------
// SETUP & LOOP
// -----------------------------------------------------------------------------

//------------------------------------------------------------------------------
void _KACurtainLoop() {

    if(_KACurtainReceiveUART()) {
        _KACurtainResult();
    } else if(_curtain_moving) { //When curtain move and no messages, get position every 600ms with AT+START
        unsigned long uptime = millis();
        long diff = uptime - last_uptime;
        if(diff >= 600) {
            _KACurtainSend("AT+START");
            last_uptime = uptime;
        }
    }

#if WEB_SUPPORT
    if (_curtain_report_ws) { //Launch a websocket update
        wsPost(_curtainWebSocketUpdate);
        _curtain_report_ws = false;
    }
 #endif
}

// -----------------------------------------------------------------------------
// Public
// -----------------------------------------------------------------------------

//------------------------------------------------------------------------------
void kingartCurtainSetup() {

    // Init port to receive and send messages
    KINGART_CURTAIN_PORT.begin(KINGART_CURTAIN_BAUDRATE);

#if MQTT_SUPPORT
    // Register MQTT callback only when supported
    mqttRegister(_curtainMQTTCallback);
#endif // MQTT_SUPPORT

#if WEB_SUPPORT
    // Websockets
    wsRegister()
        .onVisible(_curtainWebSocketOnVisible)
        .onConnected(_curtainWebSocketOnConnected)
        .onKeyCheck(_curtainWebSocketOnKeyCheck)
        .onAction(_curtainWebSocketOnAction)
        .onData(_curtainWebSocketUpdate);
#endif

    // Register loop to poll the UART for new messages
    espurnaRegisterLoop(_KACurtainLoop);

}

//------------------------------------------------------------------------------
void curtainSetPosition(unsigned char id, long value) {
    if (id > 1) return;
    _KACurtainSet(CURTAIN_BUTTON_UNKNOWN, constrain(value, 0, 100));
}

unsigned char curtainCount() {
    return 1;
}

#endif // KINGART_CURTAIN_SUPPORT
