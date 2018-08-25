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
 Repeat codes is optional. You may omit ":" and codes. In this case if repeat count > 0 we repeat main code.

Receiving:
 defined IRB_RAW_IN is a MQTT topic for IR incoming messages (outgoing to MQTT).
 Topic: <root>/IRB_RAW_IN
 Payload: 1000,1000,1000,1000,1000
          |        IR codes      |
 -----------------------------------------------------------------------------

 For support long codes (Air Conditioneer) increase MQTT packet size -DMQTT_MAX_PACKET_SIZE=1200
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
  uint8_t irb_repeat=0; // count of times repeating of repeat_code
  uint16_t irb_delay; // delay between repeat codes
  uint16_t irb_freq=38; // IR modulation freq. for sending codes and repeat codes
  uint8_t irb_repeat_size=0; // size of repeat array
  uint16_t *RawAry; // array for sending codes and repeat codes
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
          unsigned char count = 1; // count of code values for allocating array
          int col = strpayload.indexOf(":"); // position of ":" which means repeat_code
          uint16_t  repeat_Raw[1];

          if (col>2) { //count & validating repeat code

            irb_repeat_size = 1;

            // count & validate repeat-string
            for(int i = col+1; i < len; i++) {
              if (i<len-1) {
                if ( payload[i] == ',' && isDigit(payload[i+1]) && i>0 ) { //validate string
                  irb_repeat_size++;
                } else if (!isDigit(payload[i])) {
                  DEBUG_MSG("[IR Bridge] MQTT payload format error. Error in repeat_code. Use comma separated unsigned\n integer values. Last three is repeat delay, repeat count(<120) and frequency.\nAfter all you may write ':' and specify repeat code followed by comma.\n");
                  return;
                }
              }
            }
            len = col; //cut repeat code from main code processing
          } // end of counting & validating repeat code


          // count & validate main code string
          for(int i = 0; i < len; i++) {
            if (i<len-1) {
              if ( payload[i] == ',' && isDigit(payload[i+1]) && i>0 ) { //validate string
                count++;
              } else if (!isDigit(payload[i])) {
                DEBUG_MSG("[IR Bridge] MQTT payload format error. Error in main code. Use comma separated unsigned\n integer values. Last three is repeat delay, repeat count(<120) and frequency.\nAfter all you may write ':' and specify repeat code followed by comma.\n");
                return;
              }
            }
          }

          RawAry = (uint16_t*)calloc(count, sizeof(uint16_t)); // allocating array for main codes

          String value = ""; // for populating values of array from comma separated string
          int j = 0; // for populating values of array from comma separated string

          // populating main code array from part of MQTT string
          for (int i = 0; i < len; i++) {
           if (payload[i] != ',') {
              value = value + strpayload[i];
            }
            if ((payload[i] == ',') || (i == len - 1)) {
              RawAry[j]= value.toInt();
              value = "";
              j++;
            }
          }

          // if count>3 then we have values, repeat delay, count and modulation frequency
          irb_repeat=0;
          if (count>3) {
            if (RawAry[count-2]<=120) { // if repeat count > 120 it's to long and ussualy unusual. maybe we get raw code without this parameters and just use defaults for freq.
              irb_freq = RawAry[count-1];
              irb_repeat = RawAry[count-2];
              irb_delay = RawAry[count-3];
              count = count - 3;
            }
          }

          DEBUG_MSG("[IR Bridge] Raw IR output %d values, repeat %d times on %d(k)Hz freq.\n", count, irb_repeat, irb_freq);

          /*DEBUG_MSG("[IR Bridge] main codes: ");
          for(int i = 0; i < count; i++) {
            DEBUG_MSG("%d,",RawAry[i]);
          }
          DEBUG_MSG("\n");*/

          irb_recv.disableIRIn();
          irb_send.sendRaw(RawAry, count, irb_freq);

          //tone(13,1000,1);
          //noTone(13);

          if (irb_repeat==0) { // no repeat, cleaning array, enabling receiver
            free(RawAry);
            irb_recv.enableIRIn();
          } else if (col>2) { // repeat with repeat_code
            free(RawAry);
            RawAry = (uint16_t*)calloc(irb_repeat_size, sizeof(uint16_t));

            String value = ""; // for populating values of array from comma separated string
            int j = 0; // for populating values of array from comma separated string
            len = strpayload.length(); //redifining length to full lenght

            // populating repeat code array from part of MQTT string
            for (int i = col+1; i < len; i++) {
               value = value + strpayload[i];
             if ((payload[i] == ',') || (i == len - 1)) {
               RawAry[j]= value.toInt();
               value = "";
               j++;
              }
            }

          } else { // if repeat code not specified (col<=2) repeat with current main code
            irb_repeat_size = count;
          }

        } // end of match topic
    } // end of MQTT message
} //end of function

void irbRepeatLoop() {
  static uint32_t repeat_millis = 0;
  static uint8_t repeat_i = 0;

  if (irb_repeat>0)
    if (millis()-repeat_millis > irb_delay) {

      irb_send.sendRaw(RawAry, irb_repeat_size, irb_freq);

      /*DEBUG_MSG("[IR Bridge] %d repeat codes: ",irb_repeat_size);
      String str = "";
      for(int i = 0; i < irb_repeat_size; i++) {
        str=str+","+RawAry[i];
      }
      DEBUG_MSG("%s,\n",str.c_str());*/

      repeat_millis = millis();

      repeat_i++;
      if (repeat_i>=irb_repeat) { // end of repeat. cleaning...
        irb_repeat = 0;
        repeat_i = 0;
        free(RawAry);
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

    //DEBUG_MSG_P(PSTR("[IR Bridge] Raw IR input  %d values: %s\n"), irb_results.rawlen-1, irstr);
    DEBUG_MSG("[IR Bridge] Raw IR input  %d values: %s\n", irb_results.rawlen-1, value.c_str());

    #if MQTT_SUPPORT
        mqttSend(IRB_RAW_IN, irstr);
    #endif
  }
}

#endif //IRB_RX_PIN

#if (defined IRB_RX_PIN) || (defined IRB_TX_PIN)

void irbSetup() {

#if (defined IRB_RX_PIN)
    DEBUG_MSG("[IR Bridge] receiver INIT \n");
    irb_recv.enableIRIn();

    // Register RX loop
    espurnaRegisterLoop(irbLoop);

#endif

#if (defined IRB_TX_PIN)
    DEBUG_MSG("[IR Bridge] transmitter INIT \n");
    irb_send.begin();


    #if MQTT_SUPPORT
        DEBUG_MSG("[IR Bridge] MQTT INIT \n");
        mqttRegister(_irbMqttCallback);
    #endif

    // Register TX repeat loop
    espurnaRegisterLoop(irbRepeatLoop);

#endif

}

#endif // (defined IRB_RX_PIN) || (defined IRB_TX_PIN)
