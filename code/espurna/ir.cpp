/*

IR MODULE

Copyright (C) 2018 by Alexander Kolesnikov (raw and MQTT implementation)
Copyright (C) 2017-2019 by François Déchery
Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

-----------------------------------------------------------------------------
Configuration
-----------------------------------------------------------------------------

To enable transmit functions define IR_TX_PIN
To enable receiver functions define IR_RX_PIN
MQTT input topic: {root}/irin
MQTT output topic: {root}/irout/set

--------------------------------------------------------------------------------
MQTT messages
--------------------------------------------------------------------------------

Decoded messages:

    Transmitting:
    Payload: 2:121944:32:1 (<type>:<code>:<bits>[:<repeat>])
    The repeat value is optional and defaults to 1

    Receiving:
    Payload: 2:121944:32 (<type>:<code>:<bits>)

Raw messages:

    Transmitting:
    Payload: 1000,1000,1000,1000,1000,DELAY,COUNT,FREQ:500,500,500,500,500
             |        IR codes      |                  | IR repeat codes |
    codes - time in microseconds when IR LED On/Off. First value - ON, second - Off ...
    DELAY - delay in milliseconds between sending repeats
    COUNT - how many repeats send. Max 120.
    FREQ - modulation frequency. Usually 38kHz. You may set 38, it means 38kHz or set 38000, it meant same.
    Repeat codes is optional. You may omit ":" and codes. In this case if repeat count > 0 we repeat main code.

    Receiving:
    Payload: 1000,1000,1000,1000,1000
             |        IR codes      |

--------------------------------------------------------------------------------
*/

#include "ir.h"

#if IR_SUPPORT

#include "light.h"
#include "mqtt.h"
#include "relay.h"

#if defined(IR_RX_PIN)
    IRrecv _ir_receiver(IR_RX_PIN, IR_BUFFER_SIZE, IR_TIMEOUT, true);
    decode_results _ir_results;
#endif // defined(IR_RX_PIN)

#if defined(IR_TX_PIN)
    IRsend _ir_sender(IR_TX_PIN);

    #if IR_USE_RAW
        uint16_t _ir_freq = 38;        // IR modulation freq. for sending codes and repeat codes
        uint8_t _ir_repeat_size = 0;   // size of repeat array
        uint16_t * _ir_raw;            // array for sending codes and repeat codes
    #else
        uint8_t _ir_type = 0;          // Type of encoding
        uint64_t _ir_code = 0;         // Code to transmit
        uint16_t _ir_bits = 0;         // Code bits
    #endif

    uint8_t _ir_repeat = 0;            // count of times repeating of repeat_code
    uint32_t _ir_delay = IR_DELAY;    // delay between repeat codes

#endif // defined(IR_TX_PIN)

// MQTT to IR
#if MQTT_SUPPORT && defined(IR_TX_PIN)

