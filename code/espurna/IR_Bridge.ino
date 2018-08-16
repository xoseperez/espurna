/*
IR-MQTT Raw Bridge MODULE
Copyright (C) 2018 by Alexander Kolesnikov

Tested on ESP8266, Sonoff RFBridge (8285)
Tested with Espurna 1.13.1 on platform 1.5.0, 1.6.0, 1.7.3, 1.8.0


MQTT input topic: {root}/IRB_RAW_IN
MQTT output topic: {root}/IRB_RAW/set

 -----------------------------------------------------------------------------
 IR-MQTT Raw Bridge

 Usage:

 For enable transmit functions define IRB_TX_PIN
 For enable receiver functions define IRB_RX_PIN

 Call irbSetup(); from bottom of setup() in espurna.ino or add:

// IR Bridge support
#if (defined IRB_RX_PIN) || (defined IRB_TX_PIN)
   irbSetup();
#endif

Transmitting:
 defined IRB_RAW is a MQTT topic for MQTT incoming messages (outgoing by IR).
 Topic: <root>/IRB_RAW/set
 Payload: 1000,1000,1000,1000,1000,DELAY,COUNT,FREQ:500,500,500,500,500
          |        IR codes      |                 | IR repeat codes  |

 codes - time in microseconds when IR LED On/Off. First value - ON, second - Off ...
 DELAY - delay in milliseconds between sending repeats
 COUNT - how many repeats send. Max 120.
 FREQ - modulation frequency. Usually 38kHz. You may set 38, it means 38kHz or set 38000, it meant same.

Receiving:
 defined IRB_RAW_IN is a MQTT topic for IR incoming messages (outgoing to MQTT).
 Topic: <root>/IRB_RAW_IN
 Payload: 1000,1000,1000,1000,1000
          |        IR codes      |
 -----------------------------------------------------------------------------
*/

#if (defined IRB_RX_PIN) || (defined IRB_TX_PIN)
  #include <IRremoteESP8266.h>
#endif

#if (defined IRB_RX_PIN)

  #define IRB_RAW_IN "IRB_RAW_IN"

  #include <IRrecv.h>
  uint16_t CAPTURE_BUFFER_SIZE = 1024;
  #define IR_TIMEOUT 15U
  IRrecv irb_recv(IRB_RX_PIN, CAPTURE_BUFFER_SIZE, IR_TIMEOUT, true);
  decode_results irb_results;
#endif //IRB_RX_PIN

#if (defined IRB_TX_PIN)

  #define IRB_RAW "IRB_RAW"

  #include <IRsend.h>
  IRsend irb_send(IRB_TX_PIN);
  uint8_t irb_repeat=0;
  uint16_t irb_repeat_code[8] = {8938,2208,554,0,0,0,0,0};
  uint32_t irb_delay;
  uint16_t irb_freq=38;
  uint8_t irb_repeat_size=0;
#endif //IRB_TX_PIN

