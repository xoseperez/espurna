/*
  Config

  Configuration file
*/

#include "Config.h"
#include "EEPROM.h"

#define DEBUG

bool ConfigClass::save() {

    #ifdef DEBUG
        Serial.println("[CONFIG] Saving");
    #endif

    String content;

    content += "hostname=" + hostname + "|";
    content += "ssid0=" + ssid[0] + "|";
    content += "pass0=" + pass[0] + "|";
    content += "ssid1=" + ssid[1] + "|";
    content += "pass1=" + pass[1] + "|";
    content += "ssid2=" + ssid[2] + "|";
    content += "pass2=" + pass[2] + "|";
    content += "mqttServer=" + mqttServer + "|";
    content += "mqttPort=" + mqttPort + "|";
    content += "mqttTopic=" + mqttTopic + "|";
    content += "rfChannel=" + rfChannel + "|";
    content += "rfDevice=" + rfDevice + "|";
    content += "nofussServer=" + nofussServer + "|";
    content += "nofussInterval=" + nofussInterval + "|";
    content += "pwMainsVoltage=" + pwMainsVoltage + "|";
    content += "pwCurrentRatio=" + pwCurrentRatio + "|";

    Serial.print("[CONFIG] ");
    Serial.println(content);

    EEPROM.write(CONFIG_START, CONFIG_VERSION);
    for (int i=0; i<content.length(); i++) {
        EEPROM.write(CONFIG_START + i + 1, content.charAt(i));
    }
    EEPROM.write(CONFIG_START + content.length() + 1, 0);
    EEPROM.commit();

    return true;

}

bool ConfigClass::load() {

    #ifdef DEBUG
        Serial.println("[CONFIG] Loading");
    #endif

    int version = EEPROM.read(CONFIG_START);
    if (version != CONFIG_VERSION) {
        #ifdef DEBUG
            Serial.println("[CONFIG] Wrong version");
        #endif
        return false;
    }

    String content;
    char character;
    int position = 0;
    do {
        character = EEPROM.read(CONFIG_START + 1 + position);
        content += character;
        position++;
    } while (character > 0);

    Serial.print("[CONFIG] ");
    Serial.println(content);

    int start = 0;
    int end = content.indexOf("|", start);
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
        else if (line.startsWith("nofussServer=")) nofussServer = line.substring(13);
        else if (line.startsWith("nofussInterval=")) nofussInterval = line.substring(15);
        else if (line.startsWith("pwMainsVoltage=")) pwMainsVoltage = line.substring(15);
        else if (line.startsWith("pwCurrentRatio=")) pwCurrentRatio = line.substring(15);

        if (end < 0) break;
        start = end + 1;
        end = content.indexOf("|", start);

    }

    #ifdef DEBUG
        Serial.println("[CONFIG] OK");
    #endif

    return true;

}

ConfigClass config;
