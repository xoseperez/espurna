// =============================================================================
// SENSORS - General data
// =============================================================================

#define SENSOR_DEBUG                        0               // Debug sensors

#define SENSOR_READ_INTERVAL                6               // Read data from sensors every 6 seconds
#define SENSOR_READ_MIN_INTERVAL            1               // Minimum read interval
#define SENSOR_READ_MAX_INTERVAL            3600            // Maximum read interval
#define SENSOR_INIT_INTERVAL                10000           // Try to re-init non-ready sensors every 10s

#define SENSOR_REPORT_EVERY                 10              // Report every this many readings
#define SENSOR_REPORT_MIN_EVERY             1               // Minimum every value
#define SENSOR_REPORT_MAX_EVERY             60              // Maximum

#define SENSOR_USE_INDEX                    0               // Use the index in topic (i.e. temperature/0)
                                                            // even if just one sensor (0 for backwards compatibility)

#ifndef SENSOR_POWER_CHECK_STATUS
#define SENSOR_POWER_CHECK_STATUS           1               // If set to 1 the reported power/current/energy will be 0 if the relay[0] is OFF
#endif

#ifndef SENSOR_TEMPERATURE_CORRECTION
#define SENSOR_TEMPERATURE_CORRECTION       0.0             // Offset correction
#endif

#ifndef TEMPERATURE_MIN_CHANGE
#define TEMPERATURE_MIN_CHANGE              0.0             // Minimum temperature change to report
#endif

#ifndef SENSOR_HUMIDITY_CORRECTION
#define SENSOR_HUMIDITY_CORRECTION          0.0             // Offset correction
#endif

#ifndef HUMIDITY_MIN_CHANGE
#define HUMIDITY_MIN_CHANGE                 0               // Minimum humidity change to report
#endif

#ifndef ENERGY_MAX_CHANGE
#define ENERGY_MAX_CHANGE                   0               // Maximum energy change to report (if >0 it will allways report when delta(E) is greater than this)
#endif

#ifndef SENSOR_SAVE_EVERY
#define SENSOR_SAVE_EVERY                   0               // Save accumulating values to EEPROM (atm only energy)
                                                            // A 0 means do not save and it's the default value
                                                            // A number different from 0 means it should store the value in EEPROM
                                                            // after these many reports
                                                            // Warning: this might wear out flash fast!
#endif

#define SENSOR_PUBLISH_ADDRESSES            0               // Publish sensor addresses
#define SENSOR_ADDRESS_TOPIC                "address"       // Topic to publish sensor addresses


#ifndef SENSOR_TEMPERATURE_UNITS
#define SENSOR_TEMPERATURE_UNITS            TMP_CELSIUS     // Temperature units (TMP_CELSIUS | TMP_FAHRENHEIT)
#endif

#ifndef SENSOR_ENERGY_UNITS
#define SENSOR_ENERGY_UNITS                 ENERGY_JOULES   // Energy units (ENERGY_JOULES | ENERGY_KWH)
#endif

#ifndef SENSOR_POWER_UNITS
#define SENSOR_POWER_UNITS                  POWER_WATTS     // Power units (POWER_WATTS | POWER_KILOWATTS)
#endif


// =============================================================================
// Specific data for each sensor
// =============================================================================

//------------------------------------------------------------------------------
// AM2320 Humidity & Temperature sensor over I2C
// Enable support by passing AM2320_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef AM2320_SUPPORT
#define AM2320_SUPPORT                  0
#endif

#ifndef AM2320_ADDRESS
#define AM2320_ADDRESS                  0x00    // 0x00 means auto
#endif

//------------------------------------------------------------------------------
// Analog sensor
// Enable support by passing ANALOG_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef ANALOG_SUPPORT
#define ANALOG_SUPPORT                  0
#endif

#ifndef ANALOG_SAMPLES
#define ANALOG_SAMPLES                  10      // Number of samples
#endif

#ifndef ANALOG_DELAY
#define ANALOG_DELAY                    0       // Delay between samples in micros
#endif

//Use the following to perform scaling of raw analog values
//   scaledRead = ( factor * rawRead ) + offset
//
//Please take note that the offset is not affected by the scaling factor

#ifndef ANALOG_FACTOR
#define ANALOG_FACTOR                    1.0       // Multiply raw reading by this factor
#endif

#ifndef ANALOG_OFFSET
#define ANALOG_OFFSET                    0.0       // Add this offset to *scaled* value
#endif

// Round to this number of decimals
#ifndef ANALOG_DECIMALS
#define ANALOG_DECIMALS                  2
#endif


//------------------------------------------------------------------------------
// BH1750
// Enable support by passing BH1750_SUPPORT=1 build flag
// http://www.elechouse.com/elechouse/images/product/Digital%20light%20Sensor/bh1750fvi-e.pdf
//------------------------------------------------------------------------------

