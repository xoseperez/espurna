// -----------------------------------------------------------------------------
// Configuration HELP
// -----------------------------------------------------------------------------
//
// MANUFACTURER: Name of the manufacturer of the board ("string")
// DEVICE: Name of the device ("string")
// BUTTON#_PIN: GPIO for the n-th button (1-based, up to 4 buttons)
// BUTTON#_RELAY: Relay number that will be bind to the n-th button (1-based)
// BUTTON#_MODE: A mask of options (BUTTON_PUSHBUTTON and BUTTON_SWITCH cannot be together)
//   - BUTTON_PUSHBUTTON: button event is fired when released
//   - BUTTON_SWITCH: button event is fired when pressed or released
//   - BUTTON_DEFAULT_HIGH: there is a pull up in place
//   - BUTTON_SET_PULLUP: set pullup by software
// RELAY#_PIN: GPIO for the n-th relay (1-based, up to 8 relays)
// RELAY#_TYPE: Relay can be RELAY_TYPE_NORMAL, RELAY_TYPE_INVERSE, RELAY_TYPE_LATCHED or RELAY_TYPE_LATCHED_INVERSE
// LED#_PIN: GPIO for the n-th LED (1-based, up to 8 LEDs)
// LED#_PIN_INVERSE: LED has inversed logic (lit when pulled down)
// LED#_MODE: Check types.h for LED_MODE_%
// LED#_RELAY: Linked relay (1-based)
//
// Besides, other hardware specific information should be stated here

// -----------------------------------------------------------------------------
// ESPurna Core
// -----------------------------------------------------------------------------

#if defined(ESPURNA_CORE)

    // This is a special device targeted to generate a light-weight binary image
    // meant to be able to do two-step-updates:
    // https://github.com/xoseperez/espurna/wiki/TwoStepUpdates

    // Info
    #define MANUFACTURER            "ESPRESSIF"
    #define DEVICE                  "ESPURNA_CORE"

    // Disable non-core modules
    #define ALEXA_SUPPORT           0
    #define BROKER_SUPPORT          0
    #define BUTTON_SUPPORT          0
    #define DOMOTICZ_SUPPORT        0
    #define HOMEASSISTANT_SUPPORT   0
    #define I2C_SUPPORT             0
    #define MDNS_SERVER_SUPPORT     0
    #define MQTT_SUPPORT            0
    #define NTP_SUPPORT             0
    #define SCHEDULER_SUPPORT       0
    #define SENSOR_SUPPORT          0
    #define THINGSPEAK_SUPPORT      0
    #define WEB_SUPPORT             0

    // Extra light-weight image
    //#define DEBUG_SERIAL_SUPPORT    0
    //#define DEBUG_TELNET_SUPPORT    0
    //#define DEBUG_WEB_SUPPORT       0
    //#define LED_SUPPORT             0
    //#define TELNET_SUPPORT          0
    //#define TERMINAL_SUPPORT        0

// -----------------------------------------------------------------------------
// Development boards
// -----------------------------------------------------------------------------

#elif defined(NODEMCU_LOLIN)

    // Info
    #define MANUFACTURER        "NODEMCU"
    #define DEVICE              "LOLIN"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Hidden button will enter AP mode if dblclick and reset the device when long-long-clicked
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // Light
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

#elif defined(NODEMCU_BASIC)
    // Info
    // Generic NodeMCU Board without any buttons or relays connected.
    #define MANUFACTURER        "NODEMCU"
    #define DEVICE              "BASIC"

#elif defined(WEMOS_D1_MINI)

    // Info
    #define MANUFACTURER        "WEMOS"
    #define DEVICE              "D1_MINI"

    // Buttons
    // No buttons on the D1 MINI alone, but defining it without adding a button doen't create problems
    #define BUTTON1_PIN         0   // Connect a pushbutton between D3 and GND,
                                    // it's the same as using a Wemos one button shield
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    #define I2C_SDA_PIN         4  // D2
    #define I2C_SCL_PIN         5  // D1

#elif defined(WEMOS_D1_MINI_RELAYSHIELD)

    // Info
    #define MANUFACTURER        "WEMOS"
    #define DEVICE              "D1_MINI_RELAYSHIELD"

        // Buttons
    // No buttons on the D1 MINI alone, but defining it without adding a button doen't create problems
    #define BUTTON1_PIN         0   // Connect a pushbutton between D3 and GND,
                                    // it's the same as using a Wemos one button shield
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // Light RGBW 
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // When Wemos relay shield is connected GPIO5 (D1) is used for relay,
    // so I2C must be remapped to other pins
    #define I2C_SDA_PIN         12  // D6
    #define I2C_SCL_PIN         14  // D5

#elif defined(WEMOS_D1_TARPUNA_SHIELD)

    // Info
    #define MANUFACTURER        "WEMOS"
    #define DEVICE              "D1_TARPUNA_SHIELD"

// -----------------------------------------------------------------------------
// ESPurna
// -----------------------------------------------------------------------------

#elif defined(TINKERMAN_ESPURNA_H06)

    // Info
    #define MANUFACTURER        "TINKERMAN"
    #define DEVICE              "ESPURNA_H06"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_RELAY       1

    // Normal pushbutton
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_INVERSE

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     2
    #define HLW8012_CF1_PIN     13
    #define HLW8012_CF_PIN      14

#elif defined(TINKERMAN_ESPURNA_H08)

    // Info
    #define MANUFACTURER        "TINKERMAN"
    #define DEVICE              "ESPURNA_H08"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_RELAY       1

    // Normal pushbutton
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     5
    #define HLW8012_CF1_PIN     13
    #define HLW8012_CF_PIN      14

#elif defined(TINKERMAN_ESPURNA_SWITCH)

    // Info
    #define MANUFACTURER        "TINKERMAN"
    #define DEVICE              "ESPURNA_SWITCH"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_RELAY       1

    // Touch button
    #define BUTTON1_MODE            BUTTON_PUSHBUTTON
    #define BUTTON1_PRESS           BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK           BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK        BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK        BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK     BUTTON_MODE_NONE

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_INVERSE


// Check http://tinkerman.cat/rfm69-wifi-gateway/
#elif defined(TINKERMAN_RFM69GW)

    // Info
    #define MANUFACTURER                "TINKERMAN"
    #define DEVICE                      "RFM69GW"

    // Buttons
    #define BUTTON1_PIN                 0
    #define BUTTON1_MODE                BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // RFM69GW
    #define RFM69_SUPPORT               1

    // Disable non-core modules
    #define ALEXA_SUPPORT               0
    #define DOMOTICZ_SUPPORT            0
    #define HOMEASSISTANT_SUPPORT       0
    #define I2C_SUPPORT                 0
    #define SCHEDULER_SUPPORT           0
    #define SENSOR_SUPPORT              0
    #define THINGSPEAK_SUPPORT          0

// -----------------------------------------------------------------------------
// Itead Studio boards
// -----------------------------------------------------------------------------

#elif defined(ITEAD_SONOFF_BASIC)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_BASIC"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1
    #define BUTTON2_PIN         14
    #define BUTTON2_MODE        BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON2_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_RF)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_RF"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1
    #define BUTTON2_PIN         14
    #define BUTTON2_MODE        BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON2_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_TH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_TH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Jack is connected to GPIO14 (and with a small hack to GPIO4)
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT      1
    #endif
    #define DALLAS_PIN          14

    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT         1
    #endif
    #define DHT_PIN             14

    //#define I2C_SDA_PIN         4
    //#define I2C_SCL_PIN         14

#elif defined(ITEAD_SONOFF_SV)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_SV"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SLAMPHER)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SLAMPHER"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_S20)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "S20"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_TOUCH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_TOUCH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK       BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_MODE_RESET
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_POW)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_POW"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            15
    #define LED1_PIN_INVERSE    0

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     5
    #define HLW8012_CF1_PIN     13
    #define HLW8012_CF_PIN      14

#elif defined(ITEAD_SONOFF_POW_R2)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_POW_R2"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT    0

    // CSE7766
    #ifndef CSE7766_SUPPORT
    #define CSE7766_SUPPORT     1
    #endif
    #define CSE7766_PIN         1

