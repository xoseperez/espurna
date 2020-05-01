// -----------------------------------------------------------------------------
// DEFAULTS PER MODULE
// -----------------------------------------------------------------------------

#pragma once

// -----------------------------------------------------------------------------
// Buttons
// -----------------------------------------------------------------------------

#ifndef BUTTON1_PIN
#define BUTTON1_PIN         GPIO_NONE
#endif
#ifndef BUTTON2_PIN
#define BUTTON2_PIN         GPIO_NONE
#endif
#ifndef BUTTON3_PIN
#define BUTTON3_PIN         GPIO_NONE
#endif
#ifndef BUTTON4_PIN
#define BUTTON4_PIN         GPIO_NONE
#endif
#ifndef BUTTON5_PIN
#define BUTTON5_PIN         GPIO_NONE
#endif
#ifndef BUTTON6_PIN
#define BUTTON6_PIN         GPIO_NONE
#endif
#ifndef BUTTON7_PIN
#define BUTTON7_PIN         GPIO_NONE
#endif
#ifndef BUTTON8_PIN
#define BUTTON8_PIN         GPIO_NONE
#endif

#ifndef BUTTON1_CONFIG
#define BUTTON1_CONFIG         BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
#endif
#ifndef BUTTON2_CONFIG
#define BUTTON2_CONFIG         BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
#endif
#ifndef BUTTON3_CONFIG
#define BUTTON3_CONFIG         BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
#endif
#ifndef BUTTON4_CONFIG
#define BUTTON4_CONFIG         BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
#endif
#ifndef BUTTON5_CONFIG
#define BUTTON5_CONFIG         BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
#endif
#ifndef BUTTON6_CONFIG
#define BUTTON6_CONFIG         BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
#endif
#ifndef BUTTON7_CONFIG
#define BUTTON7_CONFIG         BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
#endif
#ifndef BUTTON8_CONFIG
#define BUTTON8_CONFIG         BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
#endif

#ifndef BUTTON1_PRESS
#define BUTTON1_PRESS       BUTTON_ACTION_NONE
#endif
#ifndef BUTTON2_PRESS
#define BUTTON2_PRESS       BUTTON_ACTION_NONE
#endif
#ifndef BUTTON3_PRESS
#define BUTTON3_PRESS       BUTTON_ACTION_NONE
#endif
#ifndef BUTTON4_PRESS
#define BUTTON4_PRESS       BUTTON_ACTION_NONE
#endif
#ifndef BUTTON5_PRESS
#define BUTTON5_PRESS       BUTTON_ACTION_NONE
#endif
#ifndef BUTTON6_PRESS
#define BUTTON6_PRESS       BUTTON_ACTION_NONE
#endif
#ifndef BUTTON7_PRESS
#define BUTTON7_PRESS       BUTTON_ACTION_NONE
#endif
#ifndef BUTTON8_PRESS
#define BUTTON8_PRESS       BUTTON_ACTION_NONE
#endif

#ifndef BUTTON1_CLICK
#define BUTTON1_CLICK       BUTTON_ACTION_TOGGLE
#endif
#ifndef BUTTON2_CLICK
#define BUTTON2_CLICK       BUTTON_ACTION_TOGGLE
#endif
#ifndef BUTTON3_CLICK
#define BUTTON3_CLICK       BUTTON_ACTION_TOGGLE
#endif
#ifndef BUTTON4_CLICK
#define BUTTON4_CLICK       BUTTON_ACTION_TOGGLE
#endif
#ifndef BUTTON5_CLICK
#define BUTTON5_CLICK       BUTTON_ACTION_TOGGLE
#endif
#ifndef BUTTON6_CLICK
#define BUTTON6_CLICK       BUTTON_ACTION_TOGGLE
#endif
#ifndef BUTTON7_CLICK
#define BUTTON7_CLICK       BUTTON_ACTION_TOGGLE
#endif
#ifndef BUTTON8_CLICK
#define BUTTON8_CLICK       BUTTON_ACTION_TOGGLE
#endif