#ifndef BH1750_SUPPORT
#define BH1750_SUPPORT                  0
#endif

#ifndef BH1750_ADDRESS
#define BH1750_ADDRESS                  0x00    // 0x00 means auto
#endif

#define BH1750_MODE                     BH1750_CONTINUOUS_HIGH_RES_MODE

//------------------------------------------------------------------------------
// BMP085/BMP180
// Enable support by passing BMP180_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef BMP180_SUPPORT
#define BMP180_SUPPORT                  0
#endif

#ifndef BMP180_ADDRESS
#define BMP180_ADDRESS                  0x00    // 0x00 means auto
#endif

#define BMP180_MODE                     3       // 0 for ultra-low power, 1 for standard, 2 for high resolution and 3 for ultrahigh resolution

//------------------------------------------------------------------------------
// BME280/BMP280
// Enable support by passing BMX280_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef BMX280_SUPPORT
#define BMX280_SUPPORT                  0
#endif

#ifndef BMX280_NUMBER
#define BMX280_NUMBER                   1       // Number of sensors present. Either 1 or 2 allowed
#endif
#ifndef BMX280_ADDRESS
#define BMX280_ADDRESS                  0x00    // 0x00 means auto (0x76 or 0x77 allowed) for sensor #0
#endif                                          // If (BMX280_NUMBER == 2) and
                                                //   (BMX280_ADDRESS == 0x00) then sensor #1 is auto-discovered
                                                //   (BMX280_ADDRESS != 0x00) then sensor #1 is the unnamed address

#define BMX280_MODE                     1       // 0 for sleep mode, 1 or 2 for forced mode, 3 for normal mode
#define BMX280_STANDBY                  0       // 0 for 0.5ms, 1 for 62.5ms, 2 for 125ms
                                                // 3 for 250ms, 4 for 500ms, 5 for 1000ms
                                                // 6 for 10ms, 7 for 20ms
#define BMX280_FILTER                   0       // 0 for OFF, 1 for 2 values, 2 for 4 values, 3 for 8 values and 4 for 16 values
#define BMX280_TEMPERATURE              1       // Oversampling for temperature (set to 0 to disable magnitude)
                                                // 0b000 = 0 = Skip measurement
                                                // 0b001 = 1 = 1x 16bit/0.0050C resolution
                                                // 0b010 = 2 = 2x 17bit/0.0025C
                                                // 0b011 = 3 = 4x 18bit/0.0012C
                                                // 0b100 = 4 = 8x 19bit/0.0006C
                                                // 0b101 = 5 = 16x 20bit/0.0003C
#define BMX280_HUMIDITY                 1       // Oversampling for humidity (set to 0 to disable magnitude, only for BME280)
                                                // 0b000 = 0 = Skip measurement
                                                // 0b001 = 1 = 1x 0.07% resolution
                                                // 0b010 = 2 = 2x 0.05%
                                                // 0b011 = 3 = 4x 0.04%
                                                // 0b100 = 4 = 8x 0.03%
                                                // 0b101 = 5 = 16x 0.02%
#define BMX280_PRESSURE                 1       // Oversampling for pressure (set to 0 to disable magnitude)
                                                // 0b000 = 0 = Skipped
                                                // 0b001 = 1 = 1x 16bit/2.62 Pa resolution
                                                // 0b010 = 2 = 2x 17bit/1.31 Pa
                                                // 0b011 = 3 = 4x 18bit/0.66 Pa
                                                // 0b100 = 4 = 8x 19bit/0.33 Pa
                                                // 0b101 = 5 = 16x 20bit/0.16 Pa
  
//------------------------------------------------------------------------------
// Dallas OneWire temperature sensors
// Enable support by passing DALLAS_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef DALLAS_SUPPORT
#define DALLAS_SUPPORT                  0
#endif

#ifndef DALLAS_PIN
#define DALLAS_PIN                      14
#endif

#define DALLAS_RESOLUTION               9           // Not used atm
#define DALLAS_READ_INTERVAL            2000        // Force sensor read & cache every 2 seconds

//------------------------------------------------------------------------------
// DHTXX temperature/humidity sensor
// Enable support by passing DHT_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef DHT_SUPPORT
#define DHT_SUPPORT                     0
#endif

#ifndef DHT_PIN
#define DHT_PIN                         14
#endif

#ifndef DHT_TYPE
#define DHT_TYPE                        DHT_CHIP_DHT22
#endif

//------------------------------------------------------------------------------
// CSE7766 based power sensor
// Enable support by passing CSE7766_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef CSE7766_SUPPORT
#define CSE7766_SUPPORT                 0
#endif

#ifndef CSE7766_PIN
#define CSE7766_PIN                     1       // TX pin from the CSE7766
#endif

#ifndef CSE7766_PIN_INVERSE
#define CSE7766_PIN_INVERSE             0       // Signal is inverted
#endif