#elif defined(ITEAD_SONOFF_DUAL)

    // Info
    #define MANUFACTURER            "ITEAD"
    #define DEVICE                  "SONOFF_DUAL"
    #define SERIAL_BAUDRATE         19230
    #define RELAY_PROVIDER          RELAY_PROVIDER_DUAL
    #define DUMMY_RELAY_COUNT       2
    #define DEBUG_SERIAL_SUPPORT    0

    // Buttons
    #define BUTTON3_RELAY       1

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_DUAL_R2)

    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_DUAL_R2"

    // Buttons
    #define BUTTON1_PIN         0       // Button 0 on header
    #define BUTTON2_PIN         9       // Button 1 on header
    #define BUTTON3_PIN         10      // Physical button
    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON3_RELAY       1
    #define BUTTON1_MODE        BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON3_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_4CH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_4CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9
    #define BUTTON3_PIN         10
    #define BUTTON4_PIN         14

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON4_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON3_RELAY       3
    #define BUTTON4_RELAY       4

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5
    #define RELAY3_PIN          4
    #define RELAY4_PIN          15

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL
    #define RELAY4_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_4CH_PRO)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_4CH_PRO"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9
    #define BUTTON3_PIN         10
    #define BUTTON4_PIN         14

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON4_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON3_RELAY       3
    #define BUTTON4_RELAY       4

    // Sonoff 4CH Pro uses a secondary STM32 microcontroller to handle
    // buttons and relays, but it also forwards button presses to the ESP8285.
    // This allows ESPurna to handle button presses -almost- the same way
    // as with other devices except:
    // * Double click seems to break/disable the button on the STM32 side
    // * With S6 switch to 1 (self-locking and inching modes) everything's OK
    // * With S6 switch to 0 (interlock mode) if there is a relay ON
    //    and you click on another relay button, the STM32 sends a "press"
    //    event for the button of the first relay (to turn it OFF) but it
    //    does not send a "release" event. It's like it's holding the
    //    button down since you can see it is still LOW.
    //    Whatever reason the result is that it may actually perform a
    //    long click or long-long click.
    // The configuration below make the button toggle the relay on press events
    // and disables any possibly harmful combination with S6 set to 0.
    // If you are sure you will only use S6 to 1 you can comment the
    // BUTTON1_LNGCLICK and BUTTON1_LNGLNGCLICK options below to recover the
    // reset mode and factory reset functionalities, or link other actions like
    // AP mode in the commented line below.

    #define BUTTON1_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK       BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK    BUTTON_MODE_NONE
    //#define BUTTON1_LNGCLICK    BUTTON_MODE_AP
    #define BUTTON1_LNGLNGCLICK BUTTON_MODE_NONE
    #define BUTTON2_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON2_CLICK       BUTTON_MODE_NONE
    #define BUTTON3_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON3_CLICK       BUTTON_MODE_NONE
    #define BUTTON4_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON4_CLICK       BUTTON_MODE_NONE

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5
    #define RELAY3_PIN          4
    #define RELAY4_PIN          15

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL
    #define RELAY4_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_1CH_INCHING)

    // The inching functionality is managed by a misterious IC in the board.
    // You cannot control the inching button and functionality from the ESP8266
    // Besides, enabling the inching functionality using the hardware button
    // will result in the relay switching on and off continuously.
    // Fortunately the unkown IC keeps memory of the hardware inching status
    // so you can just disable it and forget. The inching LED must be lit.
    // You can still use the pulse options from the web interface
    // without problem.

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "1CH_INCHING"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_MOTOR)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "MOTOR"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_BNSZ01)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "BNSZ01"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CHANNELS      1
    #define LIGHT_CH1_PIN       12
    #define LIGHT_CH1_INVERSE   0

#elif defined(ITEAD_SONOFF_RFBRIDGE)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_RFBRIDGE"
    #define RELAY_PROVIDER      RELAY_PROVIDER_RFBRIDGE

    // Number of virtual switches
    #ifndef DUMMY_RELAY_COUNT
    #define DUMMY_RELAY_COUNT   8
    #endif

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // RFB Direct hack thanks to @wildwiz
    // https://github.com/xoseperez/espurna/wiki/Hardware-Itead-Sonoff-RF-Bridge---Direct-Hack
    #ifndef RFB_DIRECT
    #define RFB_DIRECT          0
    #endif

    #ifndef RFB_RX_PIN
    #define RFB_RX_PIN          4   // GPIO for RX when RFB_DIRECT
    #endif

    #ifndef RFB_TX_PIN
    #define RFB_TX_PIN          5   // GPIO for TX when RFB_DIRECT
    #endif

    // When using un-modified harware, ESPurna communicates with the secondary
    // MCU EFM8BB1 via UART at 19200 bps so we need to change the speed of
    // the port and remove UART noise on serial line
    #if not RFB_DIRECT
    #define SERIAL_BAUDRATE         19200
    #define DEBUG_SERIAL_SUPPORT    0
    #endif

#elif defined(ITEAD_SONOFF_B1)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_B1"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      5
    #define MY92XX_MODEL        MY92XX_MODEL_MY9231
    #define MY92XX_CHIPS        2
    #define MY92XX_DI_PIN       12
    #define MY92XX_DCKI_PIN     14
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_MAPPING      4, 3, 5, 0, 1
    #define LIGHT_WHITE_FACTOR  (0.1)                    // White LEDs are way more bright in the B1

#elif defined(ITEAD_SONOFF_LED)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_LED"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CHANNELS      2
    #define LIGHT_CH1_PIN       12  // Cold white
    #define LIGHT_CH2_PIN       14  // Warm white
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0

#elif defined(ITEAD_SONOFF_T1_1CH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_T1_1CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK       BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_MODE_RESET
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_T1_2CH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_T1_2CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK       BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON2_CLICK       BUTTON_MODE_NONE
    #define BUTTON2_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON2_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON2_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_T1_3CH)

    // Info
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_T1_3CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9
    #define BUTTON3_PIN         10

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK       BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON2_CLICK       BUTTON_MODE_NONE
    #define BUTTON2_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON2_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON2_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON3_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON3_CLICK       BUTTON_MODE_NONE
    #define BUTTON3_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON3_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON3_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON3_RELAY       3

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5
    #define RELAY3_PIN          4

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

#elif defined(ITEAD_SONOFF_S31)

    // Info
    #define MANUFACTURER            "ITEAD"
    #define DEVICE                  "SONOFF_S31"

    // Buttons
    #define BUTTON1_PIN             0
    #define BUTTON1_MODE            BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY           1

    // Relays
    #define RELAY1_PIN              12
    #define RELAY1_TYPE             RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                13
    #define LED1_PIN_INVERSE        1

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT    0

    // CSE7766
    #define CSE7766_SUPPORT         1
    #define CSE7766_PIN             1

#elif defined(ITEAD_SONOFF_IFAN02)

    // Info
    #define MANUFACTURER            "ITEAD"
    #define DEVICE                  "SONOFF_IFAN02"

    // These are virtual buttons triggered by the remote
    #define BUTTON1_PIN             0
    #define BUTTON2_PIN             9
    #define BUTTON3_PIN             10
    #define BUTTON4_PIN             14
    #define BUTTON1_MODE            BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE            BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_MODE            BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON4_MODE            BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN              12
    #define RELAY2_PIN              5
    #define RELAY3_PIN              4
    #define RELAY4_PIN              15
    #define RELAY1_TYPE             RELAY_TYPE_NORMAL
    #define RELAY2_TYPE             RELAY_TYPE_NORMAL
    #define RELAY3_TYPE             RELAY_TYPE_NORMAL
    #define RELAY4_TYPE             RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                13
    #define LED1_PIN_INVERSE        1

// -----------------------------------------------------------------------------
// ORVIBO
// -----------------------------------------------------------------------------

#elif defined(ORVIBO_B25)

    // Info
    #define MANUFACTURER        "ORVIBO"
    #define DEVICE              "B25"

    // Buttons
    #define BUTTON1_PIN         14
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            12   // 4 blue led
    #define LED1_PIN_INVERSE    1
    #define LED2_PIN            4  // 12 red led
    #define LED2_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// YJZK
// -----------------------------------------------------------------------------

#elif defined(YJZK_SWITCH_1CH)

    // Info
    #define MANUFACTURER        "YJZK"
    #define DEVICE              "SWITCH_1CH"

    // Buttons
    #define BUTTON1_PIN         0

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK       BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    0

#elif defined(YJZK_SWITCH_2CH)

    // Info
    #define MANUFACTURER        "YJZK"
    #define DEVICE              "SWITCH_2CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK       BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON2_CLICK       BUTTON_MODE_NONE
    #define BUTTON2_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON2_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON2_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    0

// YJZK 3CH switch
// Also Lixin Touch Wifi 3M

#elif defined(YJZK_SWITCH_3CH)

    // Info
    #define MANUFACTURER        "YJZK"
    #define DEVICE              "SWITCH_3CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9
    #define BUTTON3_PIN         10

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK       BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON2_CLICK       BUTTON_MODE_NONE
    #define BUTTON2_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON2_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON2_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON3_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON3_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON3_CLICK       BUTTON_MODE_NONE
    #define BUTTON3_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON3_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON3_LNGLNGCLICK BUTTON_MODE_RESET

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON3_RELAY       3

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          5
    #define RELAY3_PIN          4

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// Electrodragon boards
// -----------------------------------------------------------------------------

#elif defined(ELECTRODRAGON_WIFI_IOT)

    // Info
    #define MANUFACTURER        "ELECTRODRAGON"
    #define DEVICE              "WIFI_IOT"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         2

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          13

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            16
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// WorkChoice ecoPlug
// -----------------------------------------------------------------------------