void _irMqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(MQTT_TOPIC_IROUT);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        String t = mqttMagnitude((char *) topic);

        // Match topic
        if (t.equals(MQTT_TOPIC_IROUT)) {

            String data = String(payload);
            unsigned int len = data.length();
            int col = data.indexOf(":"); // position of ":" which means repeat_code

            #if IR_USE_RAW

                unsigned char count = 1; // count of code values for allocating array

                if (col > 2) { // count & validating repeat code

                    _ir_repeat_size = 1;

                    // count & validate repeat-string
                    for(unsigned int i = col+1; i < len; i++) {
                        if (i < len-1) {
                            if ( payload[i] == ',' && isDigit(payload[i+1]) && i>0 ) { //validate string
                                _ir_repeat_size++;
                            } else if (!isDigit(payload[i])) {
                                // Error in repeat_code. Use comma separated unsigned integer values.
                                // Last three is repeat delay, repeat count(<120) and frequency.
                                // After all you may write ':' and specify repeat code followed by comma.
                                DEBUG_MSG_P(PSTR("[IR] Error in repeat code.\n"));
                                return;
                            }
                        }
                    }

                    len = col; //cut repeat code from main code processing

                } // end of counting & validating repeat code

                // count & validate main code string
                for(unsigned int i = 0; i < len; i++) {
                    if (i<len-1) {
                        if ( payload[i] == ',' && isDigit(payload[i+1]) && i>0 ) { //validate string
                            count++;
                        } else if (!isDigit(payload[i])) {
                            // Error in main code. Use comma separated unsigned integer values.
                            // Last three is repeat delay, repeat count(<120) and frequency.
                            // After all you may write ':' and specify repeat code followed by comma.
                            DEBUG_MSG_P(PSTR("[IR] Error in main code.\n"));
                            return;
                        }
                    }

                }

                _ir_raw = (uint16_t*)calloc(count, sizeof(uint16_t)); // allocating array for main codes
                String value = ""; // for populating values of array from comma separated string
                int j = 0; // for populating values of array from comma separated string

                // populating main code array from part of MQTT string
                for (unsigned int i = 0; i < len; i++) {
                    if (payload[i] != ',') {
                        value = value + data[i];
                    }
                    if ((payload[i] == ',') || (i == len - 1)) {
                        _ir_raw[j]= value.toInt();
                        value = "";
                        j++;
                    }
                }

                // if count>3 then we have values, repeat delay, count and modulation frequency
                _ir_repeat=0;
                if (count>3) {
                    if (_ir_raw[count-2] <= 120) { // if repeat count > 120 it's to long and ussualy unusual. maybe we get raw code without this parameters and just use defaults for freq.
                        _ir_freq = _ir_raw[count-1];
                        _ir_repeat = _ir_raw[count-2];
                        _ir_delay = _ir_raw[count-3];
                        count = count - 3;
                    }
                }

                DEBUG_MSG_P(PSTR("[IR] Raw IR output %d codes, repeat %d times on %d(k)Hz freq.\n"), count, _ir_repeat, _ir_freq);

                #if defined(IR_RX_PIN)
                    _ir_receiver.disableIRIn();
                #endif
                _ir_sender.sendRaw(_ir_raw, count, _ir_freq);

                if (_ir_repeat==0) { // no repeat, cleaning array, enabling receiver
                    free(_ir_raw);
                    #if defined(IR_RX_PIN)
                        _ir_receiver.enableIRIn();
                    #endif
                } else if (col>2) { // repeat with repeat_code

                    DEBUG_MSG_P(PSTR("[IR] Repeat codes count: %d\n"), _ir_repeat_size);

                    free(_ir_raw);
                    _ir_raw = (uint16_t*)calloc(_ir_repeat_size, sizeof(uint16_t));

                    String value = ""; // for populating values of array from comma separated string
                    int j = 0; // for populating values of array from comma separated string
                    len = data.length(); //redifining length to full lenght

                    // populating repeat code array from part of MQTT string
                    for (unsigned int i = col+1; i < len; i++) {
                        value = value + data[i];
                        if ((payload[i] == ',') || (i == len - 1)) {
                            _ir_raw[j]= value.toInt();
                            value = "";
                            j++;
                        }
                    }
                } else { // if repeat code not specified (col<=2) repeat with current main code
                    _ir_repeat_size = count;
                }

            #else

                _ir_repeat = 0;

                if (col > 0) {

                    _ir_type = data.toInt();
                    _ir_code = strtoul(data.substring(col+1).c_str(), NULL, 10);

                    col = data.indexOf(":", col+1);
                    if (col > 0) {
                        _ir_bits = data.substring(col+1).toInt();
                        col = data.indexOf(":", col+1);
                        if (col > 2) {
                            _ir_repeat = data.substring(col+1).toInt();
                        } else {
                            _ir_repeat = IR_REPEAT;
                        }
                    }
                }

                if (_ir_repeat > 0) {
                    DEBUG_MSG_P(PSTR("[IR] IROUT: %d:%lu:%d:%d\n"), _ir_type, (unsigned long) _ir_code, _ir_bits, _ir_repeat);
                } else {
                    DEBUG_MSG_P(PSTR("[IR] Wrong MQTT payload format (%s)\n"), payload);
                }

            #endif // IR_USE_RAW

        } // end of match topic

    } // end of MQTT message

} //end of function

void _irTXLoop() {

    static uint32_t last = 0;
    if ((_ir_repeat > 0) && (millis() - last > _ir_delay)) {
        last = millis();

        // Send message
        #if IR_USE_RAW
            _ir_sender.sendRaw(_ir_raw, _ir_repeat_size, _ir_freq);
        #else
            _ir_sender.send(_ir_type, _ir_code, _ir_bits);
        #endif

        // Update repeat count
        --_ir_repeat;
        if (0 == _ir_repeat) {
            #if IR_USE_RAW
                free(_ir_raw);
            #endif
            #if defined(IR_RX_PIN)
                _ir_receiver.enableIRIn();
            #endif
        }

    }

}