#define CSE7766_SYNC_INTERVAL           300     // Safe time between transmissions (ms)
#define CSE7766_BAUDRATE                4800    // UART baudrate

#define CSE7766_V1R                     1.0     // 1mR current resistor
#define CSE7766_V2R                     1.0     // 1M voltage resistor


//------------------------------------------------------------------------------
// Digital sensor
// Enable support by passing DIGITAL_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef DIGITAL_SUPPORT
#define DIGITAL_SUPPORT                 0
#endif

#ifndef DIGITAL_PIN
#define DIGITAL_PIN                     2
#endif

#ifndef DIGITAL_PIN_MODE
#define DIGITAL_PIN_MODE                INPUT_PULLUP
#endif

#ifndef DIGITAL_DEFAULT_STATE
#define DIGITAL_DEFAULT_STATE           1
#endif

//------------------------------------------------------------------------------
// ECH1560 based power sensor
// Enable support by passing ECH1560_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef ECH1560_SUPPORT
#define ECH1560_SUPPORT                 0
#endif

#ifndef ECH1560_CLK_PIN
#define ECH1560_CLK_PIN                 4       // CLK pin for the ECH1560
#endif

#ifndef ECH1560_MISO_PIN
#define ECH1560_MISO_PIN                5       // MISO pin for the ECH1560
#endif

#ifndef ECH1560_INVERTED
#define ECH1560_INVERTED                0       // Signal is inverted
#endif

//------------------------------------------------------------------------------
// Energy Monitor general settings
//------------------------------------------------------------------------------

#define EMON_MAX_SAMPLES                1000        // Max number of samples to get
#define EMON_MAX_TIME                   250         // Max time in ms to sample
#define EMON_FILTER_SPEED               512         // Mobile average filter speed
#define EMON_REFERENCE_VOLTAGE          3.3         // Reference voltage of the ADC

#ifndef EMON_MAINS_VOLTAGE
#define EMON_MAINS_VOLTAGE              230         // Mains voltage
#endif

#ifndef EMON_CURRENT_RATIO
#define EMON_CURRENT_RATIO              30          // Current ratio in the clamp (30A/1V)
#endif

#ifndef EMON_REPORT_CURRENT

#define EMON_REPORT_CURRENT             0           // Report current
#endif

#ifndef EMON_REPORT_POWER
#define EMON_REPORT_POWER               1           // Report power
#endif

#ifndef EMON_REPORT_ENERGY
#define EMON_REPORT_ENERGY              1           // Report energy
#endif

//------------------------------------------------------------------------------
// Energy Monitor based on ADC121
// Enable support by passing EMON_ADC121_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef EMON_ADC121_SUPPORT
#define EMON_ADC121_SUPPORT             0       // Do not build support by default
#endif

#define EMON_ADC121_I2C_ADDRESS         0x00    // 0x00 means auto

//------------------------------------------------------------------------------
// Energy Monitor based on ADS1X15
// Enable support by passing EMON_ADS1X15_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef EMON_ADS1X15_SUPPORT
#define EMON_ADS1X15_SUPPORT            0       // Do not build support by default
#endif

#define EMON_ADS1X15_I2C_ADDRESS        0x00    // 0x00 means auto
#define EMON_ADS1X15_TYPE               ADS1X15_CHIP_ADS1115
#define EMON_ADS1X15_GAIN               ADS1X15_REG_CONFIG_PGA_4_096V
#define EMON_ADS1X15_MASK               0x0F    // A0=1 A1=2 A2=4 A3=8

//------------------------------------------------------------------------------
// Energy Monitor based on interval analog GPIO
// Enable support by passing EMON_ANALOG_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef EMON_ANALOG_SUPPORT
#define EMON_ANALOG_SUPPORT             0       // Do not build support by default
#endif

//------------------------------------------------------------------------------
// Counter sensor
// Enable support by passing EVENTS_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef EVENTS_SUPPORT
#define EVENTS_SUPPORT                  0       // Do not build with counter support by default
#endif

#ifndef EVENTS_TRIGGER
#define EVENTS_TRIGGER                  1       // 1 to trigger callback on events,
                                                // 0 to only count them and report periodically
#endif

#ifndef EVENTS_PIN
#define EVENTS_PIN                      2       // GPIO to monitor
#endif

#ifndef EVENTS_PIN_MODE
#define EVENTS_PIN_MODE                 INPUT   // INPUT, INPUT_PULLUP
#endif

#ifndef EVENTS_INTERRUPT_MODE
#define EVENTS_INTERRUPT_MODE           RISING  // RISING, FALLING, CHANGE
#endif

#define EVENTS_DEBOUNCE                 50      // Do not register events within less than 50 millis

//------------------------------------------------------------------------------
// Geiger sensor
// Enable support by passing GEIGER_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef GEIGER_SUPPORT
#define GEIGER_SUPPORT                  0       // Do not build with geiger support by default
#endif

