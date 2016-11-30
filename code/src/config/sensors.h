//--------------------------------------------------------------------------------
// Custom RF module
// Check http://tinkerman.cat/adding-rf-to-a-non-rf-itead-sonoff/
// Enable support by passing ENABLE_RF=1 build flag
//--------------------------------------------------------------------------------

#define RF_PIN                  14
#define RF_CHANNEL              31
#define RF_DEVICE               1

//--------------------------------------------------------------------------------
// DHTXX temperature/humidity sensor
// Enable support by passing ENABLE_DHT=1 build flag
//--------------------------------------------------------------------------------

#define DHT_PIN                 14
#define DHT_UPDATE_INTERVAL     300000
#define DHT_TYPE                DHT22
#define DHT_TIMING              11
#define DHT_TEMPERATURE_TOPIC   "/temperature"
#define DHT_HUMIDITY_TOPIC      "/humidity"

//--------------------------------------------------------------------------------
// DS18B20 temperature sensor
// Enable support by passing ENABLE_DS18B20=1 build flag
//--------------------------------------------------------------------------------

#define DS_PIN                  14
#define DS_UPDATE_INTERVAL      60000
#define DS_TEMPERATURE_TOPIC    "/temperature"

//--------------------------------------------------------------------------------
// Custom current sensor
// Check http://tinkerman.cat/your-laundry-is-done/
// Enable support by passing ENABLE_EMON=1 build flag
//--------------------------------------------------------------------------------

#define EMON_CURRENT_PIN        0
#define EMON_SAMPLES            1000
#define EMON_INTERVAL           10000
#define EMON_MEASUREMENTS       6
#define EMON_ADC_BITS           10
#define EMON_REFERENCE_VOLTAGE  1.0
#define EMON_CURRENT_PRECISION  1
#define EMON_CURRENT_OFFSET     0.25
#define EMON_MAINS_VOLTAGE      230
#define EMON_CURRENT_RATIO      180
#define EMON_POWER_TOPIC        "/power"

//--------------------------------------------------------------------------------
// HLW8012 power sensor (Sonoff POW)
// Enable support by passing ENABLE_POW=1 build flag
// Enabled by default when selecting SONOFF_POW hardware
//--------------------------------------------------------------------------------

#define POW_SEL_PIN             5
#define POW_CF1_PIN             13
#define POW_CF_PIN              14
#define POW_SEL_CURRENT         HIGH
#define POW_CURRENT_R           0.001
#define POW_VOLTAGE_R_UP        ( 5 * 470000 ) // Real: 2280k
#define POW_VOLTAGE_R_DOWN      ( 1000 ) // Real 1.009k
#define POW_POWER_TOPIC         "/power"
#define POW_UPDATE_INTERVAL     10000
#define POW_REPORT_EVERY        6
