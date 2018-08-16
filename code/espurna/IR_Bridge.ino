/*
IR-MQTT Raw Bridge MODULE
Copyright (C) 2018 by Alexander Kolesnikov

Tested on ESP8266, Sonoff RFBridge
Tested with Espurna

MQTT input topic: {root}/IRB_RAW_IN
MQTT output topic: {root}/IRB_RAW/set

*/

// -----------------------------------------------------------------------------
// IR Bridge
// IRB_RX_PIN - receiver pin
// IRB_TX_PIN - transmitter pin
// -----------------------------------------------------------------------------
#if (defined IRB_RX_PIN) || (defined IRB_TX_PIN)
  #include <IRremoteESP8266.h>
#endif

#if (defined IRB_RX_PIN)
  #include <IRrecv.h>
  uint16_t CAPTURE_BUFFER_SIZE = 1024;
  #define IR_TIMEOUT 15U
  IRrecv irb_recv(IRB_RX_PIN, CAPTURE_BUFFER_SIZE, IR_TIMEOUT, true);
  decode_results irb_results;
#endif //IRB_RX_PIN

#if (defined IRB_TX_PIN)
  #include <IRsend.h>
  IRsend irb_send(IRB_TX_PIN);
  #define IRB_RAW "IRB_RAW"
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

          for(int i = 0; i < len; i++)
          {
            if (i<len-1) {
              if ( payload[i] == ',' && isDigit(payload[i+1]) && i>0 ) { //validate string
                count++;
              } else if (!isDigit(payload[i])) {
                DEBUG_MSG_P(PSTR("[IR Bridge] MQTT payload format error. Use comma separated unsigned integer values. Last two is repeat(<20) count and frequency.\n"));
                return;
              }

            }

          }

          uint16_t  Raw[count];

          String value = "";
          int j = 0;
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

          uint16_t freq=38;
          uint8_t repeat=1;
          // if count >2 then we have values and repeat number and frequency
          if (count>2) {
            if (Raw[count-2]<20) {
              freq = Raw[count-1];
              repeat = Raw[count-2];
              count = count - 2;
            }
          }

          char * irstr;
          if (strpayload.length()!=0){
            irstr = const_cast<char*>(strpayload.c_str());
          }

          DEBUG_MSG_P(PSTR("[IR Bridge] Raw IR output %d values %d times on %d(k)Hz frequency: %s\n"), count, repeat, freq, irstr);
          irb_recv.disableIRIn();
          for (int i=0; i < repeat; i++) {
            irb_send.sendRaw(Raw, count, freq);
          }
          irb_recv.enableIRIn();

        }
    }
}
#endif // IRB_TX_PIN & MQTT


// IR to MQTT
#if (defined IRB_RX_PIN)

void irbLoop() {

  if (irb_recv.decode(&irb_results)) {
    //unsigned long code = _ir_results.value;
    //_ir_send->sendRaw(_ir_results.rawbuf, _ir_results.rawlen, 38);
    irb_recv.resume(); // Receive the next value

    char * irstr;
    String value = "";

    if (irb_results.rawlen>1) {
      for(int i = 1; i < irb_results.rawlen; i++) {
        //if (i % 100 == 0) yield();  // Preemptive yield every 100th entry to feed the WDT.
        value = value + String(irb_results.rawbuf[i] * RAWTICK);
        //value = value + String(_ir_results.rawbuf[i]);

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
        mqttSend("IRB_RAW_IN", irstr);
    #endif
  }

}

#endif //IRB_RX_PIN

#if (defined IRB_RX_PIN) || (defined IRB_TX_PIN)

void irbSetup() {

#if (defined IRB_RX_PIN)
    DEBUG_MSG_P("[IR Bridge] receiver INIT \n");
    irb_recv.enableIRIn();

    #if MQTT_SUPPORT
        DEBUG_MSG_P("[IR Bridge] MQTT INIT \n");
        mqttRegister(_irbMqttCallback);
    #endif

#endif

#if (defined IRB_TX_PIN)
    DEBUG_MSG_P("[IR Bridge] transmitter INIT \n");
    irb_send.begin();
    // Register loop
    espurnaRegisterLoop(irbLoop);
#endif

}

#endif // (defined IRB_RX_PIN) || (defined IRB_TX_PIN)