#ifndef GEIGER_PIN
#define GEIGER_PIN                      D1       // GPIO to monitor "D1" => "GPIO5"
#endif

#ifndef GEIGER_PIN_MODE
#define GEIGER_PIN_MODE                 INPUT   // INPUT, INPUT_PULLUP
#endif

#ifndef GEIGER_INTERRUPT_MODE
#define GEIGER_INTERRUPT_MODE           RISING  // RISING, FALLING, CHANGE
#endif

#define GEIGER_DEBOUNCE                 25      // Do not register events within less than 25 millis.
                                                // Value derived here: Debounce time 25ms, because https://github.com/Trickx/espurna/wiki/Geiger-counter

#define GEIGER_CPM2SIEVERT              240     // CPM to µSievert per hour conversion factor
                                                // Typically the literature uses the invers, but I find an integer type more convienient.
#define GEIGER_REPORT_SIEVERTS          1       // Enabler for local dose rate reports in µSv/h
#define GEIGER_REPORT_CPM               1       // Enabler for local dose rate reports in counts per minute

//------------------------------------------------------------------------------
// GUVAS12SD UV Sensor (analog)
// Enable support by passing GUVAS12SD_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef GUVAS12SD_SUPPORT
#define GUVAS12SD_SUPPORT               0
#endif

#ifndef GUVAS12SD_PIN
#define GUVAS12SD_PIN                   14
#endif

//------------------------------------------------------------------------------
// HLW8012 Energy monitor IC
// Enable support by passing HLW8012_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef HLW8012_SUPPORT
#define HLW8012_SUPPORT                 0
#endif

#ifndef HLW8012_SEL_PIN
#define HLW8012_SEL_PIN                 5
#endif

#ifndef HLW8012_CF1_PIN
#define HLW8012_CF1_PIN                 13
#endif

#ifndef HLW8012_CF_PIN
#define HLW8012_CF_PIN                  14
#endif

#ifndef HLW8012_SEL_CURRENT
#define HLW8012_SEL_CURRENT             HIGH    // SEL pin to HIGH to measure current
#endif

#ifndef HLW8012_CURRENT_R
#define HLW8012_CURRENT_R               0.001   // Current resistor
#endif

#ifndef HLW8012_VOLTAGE_R_UP
#define HLW8012_VOLTAGE_R_UP            ( 5 * 470000 )  // Upstream voltage resistor
#endif

#ifndef HLW8012_VOLTAGE_R_DOWN
#define HLW8012_VOLTAGE_R_DOWN          ( 1000 )        // Downstream voltage resistor
#endif

#ifndef HLW8012_CURRENT_RATIO
#define HLW8012_CURRENT_RATIO           0       // Set to 0 to use factory defaults
#endif

#ifndef HLW8012_VOLTAGE_RATIO
#define HLW8012_VOLTAGE_RATIO           0       // Set to 0 to use factory defaults
#endif

#ifndef HLW8012_POWER_RATIO
#define HLW8012_POWER_RATIO             0       // Set to 0 to use factory defaults
#endif

#ifndef HLW8012_USE_INTERRUPTS
#define HLW8012_USE_INTERRUPTS          1       // Use interrupts to trap HLW8012 signals
#endif

#ifndef HLW8012_WAIT_FOR_WIFI
#define HLW8012_WAIT_FOR_WIFI           0       // Weather to enable interrupts only after
                                                // wifi connection has been stablished
#endif

#ifndef HLW8012_INTERRUPT_ON
#define HLW8012_INTERRUPT_ON            CHANGE  // When to trigger the interrupt
                                                // Use CHANGE for HLW8012
                                                // Use FALLING for BL0937 / HJL0
#endif

//------------------------------------------------------------------------------
// LDR sensor
// Enable support by passing LDR_SUPPORT=1 build flag
//------------------------------------------------------------------------------
 
#ifndef SENSOR_LUX_CORRECTION
#define SENSOR_LUX_CORRECTION           0.0     // Offset correction
#endif

#ifndef LDR_SUPPORT
#define LDR_SUPPORT                     0
#endif
 
#ifndef LDR_SAMPLES
#define LDR_SAMPLES                     10      // Number of samples
#endif
 
#ifndef LDR_DELAY
#define LDR_DELAY                       0       // Delay between samples in micros
#endif
 
#ifndef LDR_TYPE
#define LDR_TYPE                        LDR_GL5528
#endif
 
#ifndef LDR_ON_GROUND
#define LDR_ON_GROUND                   true
#endif
 
#ifndef LDR_RESISTOR
#define LDR_RESISTOR                    10000   // Resistance
#endif
 
#ifndef LDR_MULTIPLICATION
#define LDR_MULTIPLICATION              32017200
#endif
 
#ifndef LDR_POWER
#define LDR_POWER                       1.5832
#endif
 
