#!/bin/bash

set -eu -o pipefail

cfg_basic="
#define LED1_PIN 0
#define RELAY1_PIN 0
#define BUTTON1_PIN 0
#define BUTTON1_MODE BUTTON_PUSHBUTTON
#define BUTTON1_RELAY 1"

cfg_sensor="
#define AM2320_SUPPORT 1
#define BH1750_SUPPORT 1
#define BMP180_SUPPORT 1
#define BMX280_SUPPORT 1
#define SHT3X_I2C_SUPPORT 1
#define SHT3X_I2C_SUPPORT 1
#define SI7021_SUPPORT 1
#define PMSX003_SUPPORT 1
#define SENSEAIR_SUPPORT 1
#define VL53L1X_SUPPORT 1
#define DALLAS_SUPPORT 1
#define MICS2710_SUPPORT 1
#define MICS5525_SUPPORT 1
#define MAX6675_SUPPORT 1
#define DHT_SUPPORT 1
#define TMP3X_SUPPORT 1
#define EVENTS_SUPPORT 1
#define DIGITAL_SUPPORT 1
#define SONAR_SUPPORT 1
#define MHZ19_SUPPORT 1
#define NTC_SUPPORT 1
#define LDR_SUPPORT 1
#define GUVAS12SD_SUPPORT 1"

cfg_emon="
#define EMON_ADC121_SUPPORT 1
#define EMON_ADS1X15_SUPPORT 1
#define CSE7766_SUPPORT 1
#define PZEM004T_SUPPORT 1
#define HLW8012_SUPPORT 1
#define ECH1560_SUPPORT 1
#define PULSEMETER_SUPPORT 1
#define V9261F_SUPPORT 1
#define ADE7953_SUPPORT 1
#define EMON_ANALOG_SUPPORT 1"

cfg_light_my92xx="
#define RELAY_PROVIDER RELAY_PROVIDER_LIGHT
#define LIGHT_PROVIDER LIGHT_PROVIDER_MY92XX
#define DUMMY_RELAY_COUNT 1
#define LIGHT_CHANNELS 5
#define MY92XX_MODEL MY92XX_MODEL_MY9231
#define MY92XX_CHIPS 2
#define MY92XX_DI_PIN 1
#define MY92XX_DCKI_PIN 2
#define MY92XX_COMMAND MY92XX_COMMAND_DEFAULT
#define MY92XX_MAPPING 4,3,5,0,1
#define IR_SUPPORT 1"

cfg_light_dimmer="
#define RELAY_PROVIDER RELAY_PROVIDER_LIGHT
#define LIGHT_PROVIDER LIGHT_PROVIDER_DIMMER
#define DUMMY_RELAY_COUNT 1
#define LIGHT_CHANNELS 5
#define LIGHT_CH1_PIN 5
#define LIGHT_CH2_PIN 4
#define LIGHT_CH3_PIN 12
#define LIGHT_CH4_PIN 13
#define LIGHT_CH5_PIN 14
#define IR_SUPPORT 1"

cfg_nondefault="
#define LLMNR_SUPPORT 1
#define NETBIOS_SUPPORT 1
#define SSDP_SUPPORT 1
#define RF_SUPPORT 1
#define RFB_DIRECT 1
#define MDNS_CLIENT_SUPPORT 1
#define NOFUSS_SUPPORT 1
#define UART_MQTT_SUPPORT 1
#define INFLUXDB_SUPPORT 1
#define OTA_MQTT_SUPPORT 1"

cfg_secureclient="
#define SECURE_CLIENT SECURE_CLIENT_BEARSSL
#define MQTT_LIBRARY MQTT_LIBRARY_ARDUINOMQTT
"

TARGET_ENVIRONMENT=${1:?"pio env name"}
shift 1

EXTRA_CONFIGURATIONS=$@

for cfg in cfg_basic cfg_sensor cfg_emon cfg_light_my92xx cfg_light_dimmer cfg_nondefault ${EXTRA_CONFIGURATIONS} ; do
    echo travis_fold:start:build_${cfg}
    echo "- building ${cfg}"
    printf '%s\n' "${!cfg}" | tee espurna/config/custom.h
    PLATFORMIO_SRC_BUILD_FLAGS="'-DMANUFACTURER=\"TEST\"' '-DDEVICE=\"TEST\"' -DUSE_CUSTOM_H" time pio run -s -e $TARGET_ENVIRONMENT
    echo travis_fold:end:build_${cfg}
done