#ifndef BUTTON1_DBLCLICK
#define BUTTON1_DBLCLICK    BUTTON_ACTION_AP
#endif
#ifndef BUTTON2_DBLCLICK
#define BUTTON2_DBLCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON3_DBLCLICK
#define BUTTON3_DBLCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON4_DBLCLICK
#define BUTTON4_DBLCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON5_DBLCLICK
#define BUTTON5_DBLCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON6_DBLCLICK
#define BUTTON6_DBLCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON7_DBLCLICK
#define BUTTON7_DBLCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON8_DBLCLICK
#define BUTTON8_DBLCLICK    BUTTON_ACTION_NONE
#endif

#ifndef BUTTON1_TRIPLECLICK
#define BUTTON1_TRIPLECLICK    BUTTON_ACTION_SMART_CONFIG
#endif
#ifndef BUTTON2_TRIPLECLICK
#define BUTTON2_TRIPLECLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON3_TRIPLECLICK
#define BUTTON3_TRIPLECLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON4_TRIPLECLICK
#define BUTTON4_TRIPLECLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON5_TRIPLECLICK
#define BUTTON5_TRIPLECLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON6_TRIPLECLICK
#define BUTTON6_TRIPLECLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON7_TRIPLECLICK
#define BUTTON7_TRIPLECLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON8_TRIPLECLICK
#define BUTTON8_TRIPLECLICK    BUTTON_ACTION_NONE
#endif

#ifndef BUTTON1_LNGCLICK
#define BUTTON1_LNGCLICK    BUTTON_ACTION_RESET
#endif
#ifndef BUTTON2_LNGCLICK
#define BUTTON2_LNGCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON3_LNGCLICK
#define BUTTON3_LNGCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON4_LNGCLICK
#define BUTTON4_LNGCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON5_LNGCLICK
#define BUTTON5_LNGCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON6_LNGCLICK
#define BUTTON6_LNGCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON7_LNGCLICK
#define BUTTON7_LNGCLICK    BUTTON_ACTION_NONE
#endif
#ifndef BUTTON8_LNGCLICK
#define BUTTON8_LNGCLICK    BUTTON_ACTION_NONE
#endif

#ifndef BUTTON1_LNGLNGCLICK
#define BUTTON1_LNGLNGCLICK BUTTON_ACTION_FACTORY
#endif
#ifndef BUTTON2_LNGLNGCLICK
#define BUTTON2_LNGLNGCLICK BUTTON_ACTION_NONE
#endif
#ifndef BUTTON3_LNGLNGCLICK
#define BUTTON3_LNGLNGCLICK BUTTON_ACTION_NONE
#endif
#ifndef BUTTON4_LNGLNGCLICK
#define BUTTON4_LNGLNGCLICK BUTTON_ACTION_NONE
#endif
#ifndef BUTTON5_LNGLNGCLICK
#define BUTTON5_LNGLNGCLICK BUTTON_ACTION_NONE
#endif
#ifndef BUTTON6_LNGLNGCLICK
#define BUTTON6_LNGLNGCLICK BUTTON_ACTION_NONE
#endif
#ifndef BUTTON7_LNGLNGCLICK
#define BUTTON7_LNGLNGCLICK BUTTON_ACTION_NONE
#endif
#ifndef BUTTON8_LNGLNGCLICK
#define BUTTON8_LNGLNGCLICK BUTTON_ACTION_NONE
#endif

#ifndef BUTTON1_RELAY
#define BUTTON1_RELAY       RELAY_NONE
#endif
#ifndef BUTTON2_RELAY
#define BUTTON2_RELAY       RELAY_NONE
#endif
#ifndef BUTTON3_RELAY
#define BUTTON3_RELAY       RELAY_NONE
#endif
#ifndef BUTTON4_RELAY
#define BUTTON4_RELAY       RELAY_NONE
#endif
#ifndef BUTTON5_RELAY
#define BUTTON5_RELAY       RELAY_NONE
#endif
#ifndef BUTTON6_RELAY
#define BUTTON6_RELAY       RELAY_NONE
#endif
#ifndef BUTTON7_RELAY
#define BUTTON7_RELAY       RELAY_NONE
#endif
#ifndef BUTTON8_RELAY
#define BUTTON8_RELAY       RELAY_NONE
#endif

