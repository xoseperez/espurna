/*
  Config

  Configuration file
*/

#ifndef Config_h
#define Config_h

#include "Arduino.h"

// -----------------------------------------------------------------------------
// Defaults
// -----------------------------------------------------------------------------

#define CONFIG_PATH         "/.config"

#define NETWORK_BUFFER      3

#define MQTT_SERVER         "192.168.1.100"
#define MQTT_PORT           1883
#define MQTT_TOPIC          "/test/switch/{identifier}"

#define AUTOOTA_SERVER      "http://192.168.1.100"
#define AUTOOTA_INTERVAL    600000

#define MAINS_VOLTAGE       230
#define CURRENT_RATIO       156

#define RF_CHANNEL          31
#define RF_DEVICE           1

// -----------------------------------------------------------------------------
// Class definition
// -----------------------------------------------------------------------------

class ConfigClass {

    public:

        String hostname;

        String ssid[NETWORK_BUFFER];
        String pass[NETWORK_BUFFER];

        String mqttServer = MQTT_SERVER;
        String mqttPort = String(MQTT_PORT);
        String mqttTopic = MQTT_TOPIC;
        String mqttUser = "";
        String mqttPassword = "";

        String rfChannel = String(RF_CHANNEL);
        String rfDevice = String(RF_DEVICE);

        String otaServer = String(AUTOOTA_SERVER);
        String otaInterval = String(AUTOOTA_INTERVAL);

        String pwMainsVoltage = String(MAINS_VOLTAGE);
        String pwCurrentRatio = String(CURRENT_RATIO);

        bool save();
        bool load();

};

extern ConfigClass config;

#endif