#endif // MQTT_SUPPORT && defined(IR_TX_PIN)

// Receiving

#if defined(IR_RX_PIN)

void _irProcess(unsigned char type, unsigned long code) {

    #if IR_BUTTON_SET > 0

        boolean found = false;

        for (unsigned char i = 0; i < IR_BUTTON_COUNT ; i++) {

            uint32_t button_code = pgm_read_dword(&IR_BUTTON[i][0]);
            if (code == button_code) {

                unsigned long button_action = pgm_read_dword(&IR_BUTTON[i][1]);
                unsigned long button_value = pgm_read_dword(&IR_BUTTON[i][2]);

                switch (button_action) {

                #if RELAY_SUPPORT
                    case IR_BUTTON_ACTION_STATE:
                        relayStatus(0, button_value);
                        break;

                    case IR_BUTTON_ACTION_TOGGLE:
                        relayToggle(button_value);
                        break;
                #endif // RELAY_SUPPORT == 1

                #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

                    case IR_BUTTON_ACTION_BRIGHTER:
                        lightBrightnessStep(button_value ? 1 : -1);
                        lightUpdate(true, true);
                        nice_delay(150); //debounce
                        break;

                    case IR_BUTTON_ACTION_RGB:
                        lightColor(button_value);
                        lightUpdate(true, true);
                        break;

                /*
                #if LIGHT_PROVIDER == LIGHT_PROVIDER_FASTLED
                    case IR_BUTTON_ACTION_EFFECT:
                        _buttonAnimMode(button_value);
                        break;
                #endif
                */

                /*
                    case IR_BUTTON_ACTION_HSV:
                        lightColor(button_value);
                        break;
                */

                }

                #endif // LIGHT_PROVIDER != LIGHT_PROVIDER_NONE

                found = true;
                break;

    		}

    	}

    	if (!found) {
    		DEBUG_MSG_P(PSTR("[IR] Code does not match any action\n"));
    	}

    #endif

}

void _irRXLoop() {

    if (_ir_receiver.decode(&_ir_results)) {

        _ir_receiver.resume(); // Receive the next value

        // Debounce
        static unsigned long last_time = 0;
        if (millis() - last_time < IR_DEBOUNCE) return;
        last_time = millis();

        #if IR_USE_RAW
            // Check code
            if (_ir_results.rawlen < 1) return;
            char * payload;
            String value = "";
            for (int i = 1; i < _ir_results.rawlen; i++) {
                if (i>1) value = value + ",";
                value = value + String(_ir_results.rawbuf[i] * RAWTICK);
            }
            payload = const_cast<char*>(value.c_str());
        #else
            // Check code
            if (_ir_results.value < 1) return;
            if (_ir_results.decode_type < 1) return;
            if (_ir_results.bits < 1) return;
            char payload[32];
            snprintf_P(payload, sizeof(payload), PSTR("%u:%lu:%u"), _ir_results.decode_type, (unsigned long) _ir_results.value, _ir_results.bits);
        #endif

        DEBUG_MSG_P(PSTR("[IR] IRIN: %s\n"), payload);

        #if not IR_USE_RAW
            _irProcess(_ir_results.decode_type, (unsigned long) _ir_results.value);
        #endif

        #if MQTT_SUPPORT
            if (strlen(payload)>0) {
                mqttSend(MQTT_TOPIC_IRIN, (const char *) payload);
            }
        #endif

    }

}

#endif // defined(IR_RX_PIN)

// -----------------------------------------------------------------------------

void _irLoop() {
    #if defined(IR_RX_PIN)
        _irRXLoop();
    #endif
    #if MQTT_SUPPORT && defined(IR_TX_PIN)
        _irTXLoop();
    #endif
}

void irSetup() {

    #if defined(IR_RX_PIN)
        _ir_receiver.enableIRIn();
        DEBUG_MSG_P(PSTR("[IR] Receiver initialized \n"));
    #endif

    #if MQTT_SUPPORT && defined(IR_TX_PIN)
        _ir_sender.begin();
        mqttRegister(_irMqttCallback);
        DEBUG_MSG_P(PSTR("[IR] Transmitter initialized \n"));
    #endif

    espurnaRegisterLoop(_irLoop);

}

#endif // IR_SUPPORT