#ifndef BUTTON1_DEBOUNCE_DELAY
#define BUTTON1_DEBOUNCE_DELAY    BUTTON_DEBOUNCE_DELAY
#endif
#ifndef BUTTON2_DEBOUNCE_DELAY
#define BUTTON2_DEBOUNCE_DELAY    BUTTON_DEBOUNCE_DELAY
#endif
#ifndef BUTTON3_DEBOUNCE_DELAY
#define BUTTON3_DEBOUNCE_DELAY    BUTTON_DEBOUNCE_DELAY
#endif
#ifndef BUTTON4_DEBOUNCE_DELAY
#define BUTTON4_DEBOUNCE_DELAY    BUTTON_DEBOUNCE_DELAY
#endif
#ifndef BUTTON5_DEBOUNCE_DELAY
#define BUTTON5_DEBOUNCE_DELAY    BUTTON_DEBOUNCE_DELAY
#endif
#ifndef BUTTON6_DEBOUNCE_DELAY
#define BUTTON6_DEBOUNCE_DELAY    BUTTON_DEBOUNCE_DELAY
#endif
#ifndef BUTTON7_DEBOUNCE_DELAY
#define BUTTON7_DEBOUNCE_DELAY    BUTTON_DEBOUNCE_DELAY
#endif
#ifndef BUTTON8_DEBOUNCE_DELAY
#define BUTTON8_DEBOUNCE_DELAY    BUTTON_DEBOUNCE_DELAY
#endif

#ifndef BUTTON1_REPEAT_DELAY
#define BUTTON1_REPEAT_DELAY    BUTTON_REPEAT_DELAY
#endif
#ifndef BUTTON2_REPEAT_DELAY
#define BUTTON2_REPEAT_DELAY    BUTTON_REPEAT_DELAY
#endif
#ifndef BUTTON3_REPEAT_DELAY
#define BUTTON3_REPEAT_DELAY    BUTTON_REPEAT_DELAY
#endif
#ifndef BUTTON4_REPEAT_DELAY
#define BUTTON4_REPEAT_DELAY    BUTTON_REPEAT_DELAY
#endif
#ifndef BUTTON5_REPEAT_DELAY
#define BUTTON5_REPEAT_DELAY    BUTTON_REPEAT_DELAY
#endif
#ifndef BUTTON6_REPEAT_DELAY
#define BUTTON6_REPEAT_DELAY    BUTTON_REPEAT_DELAY
#endif
#ifndef BUTTON7_REPEAT_DELAY
#define BUTTON7_REPEAT_DELAY    BUTTON_REPEAT_DELAY
#endif
#ifndef BUTTON8_REPEAT_DELAY
#define BUTTON8_REPEAT_DELAY    BUTTON_REPEAT_DELAY
#endif

#ifndef BUTTON1_LNGCLICK_DELAY
#define BUTTON1_LNGCLICK_DELAY    BUTTON_LNGCLICK_DELAY
#endif
#ifndef BUTTON2_LNGCLICK_DELAY
#define BUTTON2_LNGCLICK_DELAY    BUTTON_LNGCLICK_DELAY
#endif
#ifndef BUTTON3_LNGCLICK_DELAY
#define BUTTON3_LNGCLICK_DELAY    BUTTON_LNGCLICK_DELAY
#endif
#ifndef BUTTON4_LNGCLICK_DELAY
#define BUTTON4_LNGCLICK_DELAY    BUTTON_LNGCLICK_DELAY
#endif
#ifndef BUTTON5_LNGCLICK_DELAY
#define BUTTON5_LNGCLICK_DELAY    BUTTON_LNGCLICK_DELAY
#endif
#ifndef BUTTON6_LNGCLICK_DELAY
#define BUTTON6_LNGCLICK_DELAY    BUTTON_LNGCLICK_DELAY
#endif
#ifndef BUTTON7_LNGCLICK_DELAY
#define BUTTON7_LNGCLICK_DELAY    BUTTON_LNGCLICK_DELAY
#endif
#ifndef BUTTON8_LNGCLICK_DELAY
#define BUTTON8_LNGCLICK_DELAY    BUTTON_LNGCLICK_DELAY
#endif