#elif defined(WORKCHOICE_ECOPLUG)

    // Info
    #define MANUFACTURER        "WORKCHOICE"
    #define DEVICE              "ECOPLUG"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          15
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// AI Thinker
// -----------------------------------------------------------------------------

#elif defined(AITHINKER_AI_LIGHT)

    // Info
    #define MANUFACTURER        "AITHINKER"
    #define DEVICE              "AI_LIGHT"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      4
    #define MY92XX_MODEL        MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS        1
    #define MY92XX_DI_PIN       13
    #define MY92XX_DCKI_PIN     15
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_MAPPING      0, 1, 2, 3

// -----------------------------------------------------------------------------
// LED Controller
// -----------------------------------------------------------------------------

#elif defined(MAGICHOME_LED_CONTROLLER)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "LED_CONTROLLER"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       13      // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

    // IR
    #define IR_SUPPORT          1
    #define IR_RX_PIN           4
    #define IR_BUTTON_SET       1

#elif defined(MAGICHOME_LED_CONTROLLER_20)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "LED_CONTROLLER_20"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

    // IR
    #define IR_SUPPORT          1
    #define IR_RX_PIN           4
    #define IR_BUTTON_SET       1

#elif defined(MAGICHOME_ZJ_WFMN_A_11)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "ZJ_WFMN_A_11"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1
    #define LED2_PIN            15
    #define LED2_PIN_INVERSE    1

    // Light
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       14      // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

    // IR
    #define IR_SUPPORT          1
    #define IR_RX_PIN           4
    #define IR_BUTTON_SET       1

#elif defined(MAGICHOME_ZJ_WFMN_B_11)

    // Info
    #define MANUFACTURER        "MAGICHOME"
    #define DEVICE              "ZJ_WFMN_B_11"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1
    #define LED2_PIN            15
    #define LED2_PIN_INVERSE    1

    // Light
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       13      // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

    // RF
    #define RF_SUPPORT          1
    #define RF_PIN              4

// -----------------------------------------------------------------------------
// HUACANXING H801 & H802
// -----------------------------------------------------------------------------

#elif defined(HUACANXING_H801)

    // Info
    #define MANUFACTURER        "HUACANXING"
    #define DEVICE              "H801"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1
    #define DEBUG_PORT          Serial1
    #define SERIAL_RX_ENABLED   1

    // LEDs
    #define LED1_PIN            5
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CHANNELS      5
    #define LIGHT_CH1_PIN       15      // RED
    #define LIGHT_CH2_PIN       13      // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       14      // WHITE1
    #define LIGHT_CH5_PIN       4       // WHITE2
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0
    #define LIGHT_CH5_INVERSE   0

#elif defined(HUACANXING_H802)

    // Info
    #define MANUFACTURER        "HUACANXING"
    #define DEVICE              "H802"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1
    #define DEBUG_PORT          Serial1
    #define SERIAL_RX_ENABLED   1

    // Light
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       14      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

// -----------------------------------------------------------------------------
// Jan Goedeke Wifi Relay
// https://github.com/JanGoe/esp8266-wifi-relay
// -----------------------------------------------------------------------------

#elif defined(JANGOE_WIFI_RELAY_NC)

    // Info
    #define MANUFACTURER        "JANGOE"
    #define DEVICE              "WIFI_RELAY_NC"

    // Buttons
    #define BUTTON1_PIN         12
    #define BUTTON2_PIN         13

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          2
    #define RELAY2_PIN          14

    #define RELAY1_TYPE         RELAY_TYPE_INVERSE
    #define RELAY2_TYPE         RELAY_TYPE_INVERSE

#elif defined(JANGOE_WIFI_RELAY_NO)

    // Info
    #define MANUFACTURER        "JANGOE"
    #define DEVICE              "WIFI_RELAY_NO"

    // Buttons
    #define BUTTON1_PIN         12
    #define BUTTON2_PIN         13

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          2
    #define RELAY2_PIN          14

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

// -----------------------------------------------------------------------------
// Jorge García Wifi+Relays Board Kit
// https://www.tindie.com/products/jorgegarciadev/wifi--relays-board-kit
// https://github.com/jorgegarciadev/wifikit
// -----------------------------------------------------------------------------

#elif defined(JORGEGARCIA_WIFI_RELAYS)

    // Info
    #define MANUFACTURER        "JORGEGARCIA"
    #define DEVICE              "WIFI_RELAYS"

    // Relays
    #define RELAY1_PIN          0
    #define RELAY2_PIN          2

    #define RELAY1_TYPE         RELAY_TYPE_INVERSE
    #define RELAY2_TYPE         RELAY_TYPE_INVERSE

// -----------------------------------------------------------------------------
// WiFi MQTT Relay / Thermostat
// -----------------------------------------------------------------------------

#elif defined(OPENENERGYMONITOR_MQTT_RELAY)

    // Info
    #define MANUFACTURER        "OPENENERGYMONITOR"
    #define DEVICE              "MQTT_RELAY"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            16
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// WiOn 50055 Indoor Wi-Fi Wall Outlet & Tap
// https://rover.ebay.com/rover/1/711-53200-19255-0/1?icep_id=114&ipn=icep&toolid=20004&campid=5338044841&mpre=http%3A%2F%2Fwww.ebay.com%2Fitm%2FWiOn-50050-Indoor-Wi-Fi-Outlet-Wireless-Switch-Programmable-Timer-%2F263112281551
// https://rover.ebay.com/rover/1/711-53200-19255-0/1?icep_id=114&ipn=icep&toolid=20004&campid=5338044841&mpre=http%3A%2F%2Fwww.ebay.com%2Fitm%2FWiOn-50055-Indoor-Wi-Fi-Wall-Tap-Monitor-Energy-Usage-Wireless-Smart-Switch-%2F263020837777
// -----------------------------------------------------------------------------

#elif defined(WION_50055)

    // Currently untested, does not support energy monitoring

    // Info
    #define MANUFACTURER        "WION"
    #define DEVICE              "50055"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          15
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// EX-Store Wifi Relay v3.1
// https://ex-store.de/ESP8266-WiFi-Relay-V31
// -----------------------------------------------------------------------------

#elif defined(EXS_WIFI_RELAY_V31)

    // Untested

    // Info
    #define MANUFACTURER        "EXS"
    #define DEVICE              "WIFI_RELAY_V31"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          13
    #define RELAY1_TYPE         RELAY_TYPE_LATCHED
    #define RELAY1_RESET_PIN    12

// -----------------------------------------------------------------------------
// EX-Store Wifi Relay v5.0
// -----------------------------------------------------------------------------

#elif defined(EXS_WIFI_RELAY_V50)

    // Info
    #define MANUFACTURER        "EXS"
    #define DEVICE              "WIFI_RELAY_V50"

    // Buttons
    #define BUTTON1_PIN         5
    #define BUTTON2_PIN         4
    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2
    #define BUTTON1_MODE        BUTTON_SWITCH | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON2_MODE        BUTTON_SWITCH | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP

    // Relays
    #define RELAY1_PIN          14
    #define RELAY1_TYPE         RELAY_TYPE_LATCHED
    #define RELAY1_RESET_PIN    16
    #define RELAY2_PIN          13
    #define RELAY2_TYPE         RELAY_TYPE_LATCHED
    #define RELAY2_RESET_PIN    12

    // LEDs
    #define LED1_PIN            15
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// V9261F
// -----------------------------------------------------------------------------

#elif defined(GENERIC_V9261F)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "V9261F"
    #define ALEXA_SUPPORT       0

    // V9261F
    #define V9261F_SUPPORT      1
    #define V9261F_PIN          2
    #define V9261F_PIN_INVERSE  1

// -----------------------------------------------------------------------------
// ECH1560
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ECH1560)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ECH1560"
    #define ALEXA_SUPPORT       0

    // ECH1560
    #define ECH1560_SUPPORT     1
    #define ECH1560_CLK_PIN     4
    #define ECH1560_MISO_PIN    5
    #define ECH1560_INVERTED    0

// -----------------------------------------------------------------------------
// ESPLive
// https://github.com/ManCaveMade/ESP-Live
// -----------------------------------------------------------------------------

#elif defined(MANCAVEMADE_ESPLIVE)

    // Info
    #define MANUFACTURER        "MANCAVEMADE"
    #define DEVICE              "ESPLIVE"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON2_PIN         5

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          12
    #define RELAY2_PIN          13

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // DS18B20
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT             	1
    #endif
    #define DALLAS_PIN                 	2
    #define DALLAS_UPDATE_INTERVAL     	5000
    #define TEMPERATURE_MIN_CHANGE      1.0

// -----------------------------------------------------------------------------
// QuinLED
// http://blog.quindorian.org/2017/02/esp8266-led-lighting-quinled-v2-6-pcb.html
// -----------------------------------------------------------------------------

