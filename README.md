# Espurna customization (dirty plugins)
## MQTT<-->IR Bridge

Just download IR_Bridge.ino and compile Espurna with flags -DIRB_RX_PIN=14 -DIRB_TX_PIN=12\

### Usage:

 For enable transmit functions define IRB_TX_PIN\
 For enable receiver functions define IRB_RX_PIN\

 Call irbSetup(); from bottom of setup() in espurna.ino or add:

```
// IR Bridge support
#if (defined IRB_RX_PIN) || (defined IRB_TX_PIN)
   irbSetup();
#endif
```

### Transmitting:
 defined IRB_RAW is a MQTT topic for MQTT incoming messages (outgoing by IR).\
 Topic: <root>/IRB_RAW/set\
 Payload: 1000,1000,1000,1000,1000,DELAY,COUNT,FREQ:500,500,500,500,500\

 codes - time in microseconds when IR LED On/Off. First value - ON, second - Off ..etc...\
 DELAY - delay in milliseconds between sending repeats\
 COUNT - how many repeats send. Max 120.\
 FREQ - modulation frequency. Usually 38kHz. You may set 38, it means 38kHz or set 38000, it meant same.\
 After all you may write ":" and set repeat-codes. Usually IR-remotes send one data code and then until we continue pressing key, they send short repeat codes.

### Receiving:
 defined IRB_RAW_IN is a MQTT topic for IR incoming messages (outgoing to MQTT).\
 Topic: <root>/IRB_RAW_IN\
 Payload: 1000,1000,1000,1000,1000\

## Blinking of LED managed by MQTT
Usefull for notifications from smart-home system.

### Usage:
Send MQTT payload "3" to interesting LED and it start blinking. Send "0" or "1" and it stop blinking.

## <dev> LCD/OLED display support
Print sensors values, switch status, etc..


# ESPurna Firmware

ESPurna ("spark" in Catalan) is a custom firmware for ESP8285/ESP8266 based smart switches, lights and sensors.\
It uses the Arduino Core for ESP8266 framework and a number of 3rd party libraries.

https://github.com/xoseperez/espurna