#ifndef BUTTON1_LNGLNGCLICK_DELAY
#define BUTTON1_LNGLNGCLICK_DELAY    BUTTON_LNGLNGCLICK_DELAY
#endif
#ifndef BUTTON2_LNGLNGCLICK_DELAY
#define BUTTON2_LNGLNGCLICK_DELAY    BUTTON_LNGLNGCLICK_DELAY
#endif
#ifndef BUTTON3_LNGLNGCLICK_DELAY
#define BUTTON3_LNGLNGCLICK_DELAY    BUTTON_LNGLNGCLICK_DELAY
#endif
#ifndef BUTTON4_LNGLNGCLICK_DELAY
#define BUTTON4_LNGLNGCLICK_DELAY    BUTTON_LNGLNGCLICK_DELAY
#endif
#ifndef BUTTON5_LNGLNGCLICK_DELAY
#define BUTTON5_LNGLNGCLICK_DELAY    BUTTON_LNGLNGCLICK_DELAY
#endif
#ifndef BUTTON6_LNGLNGCLICK_DELAY
#define BUTTON6_LNGLNGCLICK_DELAY    BUTTON_LNGLNGCLICK_DELAY
#endif
#ifndef BUTTON7_LNGLNGCLICK_DELAY
#define BUTTON7_LNGLNGCLICK_DELAY    BUTTON_LNGLNGCLICK_DELAY
#endif
#ifndef BUTTON8_LNGLNGCLICK_DELAY
#define BUTTON8_LNGLNGCLICK_DELAY    BUTTON_LNGLNGCLICK_DELAY
#endif

#ifndef BUTTON1_MQTT_SEND_ALL_EVENTS
#define BUTTON1_MQTT_SEND_ALL_EVENTS    BUTTON_MQTT_SEND_ALL_EVENTS
#endif

#ifndef BUTTON2_MQTT_SEND_ALL_EVENTS
#define BUTTON2_MQTT_SEND_ALL_EVENTS    BUTTON_MQTT_SEND_ALL_EVENTS
#endif

#ifndef BUTTON3_MQTT_SEND_ALL_EVENTS
#define BUTTON3_MQTT_SEND_ALL_EVENTS    BUTTON_MQTT_SEND_ALL_EVENTS
#endif

#ifndef BUTTON4_MQTT_SEND_ALL_EVENTS
#define BUTTON4_MQTT_SEND_ALL_EVENTS    BUTTON_MQTT_SEND_ALL_EVENTS
#endif

#ifndef BUTTON5_MQTT_SEND_ALL_EVENTS
#define BUTTON5_MQTT_SEND_ALL_EVENTS    BUTTON_MQTT_SEND_ALL_EVENTS
#endif

#ifndef BUTTON6_MQTT_SEND_ALL_EVENTS
#define BUTTON6_MQTT_SEND_ALL_EVENTS    BUTTON_MQTT_SEND_ALL_EVENTS
#endif

#ifndef BUTTON7_MQTT_SEND_ALL_EVENTS
#define BUTTON7_MQTT_SEND_ALL_EVENTS    BUTTON_MQTT_SEND_ALL_EVENTS
#endif

#ifndef BUTTON8_MQTT_SEND_ALL_EVENTS
#define BUTTON8_MQTT_SEND_ALL_EVENTS    BUTTON_MQTT_SEND_ALL_EVENTS
#endif

#ifndef BUTTON1_MQTT_RETAIN
#define BUTTON1_MQTT_RETAIN    BUTTON_MQTT_RETAIN
#endif

#ifndef BUTTON2_MQTT_RETAIN
#define BUTTON2_MQTT_RETAIN    BUTTON_MQTT_RETAIN
#endif

#ifndef BUTTON3_MQTT_RETAIN
#define BUTTON3_MQTT_RETAIN    BUTTON_MQTT_RETAIN
#endif

#ifndef BUTTON4_MQTT_RETAIN
#define BUTTON4_MQTT_RETAIN    BUTTON_MQTT_RETAIN
#endif

#ifndef BUTTON5_MQTT_RETAIN
#define BUTTON5_MQTT_RETAIN    BUTTON_MQTT_RETAIN
#endif

#ifndef BUTTON6_MQTT_RETAIN
#define BUTTON6_MQTT_RETAIN    BUTTON_MQTT_RETAIN
#endif

#ifndef BUTTON7_MQTT_RETAIN
#define BUTTON7_MQTT_RETAIN    BUTTON_MQTT_RETAIN
#endif

