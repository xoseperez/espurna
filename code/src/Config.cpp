/*
  Config

  Configuration file
*/

#include "Config.h"
#include "FS.h"

bool ConfigClass::save() {
    File file = SPIFFS.open(CONFIG_PATH, "w");
    if (file) {

        file.println("hostname=" + hostname);
        file.println("ssid0=" + ssid[0]);
        file.println("pass0=" + pass[0]);
        file.println("ssid1=" + ssid[1]);
        file.println("pass1=" + pass[1]);
        file.println("ssid2=" + ssid[2]);
        file.println("pass2=" + pass[2]);
        file.println("mqttServer=" + mqttServer);
        file.println("mqttPort=" + mqttPort);
        file.println("mqttTopic=" + mqttTopic);
        file.println("rfChannel=" + rfChannel);
        file.println("rfDevice=" + rfDevice);
        file.println("otaServer=" + otaServer);
        file.println("otaInterval=" + otaInterval);
        file.println("pwMainsVoltage=" + pwMainsVoltage);
        file.println("pwCurrentRatio=" + pwCurrentRatio);

        file.close();
        return true;
    }
    return false;
}

bool ConfigClass::load() {

    if (SPIFFS.exists(CONFIG_PATH)) {

        #ifdef DEBUG
            Serial.println("Reading config file");
        #endif

        // Read contents
        File file = SPIFFS.open(CONFIG_PATH, "r");
        String content = file.readString();
        file.close();

        // Parse contents
        content.replace("\r\n", "\n");
        content.replace("\r", "\n");

        int start = 0;
        int end = content.indexOf("\n", start);
        while (end > 0) {

            String line = content.substring(start, end);

            if (line.startsWith("hostname=")) hostname = line.substring(9);
            else if (line.startsWith("ssid0=")) ssid[0] = line.substring(6);
            else if (line.startsWith("pass0=")) pass[0] = line.substring(6);
            else if (line.startsWith("ssid1=")) ssid[1] = line.substring(6);
            else if (line.startsWith("pass1=")) pass[1] = line.substring(6);
            else if (line.startsWith("ssid2=")) ssid[2] = line.substring(6);
            else if (line.startsWith("pass2=")) pass[2] = line.substring(6);
            else if (line.startsWith("mqttServer=")) mqttServer = line.substring(11);
            else if (line.startsWith("mqttPort=")) mqttPort = line.substring(9);
            else if (line.startsWith("mqttTopic=")) mqttTopic = line.substring(10);
            else if (line.startsWith("rfChannel=")) rfChannel = line.substring(10);
            else if (line.startsWith("rfDevice=")) rfDevice = line.substring(9);
            else if (line.startsWith("otaServer=")) otaServer = line.substring(10);
            else if (line.startsWith("otaInterval=")) otaInterval = line.substring(12);
            else if (line.startsWith("pwMainsVoltage=")) pwMainsVoltage = line.substring(15);
            else if (line.startsWith("pwCurrentRatio=")) pwCurrentRatio = line.substring(15);

            if (end < 0) break;
            start = end + 1;
            end = content.indexOf("\n", start);

        }

        return true;
    }
    return false;

}

ConfigClass config;