//------------------------------------------------------------------------------
// MHZ19 CO2 sensor
// Enable support by passing MHZ19_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef MHZ19_SUPPORT
#define MHZ19_SUPPORT                   0
#endif

#ifndef MHZ19_RX_PIN
#define MHZ19_RX_PIN                    13
#endif

#ifndef MHZ19_TX_PIN
#define MHZ19_TX_PIN                    15
#endif

//------------------------------------------------------------------------------
// MICS-2710 (and MICS-4514) NO2 sensor
// Enable support by passing MICS2710_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef MICS2710_SUPPORT
#define MICS2710_SUPPORT                0
#endif

#ifndef MICS2710_NOX_PIN
#define MICS2710_NOX_PIN                0
#endif

#ifndef MICS2710_PRE_PIN
#define MICS2710_PRE_PIN                4
#endif

#define MICS2710_PREHEAT_TIME           10000   // 10s preheat for NOX read
#define MICS2710_RL                     820     // RL, load resistor
#define MICS2710_R0                     2200    // R0 calibration value for NO2 sensor,
                                                // Typical value as per datasheet

//------------------------------------------------------------------------------
// MICS-5525 (and MICS-4514) CO sensor
// Enable support by passing MICS5525_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef MICS5525_SUPPORT
#define MICS5525_SUPPORT                0
#endif

#ifndef MICS5525_RED_PIN
#define MICS5525_RED_PIN                0
#endif

#define MICS5525_RL                     820     // RL, load resistor
#define MICS5525_R0                     750000  // R0 calibration value for NO2 sensor,
                                                // Typical value as per datasheet

//------------------------------------------------------------------------------
// NTC sensor
// Enable support by passing NTC_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef NTC_SUPPORT
#define NTC_SUPPORT                     0
#endif

#ifndef NTC_SAMPLES
#define NTC_SAMPLES                     10      // Number of samples
#endif

#ifndef NTC_DELAY
#define NTC_DELAY                       0       // Delay between samples in micros
#endif

#ifndef NTC_R_UP
#define NTC_R_UP                        0       // Resistor upstream, set to 0 if none
#endif

#ifndef NTC_R_DOWN
#define NTC_R_DOWN                      10000   // Resistor downstream, set to 0 if none
#endif

#ifndef NTC_T0
#define NTC_T0                          298.15  // 25 Celsius
#endif

#ifndef NTC_R0
#define NTC_R0                          10000   // Resistance at T0
#endif

#ifndef NTC_BETA
#define NTC_BETA                        3977    // Beta coeficient
#endif

//------------------------------------------------------------------------------
// Particle Monitor based on Plantower PMS
// Enable support by passing PMSX003_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef PMSX003_SUPPORT
#define PMSX003_SUPPORT                 0
#endif

#ifndef PMS_TYPE
#define PMS_TYPE                        PMS_TYPE_X003
#endif

// You can enable smart sleep (read 6-times then sleep on 24-reading-cycles) to extend PMS sensor's life.
// Otherwise the default lifetime of PMS sensor is about 8000-hours/1-years.
// The PMS's fan will stop working on sleeping cycle, and will wake up on reading cycle.
#ifndef PMS_SMART_SLEEP
#define PMS_SMART_SLEEP                 0
#endif

#ifndef PMS_USE_SOFT
#define PMS_USE_SOFT                    0       // If PMS_USE_SOFT == 1, DEBUG_SERIAL_SUPPORT must be 0
#endif

#ifndef PMS_RX_PIN
#define PMS_RX_PIN                      13      // Software serial RX GPIO (if PMS_USE_SOFT == 1)
#endif

#ifndef PMS_TX_PIN
#define PMS_TX_PIN                      15      // Software serial TX GPIO (if PMS_USE_SOFT == 1)
#endif

#ifndef PMS_HW_PORT
#define PMS_HW_PORT                     Serial  // Hardware serial port (if PMS_USE_SOFT == 0)
#endif

//------------------------------------------------------------------------------
// Pulse Meter Energy monitor
// Enable support by passing PULSEMETER_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef PULSEMETER_SUPPORT
#define PULSEMETER_SUPPORT              0
#endif

#ifndef PULSEMETER_PIN
#define PULSEMETER_PIN                  5
#endif

#ifndef PULSEMETER_ENERGY_RATIO
#define PULSEMETER_ENERGY_RATIO         4000        // In pulses/kWh
#endif

#ifndef PULSEMETER_INTERRUPT_ON
#define PULSEMETER_INTERRUPT_ON         FALLING
#endif

#ifndef PULSEMETER_DEBOUNCE
#define PULSEMETER_DEBOUNCE             50         // Do not register pulses within less than 50 millis
#endif

//------------------------------------------------------------------------------
// PZEM004T based power monitor
// Enable support by passing PZEM004T_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef PZEM004T_SUPPORT
#define PZEM004T_SUPPORT                0
#endif

