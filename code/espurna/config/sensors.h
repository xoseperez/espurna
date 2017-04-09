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
#define DHT_UPDATE_INTERVAL     60000
#define DHT_TYPE                DHT22
#define DHT_TIMING              11
#define DHT_TEMPERATURE_TOPIC   "temperature"
#define DHT_HUMIDITY_TOPIC      "humidity"

#define HUMIDITY_NORMAL         0
#define HUMIDITY_COMFORTABLE    1
#define HUMIDITY_DRY            2
#define HUMIDITY_WET            3

//--------------------------------------------------------------------------------
// Analog sensor
// Enable support by passing ENABLE_ANALOG=1 build flag
//--------------------------------------------------------------------------------

#define ANALOG_PIN              0
#define ANALOG_UPDATE_INTERVAL  60000
#define ANALOG_TOPIC            "analog"

#if ENABLE_ANALOG
    #undef ENABLE_ADC_VCC
    #define ENABLE_ADC_VCC      0
#endif

//--------------------------------------------------------------------------------
// DS18B20 temperature sensor
// Enable support by passing ENABLE_DS18B20=1 build flag
//--------------------------------------------------------------------------------

#define DS_PIN                  14
#define DS_UPDATE_INTERVAL      60000
#define DS_TEMPERATURE_TOPIC    "temperature"

//--------------------------------------------------------------------------------
// Custom current sensor
// Check http://tinkerman.cat/your-laundry-is-done/
// Check http://tinkerman.cat/power-monitoring-sonoff-th-adc121/
// Enable support by passing ENABLE_EMON=1 build flag
//--------------------------------------------------------------------------------

#define EMON_ANALOG_PROVIDER    0
#define EMON_ADC121_PROVIDER    1

// If you select EMON_ADC121_PROVIDER you need to enable and configure I2C in general.h
#define EMON_PROVIDER           EMON_ANALOG_PROVIDER

#if EMON_PROVIDER == EMON_ANALOG_PROVIDER
    #define EMON_CURRENT_PIN        0
	#define EMON_ADC_BITS           10
	#define EMON_REFERENCE_VOLTAGE  1.0
    #define EMON_CURRENT_PRECISION  1
    #define EMON_CURRENT_OFFSET     0.25
    #if ENABLE_EMON
        #undef ENABLE_ADC_VCC
		#define ENABLE_ADC_VCC      0
    #endif
#endif

#if EMON_PROVIDER == EMON_ADC121_PROVIDER
    #define EMON_ADC121_ADDRESS     0x50
	#define EMON_ADC_BITS           12
	#define EMON_REFERENCE_VOLTAGE  3.3
    #define EMON_CURRENT_PRECISION  2
    #define EMON_CURRENT_OFFSET     0.10
#endif

#define EMON_CURRENT_RATIO      30
#define EMON_SAMPLES            1000
#define EMON_INTERVAL           10000
#define EMON_MEASUREMENTS       6
#define EMON_MAINS_VOLTAGE      230
#define EMON_APOWER_TOPIC       "apower"
#define EMON_ENERGY_TOPIC       "energy"
#define EMON_CURRENT_TOPIC      "current"

//--------------------------------------------------------------------------------
// HLW8012 power sensor (Sonoff POW)
// Enable support by passing ENABLE_POW=1 build flag
// Enabled by default when selecting SONOFF_POW hardware
//--------------------------------------------------------------------------------

#ifdef ESPURNA_H
    #define POW_SEL_PIN         2
#else
    #define POW_SEL_PIN         5
#endif
#define POW_CF1_PIN             13
#define POW_CF_PIN              14

#define POW_USE_INTERRUPTS      1
#define POW_SEL_CURRENT         HIGH
#define POW_CURRENT_R           0.001
#define POW_VOLTAGE_R_UP        ( 5 * 470000 ) // Real: 2280k
#define POW_VOLTAGE_R_DOWN      ( 1000 ) // Real 1.009k
#define POW_POWER_TOPIC         "power"
#define POW_CURRENT_TOPIC       "current"
#define POW_VOLTAGE_TOPIC       "voltage"
#define POW_APOWER_TOPIC        "apower"
#define POW_RPOWER_TOPIC        "rpower"
#define POW_PFACTOR_TOPIC       "pfactor"
#define POW_ENERGY_TOPIC        "energy"
#define POW_UPDATE_INTERVAL     5000
#define POW_REPORT_EVERY        12
#define POW_MIN_POWER           5
#define POW_MAX_POWER           2500
#define POW_MIN_CURRENT         0.05
#define POW_MAX_CURRENT         10

//--------------------------------------------------------------------------------
// Internal power montior
// Enable support by passing ENABLE_ADC_VCC=1 build flag
// Do not enable this if using the analog GPIO for any other thing
//--------------------------------------------------------------------------------

#ifndef ENABLE_ADC_VCC
#define ENABLE_ADC_VCC       1
#endif

#if ENABLE_ADC_VCC
    ADC_MODE(ADC_VCC);
#endif
