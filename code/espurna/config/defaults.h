// -----------------------------------------------------------------------------
// Hardware default values
// -----------------------------------------------------------------------------

#define GPIO_NONE           0x99

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

#ifndef BUTTON1_PRESS
#define BUTTON1_PRESS       BUTTON_MODE_NONE
#endif
#ifndef BUTTON2_PRESS
#define BUTTON2_PRESS       BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_PRESS
#define BUTTON3_PRESS       BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_PRESS
#define BUTTON4_PRESS       BUTTON_MODE_NONE
#endif
#ifndef BUTTON5_PRESS
#define BUTTON5_PRESS       BUTTON_MODE_NONE
#endif
#ifndef BUTTON6_PRESS
#define BUTTON6_PRESS       BUTTON_MODE_NONE
#endif
#ifndef BUTTON7_PRESS
#define BUTTON7_PRESS       BUTTON_MODE_NONE
#endif
#ifndef BUTTON8_PRESS
#define BUTTON8_PRESS       BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_CLICK
#define BUTTON1_CLICK       BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON2_CLICK
#define BUTTON2_CLICK       BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON3_CLICK
#define BUTTON3_CLICK       BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON4_CLICK
#define BUTTON4_CLICK       BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON5_CLICK
#define BUTTON5_CLICK       BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON6_CLICK
#define BUTTON6_CLICK       BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON7_CLICK
#define BUTTON7_CLICK       BUTTON_MODE_TOGGLE
#endif
#ifndef BUTTON8_CLICK
#define BUTTON8_CLICK       BUTTON_MODE_TOGGLE
#endif

#ifndef BUTTON1_DBLCLICK
#define BUTTON1_DBLCLICK    BUTTON_MODE_AP
#endif
#ifndef BUTTON2_DBLCLICK
#define BUTTON2_DBLCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_DBLCLICK
#define BUTTON3_DBLCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_DBLCLICK
#define BUTTON4_DBLCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON5_DBLCLICK
#define BUTTON5_DBLCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON6_DBLCLICK
#define BUTTON6_DBLCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON7_DBLCLICK
#define BUTTON7_DBLCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON8_DBLCLICK
#define BUTTON8_DBLCLICK    BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_LNGCLICK
#define BUTTON1_LNGCLICK    BUTTON_MODE_RESET
#endif
#ifndef BUTTON2_LNGCLICK
#define BUTTON2_LNGCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_LNGCLICK
#define BUTTON3_LNGCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_LNGCLICK
#define BUTTON4_LNGCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON5_LNGCLICK
#define BUTTON5_LNGCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON6_LNGCLICK
#define BUTTON6_LNGCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON7_LNGCLICK
#define BUTTON7_LNGCLICK    BUTTON_MODE_NONE
#endif
#ifndef BUTTON8_LNGCLICK
#define BUTTON8_LNGCLICK    BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_LNGLNGCLICK
#define BUTTON1_LNGLNGCLICK BUTTON_MODE_FACTORY
#endif
#ifndef BUTTON2_LNGLNGCLICK
#define BUTTON2_LNGLNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON3_LNGLNGCLICK
#define BUTTON3_LNGLNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON4_LNGLNGCLICK
#define BUTTON4_LNGLNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON5_LNGLNGCLICK
#define BUTTON5_LNGLNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON6_LNGLNGCLICK
#define BUTTON6_LNGLNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON7_LNGLNGCLICK
#define BUTTON7_LNGLNGCLICK BUTTON_MODE_NONE
#endif
#ifndef BUTTON8_LNGLNGCLICK
#define BUTTON8_LNGLNGCLICK BUTTON_MODE_NONE
#endif

#ifndef BUTTON1_RELAY
#define BUTTON1_RELAY       0
#endif
#ifndef BUTTON2_RELAY
#define BUTTON2_RELAY       0
#endif
#ifndef BUTTON3_RELAY
#define BUTTON3_RELAY       0
#endif
#ifndef BUTTON4_RELAY
#define BUTTON4_RELAY       0
#endif
#ifndef BUTTON5_RELAY
#define BUTTON5_RELAY       0
#endif
#ifndef BUTTON6_RELAY
#define BUTTON6_RELAY       0
#endif
#ifndef BUTTON7_RELAY
#define BUTTON7_RELAY       0
#endif
#ifndef BUTTON8_RELAY
#define BUTTON8_RELAY       0
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
#define LED2_MODE           LED_MODE_MQTT
#endif
#ifndef LED3_MODE
#define LED3_MODE           LED_MODE_MQTT
#endif
#ifndef LED4_MODE
#define LED4_MODE           LED_MODE_MQTT
#endif
#ifndef LED5_MODE
#define LED5_MODE           LED_MODE_MQTT
#endif
#ifndef LED6_MODE
#define LED6_MODE           LED_MODE_MQTT
#endif
#ifndef LED7_MODE
#define LED7_MODE           LED_MODE_MQTT
#endif
#ifndef LED8_MODE
#define LED8_MODE           LED_MODE_MQTT
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

// -----------------------------------------------------------------------------
// General
// -----------------------------------------------------------------------------

// Default hostname will be ESPURNA_XXXXXX, where XXXXXX is last 3 octets of chipID
#ifndef HOSTNAME
#define HOSTNAME ""
#endif

// Needed for ESP8285 boards under Windows using PlatformIO (?)
#ifndef BUTTON_PUSHBUTTON
#define BUTTON_PUSHBUTTON   0
#define BUTTON_SWITCH       1
#define BUTTON_DEFAULT_HIGH 2
#define BUTTON_SET_PULLUP   4
#endif

// Relay providers
#ifndef RELAY_PROVIDER
#define RELAY_PROVIDER          RELAY_PROVIDER_RELAY
#endif

// Light provider
#ifndef LIGHT_PROVIDER
#define LIGHT_PROVIDER          LIGHT_PROVIDER_NONE
#endif