#ifndef PZEM004T_USE_SOFT
#define PZEM004T_USE_SOFT               0       // Software serial is not working atm, use hardware serial
#endif

#ifndef PZEM004T_RX_PIN
#define PZEM004T_RX_PIN                 13      // Software serial RX GPIO (if PZEM004T_USE_SOFT == 1)
#endif

#ifndef PZEM004T_TX_PIN
#define PZEM004T_TX_PIN                 15      // Software serial TX GPIO (if PZEM004T_USE_SOFT == 1)
#endif

#ifndef PZEM004T_HW_PORT
#define PZEM004T_HW_PORT                Serial  // Hardware serial port (if PZEM004T_USE_SOFT == 0)
#endif

#ifndef PZEM004T_ADDRESSES
#define PZEM004T_ADDRESSES              "192.168.1.1"  // Device(s) address(es), separated by space, "192.168.1.1 192.168.1.2 192.168.1.3"
#endif

#ifndef PZEM004T_READ_INTERVAL
#define PZEM004T_READ_INTERVAL          1500    // Read interval between same device
#endif

#ifndef PZEM004T_MAX_DEVICES
#define PZEM004T_MAX_DEVICES            3
#endif

//------------------------------------------------------------------------------
// SDS011 particulates sensor
// Enable support by passing SDS011_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef SDS011_SUPPORT
#define SDS011_SUPPORT                   0
#endif

#ifndef SDS011_RX_PIN
#define SDS011_RX_PIN                    14
#endif

#ifndef SDS011_TX_PIN
#define SDS011_TX_PIN                    12
#endif

//------------------------------------------------------------------------------
// SenseAir CO2 sensor
// Enable support by passing SENSEAIR_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef SENSEAIR_SUPPORT
#define SENSEAIR_SUPPORT                0
#endif

#ifndef SENSEAIR_RX_PIN
#define SENSEAIR_RX_PIN                 0
#endif

#ifndef SENSEAIR_TX_PIN
#define SENSEAIR_TX_PIN                 2
#endif

//------------------------------------------------------------------------------
// SHT3X I2C (Wemos) temperature & humidity sensor
// Enable support by passing SHT3X_I2C_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef SHT3X_I2C_SUPPORT
#define SHT3X_I2C_SUPPORT               0
#endif

#ifndef SHT3X_I2C_ADDRESS
#define SHT3X_I2C_ADDRESS               0x00    // 0x00 means auto
#endif

//------------------------------------------------------------------------------
// SI7021 temperature & humidity sensor
// Enable support by passing SI7021_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef SI7021_SUPPORT
#define SI7021_SUPPORT                  0
#endif

#ifndef SI7021_ADDRESS
#define SI7021_ADDRESS                  0x00    // 0x00 means auto
#endif

//------------------------------------------------------------------------------
// Sonar
// Enable support by passing SONAR_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef SONAR_SUPPORT
#define SONAR_SUPPORT                  0
#endif

#ifndef SONAR_TRIGGER
#define SONAR_TRIGGER                  12                            // GPIO for the trigger pin (output)
#endif

#ifndef SONAR_ECHO
#define SONAR_ECHO                     14                            // GPIO for the echo pin (input)
#endif

#ifndef SONAR_MAX_DISTANCE
#define SONAR_MAX_DISTANCE             MAX_SENSOR_DISTANCE           // Max sensor distance in cm
#endif

#ifndef SONAR_ITERATIONS
#define SONAR_ITERATIONS               5                             // Number of iterations to ping for
#endif                                                               // error correction.

//------------------------------------------------------------------------------
// TMP3X analog temperature sensor
// Enable support by passing TMP3X_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef TMP3X_SUPPORT
#define TMP3X_SUPPORT                   0
#endif

#ifndef TMP3X_TYPE
#define TMP3X_TYPE                      TMP3X_TMP35
#endif

//------------------------------------------------------------------------------
// V9261F based power sensor
// Enable support by passing SI7021_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef V9261F_SUPPORT
#define V9261F_SUPPORT                  0
#endif

#ifndef V9261F_PIN
#define V9261F_PIN                      2       // TX pin from the V9261F
#endif

#ifndef V9261F_PIN_INVERSE
#define V9261F_PIN_INVERSE              1       // Signal is inverted
#endif

#define V9261F_SYNC_INTERVAL            600     // Sync signal length (ms)
#define V9261F_BAUDRATE                 4800    // UART baudrate

// Default ratios
#define V9261F_CURRENT_FACTOR           79371434.0
#define V9261F_VOLTAGE_FACTOR           4160651.0
#define V9261F_POWER_FACTOR             153699.0
#define V9261F_RPOWER_FACTOR            V9261F_CURRENT_FACTOR

//------------------------------------------------------------------------------
// VEML6075 based power sensor
// Enable support by passing VEML6075_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef VEML6075_SUPPORT
#define VEML6075_SUPPORT                  0
#endif