#elif defined(INTERMITTECH_QUINLED)

    // Info
    #define MANUFACTURER        "INTERMITTECH"
    #define DEVICE              "QUINLED"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // LEDs
    #define LED1_PIN            5
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CHANNELS      2
    #define LIGHT_CH1_PIN       0
    #define LIGHT_CH2_PIN       2
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0

// -----------------------------------------------------------------------------
// Arilux AL-LC06
// -----------------------------------------------------------------------------

#elif defined(ARILUX_AL_LC01)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL_LC01"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      3
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0

#elif defined(ARILUX_AL_LC02)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL_LC02"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE1
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

#elif defined(ARILUX_AL_LC02V14)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL_LC02V14"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       13      // WHITE1
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

#elif defined(ARILUX_AL_LC06)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL_LC06"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      5
    #define LIGHT_CH1_PIN       14      // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       15      // WHITE1
    #define LIGHT_CH5_PIN       5       // WHITE2
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0
    #define LIGHT_CH5_INVERSE   0

#elif defined(ARILUX_AL_LC11)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "AL_LC11"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      5
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       4       // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       13      // WHITE1
    #define LIGHT_CH5_PIN       12      // WHITE1
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0
    #define LIGHT_CH5_INVERSE   0

#elif defined(ARILUX_E27)

    // Info
    #define MANUFACTURER        "ARILUX"
    #define DEVICE              "E27"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      4
    #define MY92XX_MODEL        MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS        1
    #define MY92XX_DI_PIN       13
    #define MY92XX_DCKI_PIN     15
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_MAPPING      0, 1, 2, 3

// -----------------------------------------------------------------------------
// XENON SM-PW701U
// -----------------------------------------------------------------------------

#elif defined(XENON_SM_PW702U)

    // Info
    #define MANUFACTURER        "XENON"
    #define DEVICE              "SM_PW702U"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// AUTHOMETION LYT8266
// https://authometion.com/shop/en/home/13-lyt8266.html
// -----------------------------------------------------------------------------

#elif defined(AUTHOMETION_LYT8266)

    // Info
    #define MANUFACTURER        "AUTHOMETION"
    #define DEVICE              "LYT8266"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       13      // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       2       // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

    #define LIGHT_ENABLE_PIN    15

#elif defined(GIZWITS_WITTY_CLOUD)

    // Info
    #define MANUFACTURER        "GIZWITS"
    #define DEVICE              "WITTY_CLOUD"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS       BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK       BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK    BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK BUTTON_MODE_RESET

    #define ANALOG_SUPPORT      1

    // LEDs
    #define LED1_PIN            2      // BLUE build-in
    #define LED1_PIN_INVERSE    1

    // Light
    #define LIGHT_CHANNELS      3
    #define LIGHT_CH1_PIN       15       // RED
    #define LIGHT_CH2_PIN       12       // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0

// -----------------------------------------------------------------------------
// KMC 70011
// https://www.amazon.com/KMC-Monitoring-Required-Control-Compatible/dp/B07313TH7B
// -----------------------------------------------------------------------------

#elif defined(KMC_70011)

    // Info
    #define MANUFACTURER        "KMC"
    #define DEVICE              "70011"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          14
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     12
    #define HLW8012_CF1_PIN     5
    #define HLW8012_CF_PIN      4

    #define HLW8012_VOLTAGE_R_UP            ( 2 * 1000000 )  // Upstream voltage resistor

// -----------------------------------------------------------------------------
// Euromate (?) Wifi Stecker Schuko
// https://www.obi.de/hausfunksteuerung/wifi-stecker-schuko/p/2291706
// Thanks to @Geitde
// -----------------------------------------------------------------------------

#elif defined(EUROMATE_WIFI_STECKER_SCHUKO)

    // Info
    #define MANUFACTURER        "EUROMATE"
    #define DEVICE              "WIFI_STECKER_SCHUKO"

    // Buttons
    #define BUTTON1_PIN         14
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // The relay in the device is not a bistable (latched) relay.
    // The device is reported to have a flip-flop circuit to drive the relay
    // So @Geitde hack is still the only possible

    // Hack: drive GPIO12 low and use GPIO5 as normal relay pin:
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define LED2_PIN            12 /* DUMMY: exploit default off state for GPIO12=low */
    #define LED2_PIN_INVERSE    0

    // LEDs
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// Euromate (?) Wifi Stecker Schuko Version 2
// This configuration is for the second generation of devices sold by OBI.
// https://www.obi.de/hausfunksteuerung/wifi-stecker-schuko-weiss/p/4077806
// -----------------------------------------------------------------------------
#elif defined(EUROMATE_WIFI_STECKER_SCHUKO_V2)

    // Info
    #define MANUFACTURER        "EUROMATE"
    #define DEVICE              "WIFI_STECKER_SCHUKO_V2"

    // Buttons
    #define BUTTON1_PIN         5
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // Green
    #define LED1_PIN            12
    #define LED1_MODE           LED_MODE_WIFI
    #define LED1_PIN_INVERSE    0

    // Red
    #define LED2_PIN            13
    #define LED2_MODE           LED_MODE_RELAY
    #define LED2_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// Generic 8CH
// -----------------------------------------------------------------------------

#elif defined(GENERIC_8CH)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "8CH"

    // Relays
    #define RELAY1_PIN          0
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_PIN          2
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_PIN          4
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL
    #define RELAY4_PIN          5
    #define RELAY4_TYPE         RELAY_TYPE_NORMAL
    #define RELAY5_PIN          12
    #define RELAY5_TYPE         RELAY_TYPE_NORMAL
    #define RELAY6_PIN          13
    #define RELAY6_TYPE         RELAY_TYPE_NORMAL
    #define RELAY7_PIN          14
    #define RELAY7_TYPE         RELAY_TYPE_NORMAL
    #define RELAY8_PIN          15
    #define RELAY8_TYPE         RELAY_TYPE_NORMAL

// -----------------------------------------------------------------------------
// STM RELAY
// -----------------------------------------------------------------------------

#elif defined(STM_RELAY)

    // Info
    #define MANUFACTURER            "STM_RELAY"
    #define DEVICE                  "2CH"

    // Relays
    #define DUMMY_RELAY_COUNT       2
    #define RELAY_PROVIDER          RELAY_PROVIDER_STM

    // Remove UART noise on serial line
    #define DEBUG_SERIAL_SUPPORT    0

// -----------------------------------------------------------------------------
// Tonbux Powerstrip02
// -----------------------------------------------------------------------------

#elif defined(TONBUX_POWERSTRIP02)

    // Info
    #define MANUFACTURER        "TONBUX"
    #define DEVICE              "POWERSTRIP02"

    // Buttons
    #define BUTTON1_PIN         5
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       0

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_INVERSE
    #define RELAY2_PIN          13
    #define RELAY2_TYPE         RELAY_TYPE_INVERSE
    #define RELAY3_PIN          12
    #define RELAY3_TYPE         RELAY_TYPE_INVERSE
    #define RELAY4_PIN          14
    #define RELAY4_TYPE         RELAY_TYPE_INVERSE
    // Not a relay. USB ports on/off
    #define RELAY5_PIN          16
    #define RELAY5_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            0   // 1 blue led
    #define LED1_PIN_INVERSE    1
    #define LED2_PIN            3   // 3 red leds
    #define LED2_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// Lingan SWA1
// -----------------------------------------------------------------------------

#elif defined(LINGAN_SWA1)

    // Info
    #define MANUFACTURER        "LINGAN"
    #define DEVICE              "SWA1"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          5
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// HEYGO HY02
// -----------------------------------------------------------------------------

#elif defined(HEYGO_HY02)

    // Info
    #define MANUFACTURER		"HEYGO"
    #define DEVICE				"HY02"

    // Buttons
    #define BUTTON1_PIN			13
    #define BUTTON1_MODE		BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY		1

    // Relays
    #define RELAY1_PIN			12
    #define RELAY1_TYPE			RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN			4
    #define LED1_PIN_INVERSE	0

// -----------------------------------------------------------------------------
// Maxcio W-US002S
// -----------------------------------------------------------------------------

#elif defined(MAXCIO_WUS002S)

    // Info
    #define MANUFACTURER		"MAXCIO"
    #define DEVICE				"WUS002S"

    // Buttons
    #define BUTTON1_PIN			2
    #define BUTTON1_MODE		BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY		1

    // Relays
    #define RELAY1_PIN			13
    #define RELAY1_TYPE			RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN			3
    #define LED1_PIN_INVERSE	0

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT		1
    #endif
    #define HLW8012_SEL_PIN		12
    #define HLW8012_CF1_PIN		5
    #define HLW8012_CF_PIN		4

    #define HLW8012_CURRENT_R               0.002            // Current resistor
    #define HLW8012_VOLTAGE_R_UP            ( 2 * 1000000 )  // Upstream voltage resistor

// -----------------------------------------------------------------------------
// Maxcio W-DE004
// -----------------------------------------------------------------------------