#ifndef BUTTON8_MQTT_RETAIN
#define BUTTON8_MQTT_RETAIN    BUTTON_MQTT_RETAIN
#endif

// -----------------------------------------------------------------------------
// Encoders
// -----------------------------------------------------------------------------

#ifndef ENCODER1_PIN1
#define ENCODER1_PIN1               GPIO_NONE
#endif
#ifndef ENCODER2_PIN1
#define ENCODER2_PIN1               GPIO_NONE
#endif
#ifndef ENCODER3_PIN1
#define ENCODER3_PIN1               GPIO_NONE
#endif
#ifndef ENCODER4_PIN1
#define ENCODER4_PIN1               GPIO_NONE
#endif
#ifndef ENCODER5_PIN1
#define ENCODER5_PIN1               GPIO_NONE
#endif

#ifndef ENCODER1_PIN2
#define ENCODER1_PIN2               GPIO_NONE
#endif
#ifndef ENCODER2_PIN2
#define ENCODER2_PIN2               GPIO_NONE
#endif
#ifndef ENCODER3_PIN2
#define ENCODER3_PIN2               GPIO_NONE
#endif
#ifndef ENCODER4_PIN2
#define ENCODER4_PIN2               GPIO_NONE
#endif
#ifndef ENCODER5_PIN2
#define ENCODER5_PIN2               GPIO_NONE
#endif

#ifndef ENCODER1_BUTTON_PIN
#define ENCODER1_BUTTON_PIN         GPIO_NONE
#endif
#ifndef ENCODER2_BUTTON_PIN
#define ENCODER2_BUTTON_PIN         GPIO_NONE
#endif
#ifndef ENCODER3_BUTTON_PIN
#define ENCODER3_BUTTON_PIN         GPIO_NONE
#endif
#ifndef ENCODER4_BUTTON_PIN
#define ENCODER4_BUTTON_PIN         GPIO_NONE
#endif
#ifndef ENCODER5_BUTTON_PIN
#define ENCODER5_BUTTON_PIN         GPIO_NONE
#endif

#ifndef ENCODER1_BUTTON_LOGIC
#define ENCODER1_BUTTON_LOGIC       HIGH
#endif
#ifndef ENCODER2_BUTTON_LOGIC
#define ENCODER2_BUTTON_LOGIC       HIGH
#endif
#ifndef ENCODER3_BUTTON_LOGIC
#define ENCODER3_BUTTON_LOGIC       HIGH
#endif
#ifndef ENCODER4_BUTTON_LOGIC
#define ENCODER4_BUTTON_LOGIC       HIGH
#endif
#ifndef ENCODER5_BUTTON_LOGIC
#define ENCODER5_BUTTON_LOGIC       HIGH
#endif

#ifndef ENCODER1_BUTTON_MODE
#define ENCODER1_BUTTON_MODE        INPUT_PULLUP
#endif
#ifndef ENCODER2_BUTTON_MODE
#define ENCODER2_BUTTON_MODE        INPUT_PULLUP
#endif
#ifndef ENCODER3_BUTTON_MODE
#define ENCODER3_BUTTON_MODE        INPUT_PULLUP
#endif
#ifndef ENCODER4_BUTTON_MODE
#define ENCODER4_BUTTON_MODE        INPUT_PULLUP
#endif
#ifndef ENCODER5_BUTTON_MODE
#define ENCODER5_BUTTON_MODE        INPUT_PULLUP
#endif

#ifndef ENCODER1_MODE
#define ENCODER1_MODE               1
#endif
#ifndef ENCODER2_MODE
#define ENCODER2_MODE               1
#endif
#ifndef ENCODER3_MODE
#define ENCODER3_MODE               1
#endif
#ifndef ENCODER4_MODE
#define ENCODER4_MODE               1
#endif
#ifndef ENCODER5_MODE
#define ENCODER5_MODE               1
#endif

#ifndef ENCODER1_CHANNEL1
#define ENCODER1_CHANNEL1           0
#endif
#ifndef ENCODER2_CHANNEL1
#define ENCODER2_CHANNEL1           0
#endif
#ifndef ENCODER3_CHANNEL1
#define ENCODER3_CHANNEL1           0
#endif
#ifndef ENCODER4_CHANNEL1
#define ENCODER4_CHANNEL1           0
#endif
#ifndef ENCODER5_CHANNEL1
#define ENCODER5_CHANNEL1           0
#endif