#ifndef VEML6075_INTEGRATION_TIME
#define VEML6075_INTEGRATION_TIME         VEML6075::IT_100MS        // The time, in milliseconds, allocated for a single
#endif                                                              // measurement. A longer timing budget allows for more
                                                                    // accurate results at the cost of power.

#ifndef VEML6075_DYNAMIC_MODE
#define VEML6075_DYNAMIC_MODE             VEML6075::DYNAMIC_NORMAL  // The dynamic mode can either be normal or high. In high
#endif                                                              // dynamic mode, the resolution increases by about two
                                                                    // times.
//------------------------------------------------------------------------------
// VL53L1X
// Enable support by passing VL53L1X_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef VL53L1X_SUPPORT
#define VL53L1X_SUPPORT                              0
#endif

#ifndef VL53L1X_I2C_ADDRESS
#define VL53L1X_I2C_ADDRESS                          0x00          // 0x00 means auto
#endif

#ifndef VL53L1X_DISTANCE_MODE
#define VL53L1X_DISTANCE_MODE                        VL53L1X::Long // The distance mode of the sensor. Can be one of
#endif                                                             // `VL53L1X::Short`, `VL53L1X::Medium`, or `VL53L1X::Long.
                                                                   // Shorter distance modes are less affected by ambient light
                                                                   // but have lower maximum ranges, especially in the dark.


#ifndef VL53L1X_MEASUREMENT_TIMING_BUDGET
#define VL53L1X_MEASUREMENT_TIMING_BUDGET            140000        // The time, in microseconds, allocated for a single
                                                                   // measurement. A longer timing budget allows for more
                                                                   // accurate at the cost of power. The minimum budget is
                                                                   // 20 ms (20000 us) in short distance mode and 33 ms for
                                                                   // medium and long distance modes.
#endif

#ifndef VL53L1X_INTER_MEASUREMENT_PERIOD
#define VL53L1X_INTER_MEASUREMENT_PERIOD             50            // Period, in milliseconds, determining how
#endif                                                             // often the sensor takes a measurement.

//------------------------------------------------------------------------------
// MAX6675
// Enable support by passing MAX6675_SUPPORT=1 build flag
//------------------------------------------------------------------------------
#ifndef MAX6675_CS_PIN
#define MAX6675_CS_PIN                               13
#endif

#ifndef MAX6675_SO_PIN
#define MAX6675_SO_PIN                               12
#endif

#ifndef MAX6675_SCK_PIN
#define MAX6675_SCK_PIN                              14
#endif

//------------------------------------------------------------------------------
// EZOPH pH meter
// Enable support by passing EZOPH_SUPPORT=1 build flag
//------------------------------------------------------------------------------

#ifndef EZOPH_SUPPORT
#define EZOPH_SUPPORT                0
#endif

#ifndef EZOPH_RX_PIN
#define EZOPH_RX_PIN                 13      // Software serial RX GPIO
#endif

#ifndef EZOPH_TX_PIN
#define EZOPH_TX_PIN                 15      // Software serial TX GPIO
#endif

#ifndef EZOPH_SYNC_INTERVAL
#define EZOPH_SYNC_INTERVAL          1000    // Amount of time (in ms) sync new readings.
#endif

// =============================================================================
// Sensor helpers configuration - can't move to dependencies.h
// =============================================================================

#ifndef SENSOR_SUPPORT
#define SENSOR_SUPPORT ( \
    AM2320_SUPPORT || \
    ANALOG_SUPPORT || \
    BH1750_SUPPORT || \
    BMP180_SUPPORT || \
    BMX280_SUPPORT || \
    CSE7766_SUPPORT || \
    DALLAS_SUPPORT || \
    DHT_SUPPORT || \
    DIGITAL_SUPPORT || \
    ECH1560_SUPPORT || \
    EMON_ADC121_SUPPORT || \
    EMON_ADS1X15_SUPPORT || \
    EMON_ANALOG_SUPPORT || \
    EVENTS_SUPPORT || \
    GEIGER_SUPPORT || \
    GUVAS12SD_SUPPORT || \
    HLW8012_SUPPORT || \
    LDR_SUPPORT || \
    MICS2710_SUPPORT || \
    MICS5525_SUPPORT || \
    MHZ19_SUPPORT || \
    NTC_SUPPORT || \
    SDS011_SUPPORT || \
    SENSEAIR_SUPPORT || \
    PMSX003_SUPPORT || \
    PZEM004T_SUPPORT || \
    PULSEMETER_SUPPORT || \
    SHT3X_I2C_SUPPORT || \
    SI7021_SUPPORT || \
    SONAR_SUPPORT || \
    TMP3X_SUPPORT || \
    V9261F_SUPPORT || \
    VEML6075_SUPPORT || \
    VL53L1X_SUPPORT || \
    MAX6675_SUPPORT || \
    EZOPH_SUPPORT \
)
#endif

