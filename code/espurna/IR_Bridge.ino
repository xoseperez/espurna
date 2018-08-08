/*
IR-MQTT Raw Bridge MODULE
Copyright (C) 2018 by Alexander Kolesnikov

Tested on ESP8266, Sonoff RFBridge
Tested with Espurna

MQTT input topic: {root}/IRB_RAW_IN
MQTT output topic: {root}/IRB_RAW/set

*/

#if IRB_SUPPORT
// -----------------------------------------------------------------------------
// IR Bridge
// IRB_RX_PIN - receiver pin
// IRB_TX_PIN - transmitter pin
// -----------------------------------------------------------------------------

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>

uint16_t CAPTURE_BUFFER_SIZE = 1024;
#define IR_TIMEOUT 15U

IRrecv irb_recv(IRB_RX_PIN, CAPTURE_BUFFER_SIZE, IR_TIMEOUT, true);

IRsend irb_send(IRB_TX_PIN);

decode_results irb_results;

// MQTT to IR
#if MQTT_SUPPORT
void _irbMqttCallback(unsigned int type, const char * topic, const char * payload) {

    if (type == MQTT_CONNECT_EVENT) {
        mqttSubscribe("IRB_RAW");
    }

    if (type == MQTT_MESSAGE_EVENT) {

        // Match topic
        String t = mqttMagnitude((char *) topic);
        if (t.equals("IRB_RAW")) {

          String strpayload = String(payload);
          unsigned int len = strpayload.length();
          unsigned int count = 1;

          for(int i = 0; i < len; i++)
          {
            if (i<len-1) {
              if ( payload[i] == ',' && isDigit(payload[i+1]) && i>0 ) { //validate string
                count++;
              } else if (!isDigit(payload[i])) {
                DEBUG_MSG_P(PSTR("[IR Bridge] MQTT payload format error. Use comma separated unsigned integer values.\n"));
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

          char * irstr;
          if (strpayload.length()!=0){
            irstr = const_cast<char*>(strpayload.c_str());
          }

          DEBUG_MSG_P(PSTR("[IR Bridge] Raw IR output %d values: %s\n"), count, irstr);
          irb_send.sendRaw(Raw, count, 38);

        }
    }
}
#endif

// IR to MQTT
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

void irbSetup() {

    DEBUG_MSG_P("[IR Bridge] receiver INIT \n");
    irb_recv.enableIRIn();

    DEBUG_MSG_P("[IR Bridge] transmitter INIT \n");
    irb_send.begin();

    // Register loop
    espurnaRegisterLoop(irbLoop);

    #if MQTT_SUPPORT
        DEBUG_MSG_P("[IR Bridge] MQTT INIT \n");
        mqttRegister(_irbMqttCallback);
    #endif
}

#endif
