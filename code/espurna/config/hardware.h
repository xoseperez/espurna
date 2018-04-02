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
// LED#_MODE: Check general.h for LED_MODE_%
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
    #define DOMOTICZ_SUPPORT        0
    #define HOMEASSISTANT_SUPPORT   0
    #define I2C_SUPPORT             0
    #define MQTT_SUPPORT            0
    #define NTP_SUPPORT             0
    #define SCHEDULER_SUPPORT       0
    #define SENSOR_SUPPORT          0
    #define THINGSPEAK_SUPPORT      0
    #define WEB_SUPPORT             0

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

    // Relays
    #define RELAY1_PIN          12
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN            2
    #define LED1_PIN_INVERSE    1

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

    // LEDs
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

#elif defined(ITEAD_SONOFF_DUAL)

    // Info
    #define MANUFACTURER            "ITEAD"
    #define DEVICE                  "SONOFF_DUAL"
    #define SERIAL_BAUDRATE         19230
    #define RELAY_PROVIDER          RELAY_PROVIDER_DUAL
    #define DUMMY_RELAY_COUNT       2
    #define DEBUG_SERIAL_SUPPORT    0
    #define TERMINAL_SUPPORT        0

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
    #define SERIAL_BAUDRATE     19200
    #define RELAY_PROVIDER      RELAY_PROVIDER_RFBRIDGE

    #ifndef DUMMY_RELAY_COUNT
    #define DUMMY_RELAY_COUNT   8
    #endif

    // Remove UART noise on serial line
    #define TERMINAL_SUPPORT        0
    #define DEBUG_SERIAL_SUPPORT    0

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

    // LEDs
    #define LED1_PIN            13
    #define LED1_PIN_INVERSE    1

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

// -----------------------------------------------------------------------------
// YJZK
// -----------------------------------------------------------------------------

#elif defined(YJZK_SWITCH_2CH)

    // Info
    #define MANUFACTURER        "YJZK"
    #define DEVICE              "SWITCH_2CH"

    // Buttons
    #define BUTTON1_PIN         0
    #define BUTTON2_PIN         9

    #define BUTTON1_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH
    #define BUTTON2_MODE        BUTTON_PUSHBUTTON | BUTTON_DEFAULT_HIGH

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
    #define IR_PIN              4
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
    #define IR_PIN              4
    #define IR_BUTTON_SET       1

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

    // DB18B20
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
    #define LIGHT_CHANNELS      4
    #define LIGHT_CH1_PIN       5       // RED
    #define LIGHT_CH2_PIN       12      // GREEN
    #define LIGHT_CH3_PIN       13      // BLUE
    #define LIGHT_CH4_PIN       14      // WHITE1
    #define LIGHT_CH1_INVERSE   0
    #define LIGHT_CH2_INVERSE   0
    #define LIGHT_CH3_INVERSE   0
    #define LIGHT_CH4_INVERSE   0

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
    #define LED1_PIN_INVERSE    0

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     12
    #define HLW8012_CF1_PIN     5
    #define HLW8012_CF_PIN      4

    #define HLW8012_VOLTAGE_R_UP            ( 2 * 1000000 )  // Upstream voltage resistor

// -----------------------------------------------------------------------------
// Euromate (?) Wifi Stecker Shuko
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
    #define TERMINAL_SUPPORT        0
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
// YiDian XS-SSA05
// -----------------------------------------------------------------------------

#elif defined(YIDIAN_XSSSA05)

    // Info
    #define MANUFACTURER		"YIDIAN"
    #define DEVICE				"XSSSA05"

    // Buttons
    #define BUTTON1_PIN			13
    #define BUTTON1_MODE		BUTTON_PUSHBUTTON
    #define BUTTON1_RELAY		1

    // Relays
    #define RELAY1_PIN			12
    #define RELAY1_TYPE			RELAY_TYPE_NORMAL

    // LEDs
    #define LED1_PIN			4
    #define LED1_PIN_INVERSE	0

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     3
    #define HLW8012_CF1_PIN     14
    #define HLW8012_CF_PIN      5

    #define HLW8012_CURRENT_R               0.001            // Current resistor
    #define HLW8012_VOLTAGE_R_UP            ( 2 * 1200000 )  // Upstream voltage resistor

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
    #define BUTTON3_RELAY           2
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
    #define BH1750_SUPPORT        1
    #define BMX280_SUPPORT        1
    #define SHT3X_I2C_SUPPORT     1
    #define EMON_ADC121_SUPPORT   1
    #define EMON_ADS1X15_SUPPORT  1
    #define SHT3X_I2C_SUPPORT     1
    #define SI7021_SUPPORT        1


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

#elif defined(TRAVIS02)

    // Relay provider dual
    #define MANUFACTURER            "TravisCI"
    #define DEVICE                  "Virtual board 02"

    // A bit of DHT - pin 1
    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT         1
    #endif
    #define DHT_PIN             1

    // Relay type dual  - pins 2,3
    #define RELAY_PROVIDER      RELAY_PROVIDER_DUAL
    #define RELAY1_PIN          2
    #define RELAY2_PIN          3
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // IR - pin 4
    #define IR_SUPPORT          1
    #define IR_PIN              4
    #define IR_BUTTON_SET       1

#elif defined(TRAVIS03)

    // Relay provider light/my92XX
    #define MANUFACTURER            "TravisCI"
    #define DEVICE                  "Virtual board 03"

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

#endif

// -----------------------------------------------------------------------------
// Check definitions
// -----------------------------------------------------------------------------

#if not defined(MANUFACTURER) || not defined(DEVICE)
    #error "UNSUPPORTED HARDWARE!!"
#endif