// -----------------------------------------------------------------------------
// ADC
// -----------------------------------------------------------------------------

// Default ADC mode is to monitor internal power supply
#ifndef ADC_MODE_VALUE
#define ADC_MODE_VALUE                  ADC_VCC
#endif

// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------

#ifndef I2C_SUPPORT
#define I2C_SUPPORT                     0       // I2C enabled (1.98Kb)
#endif

#define I2C_USE_BRZO                    0       // Use brzo_i2c library or standard Wire

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN                     SDA     // SDA GPIO (Sonoff => 4)
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN                     SCL     // SCL GPIO (Sonoff => 14)
#endif

#define I2C_CLOCK_STRETCH_TIME          200     // BRZO clock stretch time
#define I2C_SCL_FREQUENCY               1000    // BRZO SCL frequency
#define I2C_CLEAR_BUS                   0       // Clear I2C bus on boot
#define I2C_PERFORM_SCAN                1       // Perform a bus scan on boot

//--------------------------------------------------------------------------------
// Class loading
//--------------------------------------------------------------------------------

#if SENSOR_SUPPORT

#if AM2320_SUPPORT
    #include "../sensors/AM2320Sensor.h"
#endif

#if ANALOG_SUPPORT
    #include "../sensors/AnalogSensor.h"
#endif

#if BH1750_SUPPORT
    #include "../sensors/BH1750Sensor.h"
#endif

#if BMP180_SUPPORT
    #include "../sensors/BMP180Sensor.h"
#endif

#if BMX280_SUPPORT
    #include "../sensors/BMX280Sensor.h"
#endif

#if CSE7766_SUPPORT
    #include "../sensors/CSE7766Sensor.h"
#endif

#if DALLAS_SUPPORT
    #include "../sensors/DallasSensor.h"
#endif

#if DHT_SUPPORT
    #include "../sensors/DHTSensor.h"
#endif

#if DIGITAL_SUPPORT
    #include "../sensors/DigitalSensor.h"
#endif

#if ECH1560_SUPPORT
    #include "../sensors/ECH1560Sensor.h"
#endif

#if EMON_ADC121_SUPPORT
    #include "../sensors/EmonADC121Sensor.h"
#endif

#if EMON_ADS1X15_SUPPORT
    #include "../sensors/EmonADS1X15Sensor.h"
#endif

#if EMON_ANALOG_SUPPORT
    #include "../sensors/EmonAnalogSensor.h"
#endif

#if EVENTS_SUPPORT
    #include "../sensors/EventSensor.h"
#endif

#if EZOPH_SUPPORT
    #include "../sensors/EZOPHSensor.h"
#endif

#if GEIGER_SUPPORT
    #include "../sensors/GeigerSensor.h"
#endif

#if GUVAS12SD_SUPPORT
    #include "../sensors/GUVAS12SDSensor.h"
#endif

#if HLW8012_SUPPORT
    #include "../sensors/HLW8012Sensor.h"
#endif

#if LDR_SUPPORT
    #include "../sensors/LDRSensor.h"
#endif

#if MAX6675_SUPPORT
    #include "../sensors/MAX6675Sensor.h"
#endif 

#if MHZ19_SUPPORT
    #include "../sensors/MHZ19Sensor.h"
#endif

#if MICS2710_SUPPORT
    #include "../sensors/MICS2710Sensor.h"
#endif

#if MICS5525_SUPPORT
    #include "../sensors/MICS5525Sensor.h"
#endif

#if NTC_SUPPORT
    #include "../sensors/NTCSensor.h"
#endif

#if SDS011_SUPPORT
    #include "../sensors/SDS011Sensor.h"
#endif

#if SENSEAIR_SUPPORT
    #include "../sensors/SenseAirSensor.h"
#endif

#if PMSX003_SUPPORT
    #include "../sensors/PMSX003Sensor.h"
#endif

#if PULSEMETER_SUPPORT
    #include "../sensors/PulseMeterSensor.h"
#endif

#if PZEM004T_SUPPORT
    #include "../sensors/PZEM004TSensor.h"
#endif

#if SI7021_SUPPORT
    #include "../sensors/SI7021Sensor.h"
#endif

#if SHT3X_I2C_SUPPORT
    #include "../sensors/SHT3XI2CSensor.h"
#endif

#if SONAR_SUPPORT
    #include "../sensors/SonarSensor.h"
#endif

#if TMP3X_SUPPORT
    #include "../sensors/TMP3XSensor.h"
#endif

#if V9261F_SUPPORT
    #include "../sensors/V9261FSensor.h"
#endif

#if VEML6075_SUPPORT
    #include "../sensors/VEML6075Sensor.h"
#endif

#if VL53L1X_SUPPORT
    #include "../sensors/VL53L1XSensor.h"
#endif

#endif // SENSOR_SUPPORT