#elif defined(MAXCIO_WDE004)

    // Info
    #define MANUFACTURER		"MAXCIO"
    #define DEVICE				"WDE004"

    // Buttons
    #define BUTTON1_PIN			1
    #define BUTTON1_MODE		BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY		1

    // Relays
    #define RELAY1_PIN			14
    #define RELAY1_TYPE			RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN			13
    #define LED1_PIN_INVERSE	1

// -----------------------------------------------------------------------------
// YiDian XS-SSA05
// -----------------------------------------------------------------------------

#elif defined(YIDIAN_XSSSA05)

    // Info
    #define MANUFACTURER		"YIDIAN"
    #define DEVICE				"XSSSA05"

    // Buttons
    #define BUTTON1_PIN			13
    #define BUTTON1_MODE		BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY		1

    // Relays
    #define RELAY1_PIN			12
    #define RELAY1_TYPE			RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN			0  // red
    #define LED1_PIN_INVERSE	1
    #define LED1_MODE           LED_MODE_WIFI

    #define LED2_PIN			15  // blue
    #define LED2_PIN_INVERSE	1
    #define LED2_MODE           LED_MODE_RELAY

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     3
    #define HLW8012_CF1_PIN     14
    #define HLW8012_CF_PIN      5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// TONBUX XS-SSA01
// -----------------------------------------------------------------------------

#elif defined(TONBUX_XSSSA01)

    // Info
    #define MANUFACTURER        "TONBUX"
    #define DEVICE              "XSSSA01"

    // Buttons
    #define BUTTON1_PIN         4
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          14
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// TONBUX XS-SSA06
// -----------------------------------------------------------------------------

#elif defined(TONBUX_XSSSA06)

    // Info
    #define MANUFACTURER        "TONBUX"
    #define DEVICE              "XSSSA06"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          15
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            0   // R - 8 rgb led ring
    #define LED1_PIN_INVERSE    0
    #define LED2_PIN            5   // G
    #define LED2_PIN_INVERSE    0
    #define LED3_PIN            2   // B
    #define LED3_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// GREEN ESP8266 RELAY MODULE
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180323113846&SearchText=Green+ESP8266
// -----------------------------------------------------------------------------

#elif defined(GREEN_ESP8266RELAY)

    // Info
    #define MANUFACTURER        "GREEN"
    #define DEVICE              "ESP8266RELAY"

    // Buttons
    // Not a button but input via Optocoupler
    #define BUTTON1_PIN         5
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// Henrique Gravina ESPIKE
// https://github.com/Henriquegravina/Espike
// -----------------------------------------------------------------------------

#elif defined(IKE_ESPIKE)

    #define MANUFACTURER            "IKE"
    #define DEVICE                  "ESPIKE"

    #define BUTTON1_LNGLNGCLICK     BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK        BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK        BUTTON_MODE_NONE

    #define BUTTON1_PIN             13
    #define BUTTON1_RELAY           1
    #define BUTTON1_MODE            BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON2_PIN             12
    #define BUTTON2_RELAY           2
    #define BUTTON2_MODE            BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define BUTTON3_PIN             14
    #define BUTTON3_RELAY           3
    #define BUTTON3_MODE            BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    #define RELAY1_PIN              4
    #define RELAY1_TYPE             RELAY_TYPE_NORMAL

    #define RELAY2_PIN              5
    #define RELAY2_TYPE             RELAY_TYPE_NORMAL

    #define RELAY3_PIN              16
    #define RELAY3_TYPE             RELAY_TYPE_NORMAL

    #define LED1_PIN                2
    #define LED1_PIN_INVERSE        1

// -----------------------------------------------------------------------------
// SWIFITCH
// https://github.com/ArnieX/swifitch
// -----------------------------------------------------------------------------

#elif defined(ARNIEX_SWIFITCH)

    // Info
    #define MANUFACTURER          "ARNIEX"
    #define DEVICE                "SWIFITCH"

    // Buttons
    #define BUTTON1_PIN           4 // D2
    #define BUTTON1_MODE          BUTTON_SWITCH | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY         1

    #define BUTTON1_PRESS         BUTTON_MODE_NONE
    #define BUTTON1_CLICK         BUTTON_MODE_TOGGLE
    #define BUTTON1_DBLCLICK      BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK      BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK   BUTTON_MODE_NONE

    // Relays
    #define RELAY1_PIN            5 // D1
    #define RELAY1_TYPE           RELAY_TYPE_INVERSE

    // LEDs
    #define LED1_PIN              12 // D6
    #define LED1_PIN_INVERSE      1

// -----------------------------------------------------------------------------
// ESP-01S RELAY v4.0
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180404024035&SearchText=esp-01s+relay
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_RELAY_V40)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ESP01S_RELAY_40"

    // Relays
    #define RELAY1_PIN          0
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0

// -----------------------------------------------------------------------------
// ESP-01S RGB LED v1.0 (some sold with ws2818)
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180404023816&SearchText=esp-01s+led+controller
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_RGBLED_V10)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ESP01S_RGBLED_10"

    // This board is sold as RGB LED module BUT it has on board 3 pin ph2.0 connector (VCC, GPIO2, GND)
    // so, if you wish, you may connect LED, BUTTON, RELAY, SENSOR etc.

    // Buttons
    //#define BUTTON1_PIN         2

    // Relays
    //#define RELAY1_PIN          2

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    0


// -----------------------------------------------------------------------------
// ESP-01S DHT11 v1.0
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180410105907&SearchText=esp-01s+dht11
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_DHT11_V10)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ESP01S_DHT11_10"

    // DHT11
    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT         1
    #endif
    #define DHT_PIN             2
    #define DHT_TYPE            DHT_CHIP_DHT11

// -----------------------------------------------------------------------------
// ESP-01S DS18B20 v1.0
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180410105933&SearchText=esp-01s+ds18b20
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_DS18B20_V10)

    // Info
    #define MANUFACTURER        "GENERIC"
    #define DEVICE              "ESP01S_DS18B20_10"

    // DB18B20
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT      1
    #endif
    #define DALLAS_PIN          2

// -----------------------------------------------------------------------------
// ESP-DIN relay board V1
// https://github.com/pilotak/esp_din
// -----------------------------------------------------------------------------

#elif defined(PILOTAK_ESP_DIN_V1)

    // Info
    #define MANUFACTURER        "PILOTAK"
    #define DEVICE              "ESP_DIN_V1"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    #define RELAY2_PIN          5
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            15
    #define LED1_PIN_INVERSE    0

    #define I2C_SDA_PIN         12
    #define I2C_SCL_PIN         13

    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT      1
    #endif
    #define DALLAS_PIN          2

    #ifndef RF_SUPPORT
    #define RF_SUPPORT          1
    #endif
    #define RF_PIN              14

    #ifndef DIGITAL_SUPPORT
    #define DIGITAL_SUPPORT      1
    #endif
    #define DIGITAL_PIN          16
    #define DIGITAL_PIN_MODE     INPUT

// -----------------------------------------------------------------------------
// Heltec Touch Relay
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180408043114&SearchText=esp8266+touch+relay
// -----------------------------------------------------------------------------

#elif defined(HELTEC_TOUCHRELAY)

    // Info
    #define MANUFACTURER        "HELTEC"
    #define DEVICE              "TOUCH_RELAY"

    // Buttons
    #define BUTTON1_PIN         14
    #define BUTTON1_RELAY       1
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL


// -----------------------------------------------------------------------------
// Zhilde ZLD-EU44-W
// http://www.zhilde.com/product/60705150109-805652505/EU_WiFi_Surge_Protector_Extension_Socket_4_Outlets_works_with_Amazon_Echo_Smart_Power_Strip.html
// -----------------------------------------------------------------------------

#elif defined(ZHILDE_EU44_W)

    // Info
    #define MANUFACTURER            "ZHILDE"
    #define DEVICE                  "EU44_W"

    // Based on the reporter, this product uses GPIO1 and 3 for the button
    // and onboard LED, so hardware serial should be disabled...
    #define DEBUG_SERIAL_SUPPORT    0

    // Buttons
    #define BUTTON1_PIN             3
    #define BUTTON1_MODE            BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Relays
    #define RELAY1_PIN              5
    #define RELAY2_PIN              4
    #define RELAY3_PIN              12
    #define RELAY4_PIN              13
    #define RELAY5_PIN              14
    #define RELAY1_TYPE             RELAY_TYPE_NORMAL
    #define RELAY2_TYPE             RELAY_TYPE_NORMAL
    #define RELAY3_TYPE             RELAY_TYPE_NORMAL
    #define RELAY4_TYPE             RELAY_TYPE_NORMAL
    #define RELAY5_TYPE             RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                1
    #define LED1_PIN_INVERSE        1

    // -----------------------------------------------------------------------------
    // Allnet 4duino ESP8266-UP-Relais
    // http://www.allnet.de/de/allnet-brand/produkte/neuheiten/p/allnet-4duino-iot-wlan-relais-unterputz-esp8266-up-relais/
    // https://shop.allnet.de/fileadmin/transfer/products/148814.pdf
    // -----------------------------------------------------------------------------