#ifndef ENCODER1_CHANNEL2
#define ENCODER1_CHANNEL2           1
#endif
#ifndef ENCODER2_CHANNEL2
#define ENCODER2_CHANNEL2           1
#endif
#ifndef ENCODER3_CHANNEL2
#define ENCODER3_CHANNEL2           1
#endif
#ifndef ENCODER4_CHANNEL2
#define ENCODER4_CHANNEL2           1
#endif
#ifndef ENCODER5_CHANNEL2
#define ENCODER5_CHANNEL2           1
#endif

// -----------------------------------------------------------------------------
// Relays
// -----------------------------------------------------------------------------

#ifndef DUMMY_RELAY_COUNT
#define DUMMY_RELAY_COUNT     0
#endif

#ifndef RELAY1_PIN
#define RELAY1_PIN            GPIO_NONE
#endif
#ifndef RELAY2_PIN
#define RELAY2_PIN            GPIO_NONE
#endif
#ifndef RELAY3_PIN
#define RELAY3_PIN            GPIO_NONE
#endif
#ifndef RELAY4_PIN
#define RELAY4_PIN            GPIO_NONE
#endif
#ifndef RELAY5_PIN
#define RELAY5_PIN            GPIO_NONE
#endif
#ifndef RELAY6_PIN
#define RELAY6_PIN            GPIO_NONE
#endif
#ifndef RELAY7_PIN
#define RELAY7_PIN            GPIO_NONE
#endif
#ifndef RELAY8_PIN
#define RELAY8_PIN            GPIO_NONE
#endif

#ifndef RELAY1_TYPE
#define RELAY1_TYPE           RELAY_TYPE_NORMAL
#endif
#ifndef RELAY2_TYPE
#define RELAY2_TYPE           RELAY_TYPE_NORMAL
#endif
#ifndef RELAY3_TYPE
#define RELAY3_TYPE           RELAY_TYPE_NORMAL
#endif
#ifndef RELAY4_TYPE
#define RELAY4_TYPE           RELAY_TYPE_NORMAL
#endif
#ifndef RELAY5_TYPE
#define RELAY5_TYPE           RELAY_TYPE_NORMAL
#endif
#ifndef RELAY6_TYPE
#define RELAY6_TYPE           RELAY_TYPE_NORMAL
#endif
#ifndef RELAY7_TYPE
#define RELAY7_TYPE           RELAY_TYPE_NORMAL
#endif
#ifndef RELAY8_TYPE
#define RELAY8_TYPE           RELAY_TYPE_NORMAL
#endif

#ifndef RELAY1_RESET_PIN
#define RELAY1_RESET_PIN      GPIO_NONE
#endif
#ifndef RELAY2_RESET_PIN
#define RELAY2_RESET_PIN      GPIO_NONE
#endif
#ifndef RELAY3_RESET_PIN
#define RELAY3_RESET_PIN      GPIO_NONE
#endif
#ifndef RELAY4_RESET_PIN
#define RELAY4_RESET_PIN      GPIO_NONE
#endif
#ifndef RELAY5_RESET_PIN
#define RELAY5_RESET_PIN      GPIO_NONE
#endif
#ifndef RELAY6_RESET_PIN
#define RELAY6_RESET_PIN      GPIO_NONE
#endif
#ifndef RELAY7_RESET_PIN
#define RELAY7_RESET_PIN      GPIO_NONE
#endif
#ifndef RELAY8_RESET_PIN
#define RELAY8_RESET_PIN      GPIO_NONE
#endif

#ifndef RELAY1_DELAY_ON
#define RELAY1_DELAY_ON       0
#endif
#ifndef RELAY2_DELAY_ON
#define RELAY2_DELAY_ON       0
#endif
#ifndef RELAY3_DELAY_ON
#define RELAY3_DELAY_ON       0
#endif
#ifndef RELAY4_DELAY_ON
#define RELAY4_DELAY_ON       0
#endif
#ifndef RELAY5_DELAY_ON
#define RELAY5_DELAY_ON       0
#endif
#ifndef RELAY6_DELAY_ON
#define RELAY6_DELAY_ON       0
#endif
#ifndef RELAY7_DELAY_ON
#define RELAY7_DELAY_ON       0
#endif
#ifndef RELAY8_DELAY_ON
#define RELAY8_DELAY_ON       0
#endif