// MQTT to IR
#if MQTT_SUPPORT && (defined IRB_TX_PIN)
void _irbMqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe(IRB_RAW);
    }

    if (type == MQTT_MESSAGE_EVENT) {
        // Match topic
        String t = mqttMagnitude((char *) topic);
        if (t.equals(IRB_RAW)) {
          String strpayload = String(payload);
          unsigned int len = strpayload.length();
          unsigned char count = 1;
          int col = strpayload.indexOf(":");
          String value = "";
          int j = 0;

          if (col>2) { //parsing repeat code
            len = col; //cut repeat code from main code processing
            //DEBUG_MSG_P(PSTR("[IR Bridge] col %d\n"),col);

            for (int i = col+1; i < strpayload.length(); i++) {
              if (j>7) {
                DEBUG_MSG_P(PSTR("[IR Bridge] MQTT payload format error. Repeat code is max 8 codes.\n"));
                return;
              }
             if (payload[i] != ',') {
               value = value + strpayload[i];
              }
             if ((payload[i] == ',') || (i == strpayload.length() - 1)) {
               irb_repeat_code[j]= value.toInt();
               value = "";
               //DEBUG_MSG_P(PSTR("[IR Bridge] repeat code %d\n"),irb_repeat_code[j]);
               j++;
              }
            }
            irb_repeat_size = j;
          }

          // count & validate string
          for(int i = 0; i < len; i++)
          {
            if (i<len-1) {
              if ( payload[i] == ',' && isDigit(payload[i+1]) && i>0 ) { //validate string
                count++;
              } else if (!isDigit(payload[i])) {
                DEBUG_MSG_P(PSTR("[IR Bridge] MQTT payload format error. Use comma separated unsigned\n integer values. Last three is repeat delay, repeat count(<120) and frequency.\nAfter all you may write ':' and specify repeat code followed by comma (max 8 codes).\n"));
                return;
              }
            }
          }

          uint16_t  Raw[count];

          value = "";
          j = 0;
          for (int i = 0; i < len; i++) {
           if (payload[i] != ',') {
              value = value + strpayload[i];
            }
            if ((payload[i] == ',') || (i == len - 1)) {
              Raw[j]= value.toInt();
              value = "";
              j++;
            }
          }

          // if count >2 then we have values and repeat delay, count and modulation frequency
          irb_repeat=0;
          if (count>3) {
            if (Raw[count-2]<=120) {
              irb_freq = Raw[count-1];
              irb_repeat = Raw[count-2];
              irb_delay = Raw[count-3];
              count = count - 3;
            }
          }

          char * irstr;
          if (strpayload.length()!=0){
            irstr = const_cast<char*>(strpayload.c_str());
          }

          DEBUG_MSG_P(PSTR("[IR Bridge] Raw IR output %d values, repeat %d times on %d(k)Hz freq.\n"), count, irb_repeat, irb_freq);
          irb_recv.disableIRIn();
          irb_send.sendRaw(Raw, count, irb_freq);

          if (irb_repeat==0) {
            irb_recv.enableIRIn();
          }
        }
    }
}

void irbRepeatLoop(){
  static uint32_t repeat_millis = 0;
  static uint8_t repeat_i = 0;

  if (irb_repeat>0)
    if (millis()-repeat_millis > irb_delay) {
      //irb_recv.disableIRIn();
      irb_send.sendRaw(irb_repeat_code, irb_repeat_size, irb_freq);
      //DEBUG_MSG_P(PSTR("[IR Bridge] Raw IR repeat %d values on %d(k)Hz freq.\n"), irb_repeat_size, irb_freq);
      //DEBUG_MSG_P(PSTR("[IR Bridge] repeat #%d every %d millis. %d\n"),repeat_i, irb_delay, millis()-repeat_millis);
      repeat_millis = millis();
      repeat_i++;
      if (repeat_i>=irb_repeat) {
        irb_repeat = 0;
        repeat_i = 0;
        irb_recv.enableIRIn();
      }
    }

}
#endif // IRB_TX_PIN & MQTT

// IR to MQTT
#if (defined IRB_RX_PIN)

void irbLoop() {

  if (irb_recv.decode(&irb_results)) {
    irb_recv.resume(); // Receive the next value

    char * irstr;
    String value = "";

    if (irb_results.rawlen>1) {
      for(int i = 1; i < irb_results.rawlen; i++) {
        value = value + String(irb_results.rawbuf[i] * RAWTICK);

        if (i<irb_results.rawlen-1)
          value = value + ",";
      }
    } else {
      value = String(irb_results.rawbuf[1]);
    }

    if(value.length()!=0){
          irstr = const_cast<char*>(value.c_str());
    }

    DEBUG_MSG_P(PSTR("[IR Bridge] Raw IR input  %d values: %s\n"), irb_results.rawlen-1, irstr);

    #if MQTT_SUPPORT
        mqttSend(IRB_RAW_IN, irstr);
    #endif
  }
}

#endif //IRB_RX_PIN

#if (defined IRB_RX_PIN) || (defined IRB_TX_PIN)

void irbSetup() {

#if (defined IRB_RX_PIN)
    DEBUG_MSG_P("[IR Bridge] receiver INIT \n");
    irb_recv.enableIRIn();

    // Register RX loop
    espurnaRegisterLoop(irbLoop);

#endif

#if (defined IRB_TX_PIN)
    DEBUG_MSG_P("[IR Bridge] transmitter INIT \n");
    irb_send.begin();


    #if MQTT_SUPPORT
        DEBUG_MSG_P("[IR Bridge] MQTT INIT \n");
        mqttRegister(_irbMqttCallback);
    #endif

    // Register TX repeat loop
    espurnaRegisterLoop(irbRepeatLoop);

#endif

}

#endif // (defined IRB_RX_PIN) || (defined IRB_TX_PIN)