#elif defined(ALLNET_4DUINO_IOT_WLAN_RELAIS)

    // Info
    #define MANUFACTURER            "ALLNET"
    #define DEVICE                  "4DUINO_IOT_WLAN_RELAIS"

    // Relays
    #define RELAY1_PIN              14
    #define RELAY1_RESET_PIN        12
    #define RELAY1_TYPE             RELAY_TYPE_LATCHED

    // LEDs
    #define LED1_PIN                0
    #define LED1_PIN_INVERSE        1

    // Buttons
    //#define BUTTON1_PIN             0
    //#define BUTTON1_MODE            BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // Using pins labelled as SDA & SCL as buttons
    #define BUTTON2_PIN             4
    #define BUTTON2_MODE            BUTTON_PUSHBUTTON
    #define BUTTON2_PRESS           BUTTON_MODE_TOGGLE
    #define BUTTON2_CLICK           BUTTON_MODE_NONE
    #define BUTTON2_DBLCLICK        BUTTON_MODE_NONE
    #define BUTTON2_LNGCLICK        BUTTON_MODE_NONE
    #define BUTTON2_LNGLNGCLICK     BUTTON_MODE_NONE

    #define BUTTON3_PIN             5
    #define BUTTON3_MODE            BUTTON_PUSHBUTTON

    // Using pins labelled as SDA & SCL for I2C
    //#define I2C_SDA_PIN             4
    //#define I2C_SCL_PIN             5


// -----------------------------------------------------------------------------
// Luani HVIO
// https://luani.de/projekte/esp8266-hvio/
// https://luani.de/blog/esp8266-230v-io-modul/
// -----------------------------------------------------------------------------

#elif defined(LUANI_HVIO)

    // Info
    #define MANUFACTURER            "LUANI"
    #define DEVICE                  "HVIO"

    // Buttons
    #define BUTTON1_PIN             12
    #define BUTTON1_RELAY           1
    #define BUTTON1_MODE            BUTTON_SWITCH | BUTTON_DEFAULT_HIGH //Hardware Pullup

    #define BUTTON1_PRESS           BUTTON_MODE_NONE
    #define BUTTON1_CLICK           BUTTON_MODE_TOGGLE
    #define BUTTON1_DBLCLICK        BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK        BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK     BUTTON_MODE_NONE

    #define BUTTON2_PIN             13
    #define BUTTON2_RELAY           2
    #define BUTTON2_MODE            BUTTON_SWITCH | BUTTON_DEFAULT_HIGH //Hardware Pullup

    #define BUTTON2_CLICK          BUTTON_MODE_TOGGLE

    // Relays
    #define RELAY1_PIN              4
    #define RELAY2_PIN              5
    #define RELAY1_TYPE             RELAY_TYPE_NORMAL
    #define RELAY2_TYPE             RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                15
    #define LED1_PIN_INVERSE        0

// -----------------------------------------------------------------------------
// Tonbux 50-100M Smart Mosquito Killer USB
// https://www.aliexpress.com/item/Original-Tonbux-50-100M-Smart-Mosquito-Killer-USB-Plug-No-Noise-Repellent-App-Smart-Module/32859330820.html
// -----------------------------------------------------------------------------

#elif defined(TONBUX_MOSQUITO_KILLER)

    // Info
    #define MANUFACTURER        "TONBUX"
    #define DEVICE              "MOSQUITO_KILLER"

    // Buttons
    #define BUTTON1_PIN         2
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          5   // not a relay, fan
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            15  // blue led
    #define LED1_PIN_INVERSE    1
    #define LED1_MODE           LED_MODE_WIFI
    #define LED2_PIN            14  // red led
    #define LED2_PIN_INVERSE    1
    #define LED2_MODE           LED_MODE_RELAY

    #define LED3_PIN            12  // UV leds (1-2-3-4-5-6-7-8)
    #define LED3_PIN_INVERSE    0
    #define LED3_RELAY          1
    #define LED4_PIN            16  // UV leds (9-10-11)
    #define LED4_PIN_INVERSE    0
    #define LED4_RELAY          1

// -----------------------------------------------------------------------------
// NEO Coolcam NAS-WR01W Wifi Smart Power Plug
// https://es.aliexpress.com/item/-/32854589733.html?spm=a219c.12010608.0.0.6d084e68xX0y5N
// https://www.fasttech.com/product/9649426-neo-coolcam-nas-wr01w-wifi-smart-power-plug-eu
// -----------------------------------------------------------------------------

#elif defined(NEO_COOLCAM_NAS_WR01W)

    // Info
    #define MANUFACTURER        "NEO_COOLCAM"
    #define DEVICE              "NAS_WR01W"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    1


// ------------------------------------------------------------------------------
// Estink Wifi Power Strip
// https://www.amazon.de/Steckdosenleiste-Ladeger%C3%A4t-Sprachsteuerung-SmartphonesTablets-Android/dp/B0796W5FZY
// Fornorm Wi-Fi USB Extension Socket (ZLD-34EU)
// https://www.aliexpress.com/item/Fornorm-WiFi-Extension-Socket-with-Surge-Protector-Smart-Power-Strip-3-Outlets-and-4-USB-Charging/32849743948.html
// -----------------------------------------------------------------------------

#elif defined(ESTINK_WIFI_POWER_STRIP)

    // Info
    #define MANUFACTURER        "ESTINK"
    #define DEVICE              "WIFI_POWER_STRIP"

    // Disable UART noise since this board uses GPIO3
    #define DEBUG_SERIAL_SUPPORT    0

    // Buttons
    #define BUTTON1_PIN         16
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       4

    // Relays
    #define RELAY1_PIN          14  // USB power
    #define RELAY2_PIN          13  // power plug 1
    #define RELAY3_PIN          4   // power plug 2
    #define RELAY4_PIN          15  // power plug 3

    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL
    #define RELAY3_TYPE         RELAY_TYPE_NORMAL
    #define RELAY4_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            0   // power led
    #define LED2_PIN            12  // power plug 1
    #define LED3_PIN            3   // power plug 2
    #define LED4_PIN            5   // power plug 3

    #define LED1_PIN_INVERSE    1
    #define LED2_PIN_INVERSE    1
    #define LED3_PIN_INVERSE    1
    #define LED4_PIN_INVERSE    1

    #define LED1_MODE           LED_MODE_FINDME
    #define LED2_MODE           LED_MODE_FOLLOW
    #define LED3_MODE           LED_MODE_FOLLOW
    #define LED4_MODE           LED_MODE_FOLLOW

    #define LED2_RELAY          2
    #define LED3_RELAY          3
    #define LED4_RELAY          4


// -----------------------------------------------------------------------------
// Bruno Horta's OnOfre
// https://www.bhonofre.pt/
// https://github.com/brunohorta82/BH_OnOfre/
// -----------------------------------------------------------------------------

#elif defined(BH_ONOFRE)

    // Info
    #define MANUFACTURER        "BH"
    #define DEVICE              "ONOFRE"

    // Buttons
    #define BUTTON1_PIN         12
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON1_RELAY       1
    #define BUTTON2_PIN         13
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_PIN          5
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

// -----------------------------------------------------------------------------
// Several boards under different names uing a power chip labelled BL0937 or HJL-01
// * Blitzwolf (https://www.amazon.es/Inteligente-Temporización-Dispositivos-Cualquier-BlitzWolf/dp/B07BMQP142)
// * HomeCube (https://www.amazon.de/Steckdose-Homecube-intelligente-Verbrauchsanzeige-funktioniert/dp/B076Q2LKHG)
// * Coosa (https://www.amazon.com/COOSA-Monitoring-Function-Campatible-Assiatant/dp/B0788W9TDR)
// * Goosund (http://www.gosund.com/?m=content&c=index&a=show&catid=6&id=5)
// * Ablue (https://www.amazon.de/Intelligente-Steckdose-Ablue-Funktioniert-Assistant/dp/B076DRFRZC)
// -----------------------------------------------------------------------------

#elif defined(BLITZWOLF_BWSHP2)

    // Info
    #define MANUFACTURER                "BLITZWOLF"
    #define DEVICE                      "BWSHP2"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_MODE                BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    0
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// Same as the above but new board version marked V2.3
// -----------------------------------------------------------------------------

#elif defined(BLITZWOLF_BWSHP2_V23)

    // Info
    #define MANUFACTURER                "BLITZWOLF"
    #define DEVICE                      "BWSHP2V2.3"

    // Buttons
    #define BUTTON1_PIN                 3
    #define BUTTON1_MODE                BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  14
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    1
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    13
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             5
    #define HLW8012_CF_PIN              4

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// Teckin SP22 v1.4 - v1.6
// -----------------------------------------------------------------------------