#ifndef RELAY1_DELAY_OFF
#define RELAY1_DELAY_OFF      0
#endif
#ifndef RELAY2_DELAY_OFF
#define RELAY2_DELAY_OFF      0
#endif
#ifndef RELAY3_DELAY_OFF
#define RELAY3_DELAY_OFF      0
#endif
#ifndef RELAY4_DELAY_OFF
#define RELAY4_DELAY_OFF      0
#endif
#ifndef RELAY5_DELAY_OFF
#define RELAY5_DELAY_OFF      0
#endif
#ifndef RELAY6_DELAY_OFF
#define RELAY6_DELAY_OFF      0
#endif
#ifndef RELAY7_DELAY_OFF
#define RELAY7_DELAY_OFF      0
#endif
#ifndef RELAY8_DELAY_OFF
#define RELAY8_DELAY_OFF      0
#endif

#ifndef RELAY_DELAY_INTERLOCK
#define RELAY_DELAY_INTERLOCK       0
#endif

// -----------------------------------------------------------------------------
// LEDs
// -----------------------------------------------------------------------------

#ifndef LED1_PIN
#define LED1_PIN            GPIO_NONE
#endif
#ifndef LED2_PIN
#define LED2_PIN            GPIO_NONE
#endif
#ifndef LED3_PIN
#define LED3_PIN            GPIO_NONE
#endif
#ifndef LED4_PIN
#define LED4_PIN            GPIO_NONE
#endif
#ifndef LED5_PIN
#define LED5_PIN            GPIO_NONE
#endif
#ifndef LED6_PIN
#define LED6_PIN            GPIO_NONE
#endif
#ifndef LED7_PIN
#define LED7_PIN            GPIO_NONE
#endif
#ifndef LED8_PIN
#define LED8_PIN            GPIO_NONE
#endif

#ifndef LED1_MODE
#define LED1_MODE           LED_MODE_WIFI
#endif
#ifndef LED2_MODE
#define LED2_MODE           LED_MODE_MANUAL
#endif
#ifndef LED3_MODE
#define LED3_MODE           LED_MODE_MANUAL
#endif
#ifndef LED4_MODE
#define LED4_MODE           LED_MODE_MANUAL
#endif
#ifndef LED5_MODE
#define LED5_MODE           LED_MODE_MANUAL
#endif
#ifndef LED6_MODE
#define LED6_MODE           LED_MODE_MANUAL
#endif
#ifndef LED7_MODE
#define LED7_MODE           LED_MODE_MANUAL
#endif
#ifndef LED8_MODE
#define LED8_MODE           LED_MODE_MANUAL
#endif

#ifndef LED1_RELAY
#define LED1_RELAY          1
#endif
#ifndef LED2_RELAY
#define LED2_RELAY          2
#endif
#ifndef LED3_RELAY
#define LED3_RELAY          3
#endif
#ifndef LED4_RELAY
#define LED4_RELAY          4
#endif
#ifndef LED5_RELAY
#define LED5_RELAY          5
#endif
#ifndef LED6_RELAY
#define LED6_RELAY          6
#endif
#ifndef LED7_RELAY
#define LED7_RELAY          7
#endif
#ifndef LED8_RELAY
#define LED8_RELAY          8
#endif

#ifndef LED1_PIN_INVERSE
#define LED1_PIN_INVERSE            0
#endif
#ifndef LED2_PIN_INVERSE
#define LED2_PIN_INVERSE            0
#endif
#ifndef LED3_PIN_INVERSE
#define LED3_PIN_INVERSE            0
#endif
#ifndef LED4_PIN_INVERSE
#define LED4_PIN_INVERSE            0
#endif
#ifndef LED5_PIN_INVERSE
#define LED5_PIN_INVERSE            0
#endif
#ifndef LED6_PIN_INVERSE
#define LED6_PIN_INVERSE            0
#endif
#ifndef LED7_PIN_INVERSE
#define LED7_PIN_INVERSE            0
#endif
#ifndef LED8_PIN_INVERSE
#define LED8_PIN_INVERSE            0
#endif

