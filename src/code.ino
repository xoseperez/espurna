/*

ITead Sonoff Custom Firmware
Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <DebounceEvent.h>
#include <ArduinoOTA.h>
#include "FS.h"

// -----------------------------------------------------------------------------
// Configuració
// -----------------------------------------------------------------------------

#define DEBUG

#define APP_NAME                "Espurna"
#define MAX_VERSION             0
#define MIN_VERSION             9
#define APP_AUTHOR              "xose.perez@gmail.com"
#define APP_WEBSITE             "http://tinkerman.cat"

#define MODEL                   "SONOFF"
#define BUTTON_PIN              0
#define RELAY_PIN               12
#define LED_PIN                 13

#define AP_PASS                 "fibonacci"
#define OTA_PASS                "fibonacci"
#define BUFFER_SIZE             1024
#define CONFIG_PATH             "/.config"
#define WIFI_CONNECT_TIMEOUT    5000
#define WIFI_RECONNECT_DELAY    5000
#define MQTT_RECONNECT_DELAY    10000
#define NETWORK_BUFFER          3

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

ESP8266WebServer server(80);
WiFiClient client;
PubSubClient mqtt(client);

char identifier[20] = {0};

byte network = 0;
String config_ssid[NETWORK_BUFFER];
String config_pass[NETWORK_BUFFER];
String mqtt_server = "192.168.1.100";
String mqtt_topic = "/test/switch/{identifier}";
String mqtt_port = "1883";

char mqtt_subscribe_to[30];
char mqtt_publish_to[30];

DebounceEvent button1 = false;

// -----------------------------------------------------------------------------
// Relay
// -----------------------------------------------------------------------------

void switchRelayOn() {
    #ifdef DEBUG
        Serial.println("Turning the relay ON");
    #endif
    if (mqtt.connected()) {
        mqtt.publish(mqtt_publish_to, "1");
    }
    digitalWrite(RELAY_PIN, HIGH);
}

void switchRelayOff() {
    #ifdef DEBUG
        Serial.println("Turning the relay OFF");
    #endif
    if (mqtt.connected()) {
        mqtt.publish(mqtt_publish_to, "0");
    }
    digitalWrite(RELAY_PIN, LOW);
}

void toggleRelay() {
    if (digitalRead(RELAY_PIN)) {
        switchRelayOff();
    } else {
        switchRelayOn();
    }
}

// -----------------------------------------------------------------------------
// WebServer
// -----------------------------------------------------------------------------

String getContentType(String filename) {
    if (server.hasArg("download")) return "application/octet-stream";
    else if (filename.endsWith(".htm")) return "text/html";
    else if (filename.endsWith(".html")) return "text/html";
    else if (filename.endsWith(".css")) return "text/css";
    else if (filename.endsWith(".js")) return "application/javascript";
    else if (filename.endsWith(".png")) return "image/png";
    else if (filename.endsWith(".gif")) return "image/gif";
    else if (filename.endsWith(".jpg")) return "image/jpeg";
    else if (filename.endsWith(".ico")) return "image/x-icon";
    else if (filename.endsWith(".xml")) return "text/xml";
    else if (filename.endsWith(".pdf")) return "application/x-pdf";
    else if (filename.endsWith(".zip")) return "application/x-zip";
    else if (filename.endsWith(".gz")) return "application/x-gzip";
    return "text/plain";
}

void handleRelayOn() {
    #ifdef DEBUG
        Serial.println("Request: /on");
    #endif
    switchRelayOn();
    server.send(200, "text/plain", "ON");
}

void handleRelayOff() {
    #ifdef DEBUG
        Serial.println("Request: /off");
    #endif
    switchRelayOff();
    server.send(200, "text/plain", "OFF");
}

bool handleFileRead(String path) {

    #ifdef DEBUG
        Serial.println("Request: " + path);
    #endif

    if (path.endsWith("/")) path += "index.html";
    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";

    if (SPIFFS.exists(pathWithGz)) path = pathWithGz;
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        size_t sent = server.streamFile(file, contentType);
        size_t contentLength = file.size();
        file.close();
        return true;
    }

    return false;

}

void handleHome() {

    #ifdef DEBUG
        Serial.println("Request: /index.html");
    #endif

    String filename = "/index.html";
    String content = "";
    char buffer[BUFFER_SIZE];

    // Read file in chunks
    File file = SPIFFS.open(filename, "r");
    int size = file.size();
    while (size > 0) {
        size_t len = std::min(BUFFER_SIZE-1, size);
        file.read((uint8_t *) buffer, len);
        buffer[len] = 0;
        content += buffer;
        size -= len;
    }
    file.close();

    // Replace placeholders
    if (WiFi.status() == WL_CONNECTED) {
        content.replace("{status}", "Client + Acces Point");
        content.replace("{network}", config_ssid[network]);
        content.replace("{ip}", WiFi.localIP().toString());
    } else {
        content.replace("{status}", "Acces Point");
        content.replace("{network}", "");
        content.replace("{ip}", "");
    }
    content.replace("{ssid0}", config_ssid[0]);
    content.replace("{pass0}", config_pass[0]);
    content.replace("{ssid1}", config_ssid[1]);
    content.replace("{pass1}", config_pass[1]);
    content.replace("{ssid2}", config_ssid[2]);
    content.replace("{pass2}", config_pass[2]);
    content.replace("{mqtt_server}", mqtt_server);
    content.replace("{mqtt_port}", mqtt_port);
    content.replace("{mqtt_topic}", mqtt_topic);

    // Serve content
    String contentType = getContentType(filename);
    server.send(200, contentType, content);

}

void handleSave() {

    #ifdef DEBUG
        Serial.println("Request: /save");
    #endif

    config_ssid[0] = server.arg("ssid0");
    config_pass[0] = server.arg("pass0");
    config_ssid[1] = server.arg("ssid1");
    config_pass[1] = server.arg("pass1");
    config_ssid[2] = server.arg("ssid2");
    config_pass[2] = server.arg("pass2");
    mqtt_server = server.arg("mqtt_server");
    mqtt_port = server.arg("mqtt_port");
    mqtt_topic = server.arg("mqtt_topic");

    saveConfig();
    network = 0;
    wifiSetup();
    delay(100);

    String output = "{";
    output += "\"status\": \"";
    if (WiFi.status() == WL_CONNECTED) {
        output += "Client + Acces Point";
    } else {
        output += "Acces Point";
    }
    output += "\", \"ip\": \"";
    if (WiFi.status() == WL_CONNECTED) {
        output += WiFi.localIP().toString();
    }
    output += "\" }";
    server.send(200, "text/json", output);

}

void webServerSetup() {

    // Relay control
    server.on("/on", HTTP_GET, handleRelayOn);
    server.on("/off", HTTP_GET, handleRelayOff);

    // Configuration page
    server.on("/save", HTTP_POST, handleSave);
    server.on("/", HTTP_GET, handleHome);
    server.on("/index.html", HTTP_GET, handleHome);

    // Anything else
    server.onNotFound([]() {

        // Hidden files
        if (server.uri().startsWith("/.")) {
            server.send(403, "text/plain", "Forbidden");
            return;
        }

        // Existing files in SPIFFS
        if (!handleFileRead(server.uri())) {
            server.send(404, "text/plain", "NotFound");
            return;
        }

    });

    // Run server
    server.begin();

}

void webServerLoop() {
    server.handleClient();
}

// -----------------------------------------------------------------------------
// Wifi modes
// -----------------------------------------------------------------------------

char * getIdentifier() {
    if (identifier[0] == 0) {
        uint8_t mac[WL_MAC_ADDR_LENGTH];
        WiFi.softAPmacAddress(mac);
        String name = MODEL + String("_") + String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
        name.toUpperCase();
        byte length = std::min(20, (int) name.length() + 1);
        name.toCharArray(identifier, length);
    }
    return identifier;
}

void wifiSetup() {

    // Disconnect MQTT
    if (mqtt.connected()) mqtt.disconnect();

    // STA mode
    WiFi.mode(WIFI_AP_STA);
    if (config_ssid[network].length() > 0) {

        char ssid[config_ssid[network].length()+1];
        char pass[config_pass[network].length()+1];
        config_ssid[network].toCharArray(ssid, config_ssid[network].length()+1);
        config_pass[network].toCharArray(pass, config_pass[network].length()+1);
        WiFi.begin(ssid, pass);

        #ifdef DEBUG
            Serial.println("Connecting to WIFI " + config_ssid[network]);
        #endif

        // Wait
        unsigned long timeout = millis() + WIFI_CONNECT_TIMEOUT;
        while (timeout > millis()) {
            showStatus();
            if (WiFi.status() == WL_CONNECTED) break;
            delay(100);
        }

        #ifdef DEBUG
            Serial.print("STA Mode: ");
            Serial.print(config_ssid[network]);
            Serial.print("/");
            Serial.print(config_pass[network]);
            Serial.print(", IP address: ");
        #endif

        if (WiFi.status() == WL_CONNECTED) {
            #ifdef DEBUG
                Serial.println(WiFi.localIP());
            #endif
        } else {
            network = (network + 1) % NETWORK_BUFFER;
            #ifdef DEBUG
                Serial.println("NOT CONNECTED");
            #endif
        }

    }

    if (WiFi.status() != WL_CONNECTED) WiFi.mode(WIFI_AP);
    WiFi.softAP(getIdentifier(), AP_PASS);

    #ifdef DEBUG
        Serial.print("AP Mode: ");
        Serial.print(getIdentifier());
        Serial.print("/");
        Serial.print(AP_PASS);
        Serial.print(", IP address: ");
        Serial.println(WiFi.softAPIP());
    #endif

}

void wifiLoop() {
    static unsigned long timeout = millis();
    if (WiFi.status() != WL_CONNECTED) {
        if (timeout < millis()) {
            wifiSetup();
            timeout = millis() + WIFI_RECONNECT_DELAY;
        }
    }
}

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

void buildTopics() {

    // Replace identifier
    String base = mqtt_topic;
    base.replace("{identifier}", getIdentifier());

    // Get publish topic
    base.toCharArray(mqtt_publish_to, base.length()+1);
    mqtt_publish_to[base.length()+1] = 0;

    // Get subscribe topic
    String subscribe = base + "/set";
    subscribe.toCharArray(mqtt_subscribe_to, subscribe.length()+1);
    mqtt_subscribe_to[subscribe.length()+1] = 0;

}

void mqttCallback(char* topic, byte* payload, unsigned int length) {

    #ifdef DEBUG
        Serial.print("MQTT message ");
        Serial.print(topic);
        Serial.print(" => ");
        for (int i = 0; i < length; i++) {
            Serial.print((char)payload[i]);
        }
        Serial.println();
    #endif

    if ((char)payload[0] == '1') {
        switchRelayOn();
    } else {
        switchRelayOff();
    }

}

void mqttConnect() {

    if (!mqtt.connected()) {

        char buffer[mqtt_server.length()+1];
        mqtt_server.toCharArray(buffer, mqtt_server.length()+1);
        mqtt.setServer(buffer, mqtt_port.toInt());

        #ifdef DEBUG
            Serial.print("Connecting to MQTT broker: ");
        #endif

        if (mqtt.connect(getIdentifier())) {

            buildTopics();

            #ifdef DEBUG
                Serial.println("connected!");
                Serial.print("Subscribing to ");
                Serial.println(mqtt_subscribe_to);
            #endif

            mqtt.publish(mqtt_publish_to, "HOLA");
            mqtt.subscribe(mqtt_subscribe_to);


        } else {

            #ifdef DEBUG
                Serial.print("failed, rc=");
                Serial.println(mqtt.state());
            #endif

        }
    }

}

void mqttSetup() {
    mqtt.setCallback(mqttCallback);
}

void mqttLoop() {
    static unsigned long timeout = millis();
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqtt.connected()) {
            if (timeout < millis()) {
                mqttConnect();
                timeout = millis() + MQTT_RECONNECT_DELAY;
            }
        }
        if (mqtt.connected()) mqtt.loop();
    }
}

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

bool saveConfig() {
    File file = SPIFFS.open(CONFIG_PATH, "w");
    if (file) {
        file.println("ssid0=" + config_ssid[0]);
        file.println("pass0=" + config_pass[0]);
        file.println("ssid1=" + config_ssid[1]);
        file.println("pass1=" + config_pass[1]);
        file.println("ssid2=" + config_ssid[2]);
        file.println("pass2=" + config_pass[2]);
        file.println("mqtt_server=" + mqtt_server);
        file.println("mqtt_port=" + mqtt_port);
        file.println("mqtt_topic=" + mqtt_topic);
        file.close();
        return true;
    }
    return false;
}

bool loadConfig() {

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
            #ifdef DEBUG
                Serial.println(line);
            #endif
            if (line.startsWith("ssid0=")) config_ssid[0] = line.substring(6);
            else if (line.startsWith("pass0=")) config_pass[0] = line.substring(6);
            else if (line.startsWith("ssid1=")) config_ssid[1] = line.substring(6);
            else if (line.startsWith("pass1=")) config_pass[1] = line.substring(6);
            else if (line.startsWith("ssid2=")) config_ssid[2] = line.substring(6);
            else if (line.startsWith("pass2=")) config_pass[2] = line.substring(6);
            else if (line.startsWith("mqtt_server=")) mqtt_server = line.substring(12);
            else if (line.startsWith("mqtt_port=")) mqtt_port = line.substring(10);
            else if (line.startsWith("mqtt_topic=")) mqtt_topic = line.substring(11);
            if (end < 0) break;
            start = end + 1;
            end = content.indexOf("\n", start);
        }

        return true;
    }
    return false;

}

// -----------------------------------------------------------------------------
// OTA
// -----------------------------------------------------------------------------

void OTASetup() {

    // Port defaults to 8266
    ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname(getIdentifier());

    // No authentication by default
    ArduinoOTA.setPassword((const char *) OTA_PASS);

    ArduinoOTA.onStart([]() {
        #ifdef DEBUG
            Serial.println("OTA - Start");
        #endif
    });

    ArduinoOTA.onEnd([]() {
        #ifdef DEBUG
            Serial.println("OTA - End");
        #endif
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        #ifdef DEBUG
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        #endif
    });

    ArduinoOTA.onError([](ota_error_t error) {
        #ifdef DEBUG
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
        #endif
    });

    ArduinoOTA.begin();

}

void OTALoop() {
    ArduinoOTA.handle();
}

// -----------------------------------------------------------------------------
// Hardware (buttons, LEDs,...)
// -----------------------------------------------------------------------------

void hardwareSetup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    button1 = DebounceEvent(BUTTON_PIN);
}

void blink(unsigned long delayOff, unsigned long delayOn) {
    static unsigned long next = millis();
    static bool status = HIGH;
    if (next < millis()) {
        status = !status;
        digitalWrite(LED_PIN, status);
        next += ((status) ? delayOff : delayOn);
    }
}

void showStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        blink(5000, 500);
    } else {
        blink(500, 500);
    }
}

void hardwareLoop() {
    if (button1.loop()) {
        if (!button1.pressed()) toggleRelay();
    }
    showStatus();
}

// -----------------------------------------------------------------------------
// Booting
// -----------------------------------------------------------------------------

void welcome() {
    Serial.println();
    Serial.print(APP_NAME);
    Serial.print(" ");
    Serial.print(MAX_VERSION);
    Serial.print(".");
    Serial.println(MIN_VERSION);
    Serial.println(APP_WEBSITE);
    Serial.println(APP_AUTHOR);
    Serial.println();
    Serial.print("Device: ");
    Serial.println(getIdentifier());
    Serial.println();
}

void setup() {
    hardwareSetup();
    SPIFFS.begin();
    delay(5000);
    welcome();
    OTASetup();
    switchRelayOff();
    loadConfig();
    wifiSetup();
    webServerSetup();
    mqttSetup();
}

void loop() {
    OTALoop();
    wifiLoop();
    webServerLoop();
    mqttLoop();
    hardwareLoop();
    delay(1);
}