#elif defined(TECKIN_SP22_V14)

    // Info
    #define MANUFACTURER                "TECKIN"
    #define DEVICE                      "SP22_V14"

    // Buttons
    #define BUTTON1_PIN                 1
    #define BUTTON1_MODE                BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  14
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    3
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    13
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             5
    #define HLW8012_CF_PIN              4

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       20730
    #define HLW8012_VOLTAGE_RATIO       264935
    #define HLW8012_POWER_RATIO         2533110
    #define HLW8012_INTERRUPT_ON        FALLING

// ----------------------------------------------------------------------------------------
//  Homecube 16A is similar but some pins differ and it also has RGB LEDs
//  https://www.amazon.de/gp/product/B07D7RVF56/ref=oh_aui_detailpage_o00_s01?ie=UTF8&psc=1
// ----------------------------------------------------------------------------------------

#elif defined(HOMECUBE_16A)

    // Info
    #define MANUFACTURER                "HOMECUBE"
    #define DEVICE                      "16A"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_MODE                BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    //LED Pin 4 - ESP8266 onboard LED
    //Red   LED: 0
    //Green LED: 12
    //Blue  LED: 2

    // Blue
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            0

    // Green
    #define LED2_PIN                    12
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_RELAY

    // Red
    #define LED3_PIN                    0
    #define LED3_PIN_INVERSE            0
    #define LED3_MODE                   LED_MODE_OFF

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             16
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// VANZAVANZU Smart Outlet Socket (based on BL0937 or HJL-01)
// https://www.amazon.com/Smart-Plug-Wifi-Mini-VANZAVANZU/dp/B078PHD6S5
// -----------------------------------------------------------------------------

#elif defined(VANZAVANZU_SMART_WIFI_PLUG_MINI)

    // Info
    #define MANUFACTURER                "VANZAVANZU"
    #define DEVICE                      "SMART_WIFI_PLUG_MINI"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_MODE                BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN                    2
    #define LED1_PIN_INVERSE            1
    #define LED2_PIN                    0
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_FINDME
    #define LED2_RELAY                  1

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT        0

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             3
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------

#elif defined(GENERIC_AG_L4)

    // Info
    #define MANUFACTURER                "GENERIC"
    #define DEVICE                      "AG_L4"
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT           1

    // button 1: "power" button
    #define BUTTON1_PIN                 4
    #define BUTTON1_RELAY               1
    #define BUTTON1_MODE                BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_PRESS               BUTTON_MODE_TOGGLE
    #define BUTTON1_CLICK               BUTTON_MODE_NONE
    #define BUTTON1_DBLCLICK            BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK            BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK         BUTTON_MODE_RESET

    // button 2: "wifi" button
    #define BUTTON2_PIN                 2
    #define BUTTON2_MODE                BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_PRESS               BUTTON_MODE_TOGGLE
    #define BUTTON2_CLICK               BUTTON_MODE_NONE
    #define BUTTON2_DBLCLICK            BUTTON_MODE_NONE
    #define BUTTON2_LNGCLICK            BUTTON_MODE_NONE
    #define BUTTON2_LNGLNGCLICK         BUTTON_MODE_NONE

    // LEDs
    #define LED1_PIN                    5      // red status led
    #define LED1_PIN_INVERSE            0

    #define LED2_PIN                    16      // master light power
    #define LED2_PIN_INVERSE            1
    #define LED2_MODE                   LED_MODE_RELAY

    // Light
    #define LIGHT_CHANNELS              3
    #define LIGHT_CH1_PIN               14       // RED
    #define LIGHT_CH2_PIN               13       // GREEN
    #define LIGHT_CH3_PIN               12      // BLUE
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0

// -----------------------------------------------------------------------------

#elif defined(ALLTERCO_SHELLY1)

    // Info
    #define MANUFACTURER        "ALLTERCO"
    #define DEVICE              "SHELLY1"

    // Buttons
    #define BUTTON1_PIN         5
    #define BUTTON1_MODE        BUTTON_SWITCH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

#elif defined(ALLTERCO_SHELLY2)

    // Info
    #define MANUFACTURER        "ALLTERCO"
    #define DEVICE              "SHELLY2"

    // Buttons
    #define BUTTON1_PIN         12
    #define BUTTON2_PIN         14
    #define BUTTON1_MODE        BUTTON_SWITCH
    #define BUTTON2_MODE        BUTTON_SWITCH
    #define BUTTON1_RELAY       1
    #define BUTTON2_RELAY       2

    // Relays
    #define RELAY1_PIN          4
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_PIN          5
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

// -----------------------------------------------------------------------------

#elif defined(LOHAS_9W)

    // Info
    #define MANUFACTURER        "LOHAS"
    #define DEVICE              "E27_9W"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      5
    #define MY92XX_MODEL        MY92XX_MODEL_MY9231
    #define MY92XX_CHIPS        2
    #define MY92XX_DI_PIN       13
    #define MY92XX_DCKI_PIN     15
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_MAPPING      0, 1, 2, 3, 4
    #define LIGHT_WHITE_FACTOR  (0.1)                    // White LEDs are way more bright in the B1

// -----------------------------------------------------------------------------

#elif defined(XIAOMI_SMART_DESK_LAMP)

    // Info
    #define MANUFACTURER        "XIAOMI"
    #define DEVICE              "SMART_DESK_LAMP"

    // Buttons
    #define BUTTON1_PIN         2
    #define BUTTON2_PIN         14

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH | BUTTON_SET_PULLUP

    // This button doubles as switch here and as encoder mode switch below
    // Clicking it (for less than 500ms) will turn the light on and off
    // Double and Long clicks will not work as these are used to modify the encoder action
    #define BUTTON1_RELAY           1
    #define BUTTON_LNGCLICK_DELAY   500
    #define BUTTON1_DBLCLICK        BUTTON_MODE_NONE
    #define BUTTON1_LNGCLICK        BUTTON_MODE_NONE
    #define BUTTON1_LNGLNGCLICK     BUTTON_MODE_NONE

    // Hidden button will enter AP mode if dblclick and reset the device when long-long-clicked
    #define BUTTON2_DBLCLICK        BUTTON_MODE_AP
    #define BUTTON2_LNGLNGCLICK     BUTTON_MODE_RESET

    // Light
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1
    #define LIGHT_STEP          8
    #define LIGHT_CHANNELS      2
    #define LIGHT_CH1_PIN       5   // warm white
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_PIN       4   // cold white
    #define LIGHT_CH2_INVERSE   0

    // Encoder
    // If mode is ENCODER_MODE_RATIO, the value ratio between both channels is changed
    // when the button is not pressed, and the overall brightness when pressed
    // If mode is ENCODER_MODE_CHANNEL, the first channel value is changed
    // when the button is not pressed, and the second channel when pressed
    // If no ENCODERX_BUTTON_PIN defined it will only change the value of the first defined channel
    #define ENCODER_SUPPORT     1
    #define ENCODER1_PIN1       12
    #define ENCODER1_PIN2       13
    #define ENCODER1_BUTTON_PIN 2   // active low by default, with software pullup
    #define ENCODER1_CHANNEL1   0   // please note this value is 0-based (LIGHT_CH1 above)
    #define ENCODER1_CHANNEL2   1   // please note this value is 0-based (LIGHT_CH2 above)
    #define ENCODER1_MODE       ENCODER_MODE_RATIO

#elif defined(PHYX_ESP12_RGB)

    // Info
    #define MANUFACTURER        "PHYX"
    #define DEVICE              "ESP12_RGB"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      3
    #define LIGHT_CH1_PIN       4       // RED
    #define LIGHT_CH2_PIN       14      // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0

// -----------------------------------------------------------------------------
// iWoole LED Table Lamp
// http://iwoole.com/newst-led-smart-night-light-7w-smart-table-light-rgbw-wifi-app-remote-control-110v-220v-us-eu-plug-smart-lamp-google-home-decore-p00022p1.html
// -----------------------------------------------------------------------------

#elif defined(IWOOLE_LED_TABLE_LAMP)

    // Info
    #define MANUFACTURER        "IWOOLE"
    #define DEVICE              "LED_TABLE_LAMP"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       12      // RED
    #define LIGHT_CH2_PIN       5       // GREEN
    #define LIGHT_CH3_PIN       14      // BLUE
    #define LIGHT_CH4_PIN       4       // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

// -----------------------------------------------------------------------------
// Lombex Lux Nova 2 Tunable White
// https://www.amazon.com/Lombex-Compatible-Equivalent-Dimmable-2700K-6500K/dp/B07B8K72PR
// -----------------------------------------------------------------------------
#elif defined(LOMBEX_LUX_NOVA2_TUNABLE_WHITE)

    // Info
    #define MANUFACTURER        "LOMBEX"
    #define DEVICE              "LUX_NOVA2_TUNABLE_WHITE"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      5
    #define MY92XX_MODEL        MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS        1
    #define MY92XX_DI_PIN       4
    #define MY92XX_DCKI_PIN     5
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    // No RGB on this bulb. Warm white on channel 0, cool white on channel 3
    #define MY92XX_MAPPING      255, 255, 255, 3, 0