// -----------------------------------------------------------------------------
// Digital Inputs
// -----------------------------------------------------------------------------

#ifndef DIGITAL1_PIN
#define DIGITAL1_PIN         GPIO_NONE
#endif
#ifndef DIGITAL2_PIN
#define DIGITAL2_PIN         GPIO_NONE
#endif
#ifndef DIGITAL3_PIN
#define DIGITAL3_PIN         GPIO_NONE
#endif
#ifndef DIGITAL4_PIN
#define DIGITAL4_PIN         GPIO_NONE
#endif
#ifndef DIGITAL5_PIN
#define DIGITAL5_PIN         GPIO_NONE
#endif
#ifndef DIGITAL6_PIN
#define DIGITAL6_PIN         GPIO_NONE
#endif
#ifndef DIGITAL7_PIN
#define DIGITAL7_PIN         GPIO_NONE
#endif
#ifndef DIGITAL8_PIN
#define DIGITAL8_PIN         GPIO_NONE
#endif

// -----------------------------------------------------------------------------
// Events
// -----------------------------------------------------------------------------

#ifndef EVENTS1_PIN
#define EVENTS1_PIN         GPIO_NONE
#endif
#ifndef EVENTS2_PIN
#define EVENTS2_PIN         GPIO_NONE
#endif
#ifndef EVENTS3_PIN
#define EVENTS3_PIN         GPIO_NONE
#endif
#ifndef EVENTS4_PIN
#define EVENTS4_PIN         GPIO_NONE
#endif
#ifndef EVENTS5_PIN
#define EVENTS5_PIN         GPIO_NONE
#endif
#ifndef EVENTS6_PIN
#define EVENTS6_PIN         GPIO_NONE
#endif
#ifndef EVENTS7_PIN
#define EVENTS7_PIN         GPIO_NONE
#endif
#ifndef EVENTS8_PIN
#define EVENTS8_PIN         GPIO_NONE
#endif

// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

#ifndef LIGHT_CHANNELS
#define LIGHT_CHANNELS      0
#endif

#ifndef LIGHT_ENABLE_PIN
#define LIGHT_ENABLE_PIN    GPIO_NONE
#endif

#ifndef LIGHT_CH1_PIN
#define LIGHT_CH1_PIN       GPIO_NONE
#endif

#ifndef LIGHT_CH2_PIN
#define LIGHT_CH2_PIN       GPIO_NONE
#endif

#ifndef LIGHT_CH3_PIN
#define LIGHT_CH3_PIN       GPIO_NONE
#endif

#ifndef LIGHT_CH4_PIN
#define LIGHT_CH4_PIN       GPIO_NONE
#endif

#ifndef LIGHT_CH5_PIN
#define LIGHT_CH5_PIN       GPIO_NONE
#endif

#ifndef LIGHT_CH1_INVERSE
#define LIGHT_CH1_INVERSE   0
#endif

#ifndef LIGHT_CH2_INVERSE
#define LIGHT_CH2_INVERSE   0
#endif

#ifndef LIGHT_CH3_INVERSE
#define LIGHT_CH3_INVERSE   0
#endif

#ifndef LIGHT_CH4_INVERSE
#define LIGHT_CH4_INVERSE   0
#endif

#ifndef LIGHT_CH5_INVERSE
#define LIGHT_CH5_INVERSE   0
#endif

// -----------------------------------------------------------------------------
// General
// -----------------------------------------------------------------------------

// Device name (DNS, SoftAP SSID, ALEXA etc.)
// If empty, default will be ESPURNA-XXXXXX, where XXXXXX is last 3 octets of chipID
// When set, must be 1..31 characters. See:
// https://github.com/xoseperez/espurna/issues/921
// https://github.com/xoseperez/espurna/issues/1151
#ifndef HOSTNAME
#define HOSTNAME                ""
#endif

// Relay providers
#ifndef RELAY_PROVIDER
#define RELAY_PROVIDER          RELAY_PROVIDER_RELAY
#endif

// Light provider
#ifndef LIGHT_PROVIDER
#define LIGHT_PROVIDER          LIGHT_PROVIDER_NONE
#endif