// -----------------------------------------------------------------------------
// Lombex Lux Nova 2 White and Color
// https://www.amazon.com/Lombex-Compatible-Equivalent-Dimmable-2700K-6500K/dp/B07B8K72PR
// -----------------------------------------------------------------------------
#elif defined(LOMBEX_LUX_NOVA2_WHITE_COLOR)

    // Info
    #define MANUFACTURER        "LOMBEX"
    #define DEVICE              "LUX_NOVA2_WHITE_COLOR"
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX
    #define DUMMY_RELAY_COUNT   1

    // Light
    #define LIGHT_CHANNELS      4
    #define MY92XX_MODEL        MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS        1
    #define MY92XX_DI_PIN       4
    #define MY92XX_DCKI_PIN     5
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    // RGB on channels 0/1/2, either cool or warm white on channel 3
    // The bulb *should* have cool leds, but could also have warm leds as a common defect
    #define MY92XX_MAPPING      0, 1, 2, 3

// -----------------------------------------------------------------------------
// Bestek Smart Plug with 2 USB ports
// https://www.bestekcorp.com/bestek-smart-plug-works-with-amazon-alexa-google-assistant-and-ifttt-with-2-usb
// -----------------------------------------------------------------------------

#elif defined(BESTEK_MRJ1011)

    // Info
    #define MANUFACTURER        "BESTEK"
    #define DEVICE              "MRJ1011"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_SET_PULLUP | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relay
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LED
    #define LED1_PIN            4
    #define LED1_PIN_INVERSE    1

// -----------------------------------------------------------------------------
// GBLIFE RGBW SOCKET
// -----------------------------------------------------------------------------

#elif defined(GBLIFE_RGBW_SOCKET)

    // Info
    #define MANUFACTURER        "GBLIFE"
    #define DEVICE              "RGBW_SOCKET"

    // Buttons
    #define BUTTON1_PIN         13
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Relays
    #define RELAY1_PIN          15
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL 
 
    // Light RGBW 
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1

    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       14      // GREEN
    #define LIGHT_CH3_PIN       12      // BLUE
    #define LIGHT_CH4_PIN       4       // WHITE
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0	
    
// ----------------------------------------------------------------------------------------
//  Smart life Mini Smart Socket is similar Homecube 16A but some GPIOs differ
//  https://www.ebay.de/itm/Smart-Steckdose-WIFI-WLAN-Amazon-Alexa-Fernbedienung-Home-Socket-Zeitschaltuh-DE/123352026749?hash=item1cb85a8e7d:g:IasAAOSwk6dbj390
// ----------------------------------------------------------------------------------------

#elif defined(SMARTLIFE_MINI_SMART_SOCKET)

    // Info
    #define MANUFACTURER                "SMARTLIFE"
    #define DEVICE                      "MINI_SMART_SOCKET"

    // Buttons
    #define BUTTON1_PIN                 13
    #define BUTTON1_MODE                BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY               1

    // Relays
    #define RELAY1_PIN                  15
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL

    // LEDs
    //Red   LED: 0
    //Green LED: 4
    //Blue  LED: 2

    // Light
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT           1
    #define LIGHT_CHANNELS              3
    #define LIGHT_CH1_PIN               0       // RED
    #define LIGHT_CH2_PIN               4       // GREEN
    #define LIGHT_CH3_PIN               2       // BLUE
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5

    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// -----------------------------------------------------------------------------
// TEST boards (do not use!!)
// -----------------------------------------------------------------------------

#elif defined(TRAVIS01)

    // Info
    #define MANUFACTURER            "TravisCI"
    #define DEVICE                  "Virtual board 01"

    // Some buttons - pin 0
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // Some relays - pin 1
    #define RELAY1_PIN          1
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // Some LEDs - pin 2
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

    // A bit of I2C - pins 3,4
    #define I2C_SDA_PIN         3
    #define I2C_SCL_PIN         4

    // And, as they say in "From Dusk till Dawn":
    // This is a sensor blow out!
    // Alright, we got white sensor, black sensor, spanish sensor, yellow sensor. We got hot sensor, cold sensor.
    // We got wet sensor. We got smelly sensor. We got hairy sensor, bloody sensor. We got snapping sensor.
    // We got silk sensor, velvet sensor, naugahyde sensor. We even got horse sensor, dog sensor, chicken sensor.
    // C'mon, you want sensor, come on in sensor lovers!
    // If we don’t got it, you don't want it!
    #define AM2320_SUPPORT        1
    #define BH1750_SUPPORT        1
    #define BMP180_SUPPORT        1
    #define BMX280_SUPPORT        1
    #define SHT3X_I2C_SUPPORT     1
    #define EMON_ADC121_SUPPORT   1
    #define EMON_ADS1X15_SUPPORT  1
    #define SHT3X_I2C_SUPPORT     1
    #define SI7021_SUPPORT        1
    #define PMSX003_SUPPORT       1
    #define SENSEAIR_SUPPORT      1
    #define VL53L1X_SUPPORT       1

    // A bit of lights - pin 5
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1
    #define LIGHT_CHANNELS      1
    #define LIGHT_CH1_PIN       5
    #define LIGHT_CH1_INVERSE   0

    // A bit of HLW8012 - pins 6,7,8
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     6
    #define HLW8012_CF1_PIN     7
    #define HLW8012_CF_PIN      8

    // A bit of Dallas - pin 9
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT      1
    #endif
    #define DALLAS_PIN          9

    // A bit of ECH1560 - pins 10,11, 12
    #ifndef ECH1560_SUPPORT
    #define ECH1560_SUPPORT     1
    #endif
    #define ECH1560_CLK_PIN     10
    #define ECH1560_MISO_PIN    11
    #define ECH1560_INVERTED    12

    // MICS-2710 & MICS-5525 test
    #define MICS2710_SUPPORT    1
    #define MICS5525_SUPPORT    1

#elif defined(TRAVIS02)

    // Relay provider dual
    #define MANUFACTURER            "TravisCI"
    #define DEVICE                  "Virtual board 02"

    // Some buttons - pin 0
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // A bit of CSE7766 - pin 1
    #ifndef CSE7766_SUPPORT
    #define CSE7766_SUPPORT     1
    #endif
    #define CSE7766_PIN         1

    // Relay type dual  - pins 2,3
    #define RELAY_PROVIDER      RELAY_PROVIDER_DUAL
    #define RELAY1_PIN          2
    #define RELAY2_PIN          3
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // IR - pin 4
    #define IR_SUPPORT          1
    #define IR_RX_PIN           4
    #define IR_BUTTON_SET       1

    // A bit of DHT - pin 5
    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT         1
    #endif
    #define DHT_PIN             5

    // A bit of TMP3X (analog)
    #define TMP3X_SUPPORT       1

    // A bit of EVENTS - pin 10
    #define EVENTS_SUPPORT      1
    #define EVENTS_PIN          6

    // Sonar
    #define SONAR_SUPPORT       1
    #define SONAR_TRIGGER       7
    #define SONAR_ECHO          8

    // MHZ19
    #define MHZ19_SUPPORT       1
    #define MHZ19_RX_PIN        9
    #define MHZ19_TX_PIN        10

    // PZEM004T
    #define PZEM004T_SUPPORT    1
    #define PZEM004T_RX_PIN     11
    #define PZEM004T_TX_PIN     12

    // V9261F
    #define V9261F_SUPPORT      1
    #define V9261F_PIN          13

    // GUVAS12SD
    #define GUVAS12SD_SUPPORT   1
    #define GUVAS12SD_PIN       14

    // Test non-default modules
    #define MDNS_CLIENT_SUPPORT 1
    #define NOFUSS_SUPPORT      1
    #define UART_MQTT_SUPPORT   1
    #define INFLUXDB_SUPPORT    1
    #define IR_SUPPORT    1

#elif defined(TRAVIS03)

    // Relay provider light/my92XX
    #define MANUFACTURER            "TravisCI"
    #define DEVICE                  "Virtual board 03"

    // Some buttons - pin 0
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON1_RELAY       1

    // MY9231 Light - pins 1,2
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX
    #define DUMMY_RELAY_COUNT   1
    #define LIGHT_CHANNELS      5
    #define MY92XX_MODEL        MY92XX_MODEL_MY9231
    #define MY92XX_CHIPS        2
    #define MY92XX_DI_PIN       1
    #define MY92XX_DCKI_PIN     2
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_MAPPING      4, 3, 5, 0, 1

    // A bit of Analog EMON (analog)
    #ifndef EMON_ANALOG_SUPPORT
    #define EMON_ANALOG_SUPPORT 1
    #endif

    #define PULSEMETER_SUPPORT  1

    // Test non-default modules
    #define LLMNR_SUPPORT       1
    #define NETBIOS_SUPPORT     1
    #define SSDP_SUPPORT        1

#endif

// -----------------------------------------------------------------------------
// Check definitions
// -----------------------------------------------------------------------------

#if not defined(MANUFACTURER) || not defined(DEVICE)
    #error "UNSUPPORTED HARDWARE!!"
#endif
