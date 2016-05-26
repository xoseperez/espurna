/*

ESPurna
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
#include <RemoteReceiver.h>
#include "FS.h"

// -----------------------------------------------------------------------------
// Configuració
// -----------------------------------------------------------------------------

#define DEBUG

#define ENABLE_RF               1
#define ENABLE_OTA              1
#define ENABLE_MQTT             1
#define ENABLE_WEBSERVER        1

#define APP_NAME                "Espurna"
#define MAX_VERSION             0
#define MIN_VERSION             9
#define APP_AUTHOR              "xose.perez@gmail.com"
#define APP_WEBSITE             "http://tinkerman.cat"

#define MODEL                   "SONOFF"
#define BUTTON_PIN              0
#define RELAY_PIN               12
#define LED_PIN                 13

#define ADMIN_PASS              "fibonacci"
#define CONFIG_PATH             "/.config"
#define WIFI_CONNECT_TIMEOUT    5000
#define WIFI_RECONNECT_DELAY    5000
#define MQTT_RECONNECT_DELAY    10000
#define NETWORK_BUFFER          3
#define BUFFER_SIZE             1024

#define RF_PIN                  14
#define RF_CHANNEL              31
#define RF_DEVICE               1

#define MQTT_RETAIN             true

// -----------------------------------------------------------------------------
// Globals
// -----------------------------------------------------------------------------

char identifier[20] = {0};

byte network = 0;
String config_ssid[NETWORK_BUFFER];
String config_pass[NETWORK_BUFFER];

DebounceEvent button1 = false;

#if ENABLE_WEBSERVER
    ESP8266WebServer server(80);
#endif

#if ENABLE_MQTT
    WiFiClient client;
    PubSubClient mqtt(client);
    String mqtt_server = "192.168.1.100";
    String mqtt_topic = "/test/switch/{identifier}";
    String mqtt_port = "1883";
    char mqtt_subscribe_status[30];
    char mqtt_publish_status[30];
    char mqtt_publish_ip[30];
#endif

#if ENABLE_RF
    unsigned long rf_code = 0;
    unsigned long rf_code_on = 0;
    unsigned long rf_code_off = 0;
    String rf_channel = String(RF_CHANNEL);
    String rf_device = String(RF_DEVICE);
#endif

// -----------------------------------------------------------------------------
// Relay
// -----------------------------------------------------------------------------

void switchRelayOn() {
    if (!digitalRead(RELAY_PIN)) {
        #ifdef DEBUG
            Serial.println("Turning the relay ON");
        #endif
        #if ENABLE_MQTT
            if (mqtt.connected()) {
                mqtt.publish(mqtt_publish_status, "1", MQTT_RETAIN);
            }
        #endif
        digitalWrite(RELAY_PIN, HIGH);
    }
}

void switchRelayOff() {
    if (digitalRead(RELAY_PIN)) {
        #ifdef DEBUG
            Serial.println("Turning the relay OFF");
        #endif
        #if ENABLE_MQTT
            if (mqtt.connected()) {
                mqtt.publish(mqtt_publish_status, "0", MQTT_RETAIN);
            }
        #endif
        digitalWrite(RELAY_PIN, LOW);
    }
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

#if ENABLE_WEBSERVER

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
        content.replace("{appname}", APP_NAME);
        content.replace("{appversion}", String(MAX_VERSION) + String(".") + String(MIN_VERSION));
        content.replace("{ssid0}", config_ssid[0]);
        content.replace("{pass0}", config_pass[0]);
        content.replace("{ssid1}", config_ssid[1]);
        content.replace("{pass1}", config_pass[1]);
        content.replace("{ssid2}", config_ssid[2]);
        content.replace("{pass2}", config_pass[2]);
        #if ENABLE_MQTT
            content.replace("{mqtt_server}", mqtt_server);
            content.replace("{mqtt_port}", mqtt_port);
            content.replace("{mqtt_topic}", mqtt_topic);
        #endif
        #if ENABLE_RF
            content.replace("{rf_channel}", rf_channel);
            content.replace("{rf_device}", rf_device);
        #endif

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
        #if ENABLE_MQTT
            mqtt_server = server.arg("mqtt_server");
            mqtt_port = server.arg("mqtt_port");
            mqtt_topic = server.arg("mqtt_topic");
        #endif
        #if ENABLE_RF
            rf_channel = server.arg("rf_channel");
            rf_device = server.arg("rf_device");
        #endif

        server.send(202, "text/json", "{}");

        saveConfig();
        #if ENABLE_RF
            rfBuildCodes();
        #endif
        network = 0;
        wifiSetup(true);

    }

    void handleStatus() {

        #ifdef DEBUG
            //Serial.println("Request: /status");
        #endif

        String output = "{ ";
        output += "\"wifi\": ";
        output += (WiFi.status() == WL_CONNECTED) ? "1": "0";
        if ((WiFi.status() == WL_CONNECTED)) {
            output += ", \"network\": \"";
            output += WiFi.SSID();
            output += "\", \"ip\": \"";
            output += WiFi.localIP().toString();
            output += "\"";
        }
        #if ENABLE_MQTT
            output += ", \"mqtt\": ";
            output += (mqtt.connected()) ? "1": "0";
        #endif
        output += ", \"relay\": ";
        output += (digitalRead(RELAY_PIN)) ? "1": "0";
        output += " }";

        server.send(200, "text/json", output);
    }

    void webServerSetup() {

        // Relay control
        server.on("/relay/on", HTTP_GET, handleRelayOn);
        server.on("/relay/off", HTTP_GET, handleRelayOff);

        // Configuration page
        server.on("/save", HTTP_POST, handleSave);
        server.on("/status", HTTP_GET, handleStatus);
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

#endif

// -----------------------------------------------------------------------------
// Wifi
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

void wifiSetup(bool force) {

    if ((!force) && (WiFi.status() == WL_CONNECTED)) return;

    #if ENABLE_MQTT
        if (mqtt.connected()) mqtt.disconnect();
    #endif

    #if ENABLE_RF
        RemoteReceiver::disable();
    #endif

    WiFi.mode(WIFI_AP_STA);
    #ifdef DEBUG
        WiFi.printDiag(Serial);
    #endif

    // SoftAP mode
    WiFi.softAP(getIdentifier(), ADMIN_PASS);
    #ifdef DEBUG
        Serial.print("AP Mode: ");
        Serial.print(getIdentifier());
        Serial.print("/");
        Serial.print(ADMIN_PASS);
        Serial.print(", IP address: ");
        Serial.println(WiFi.softAPIP());
    #endif

    // STA mode
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
            WiFi.setAutoConnect(true);
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

    #if ENABLE_RF
        RemoteReceiver::enable();
    #endif

}

void wifiLoop() {
    static unsigned long timeout = millis();
    if (WiFi.status() != WL_CONNECTED) {
        if (timeout < millis()) {
            #if ENABLE_RF
                //RemoteReceiver::disable();
            #endif
            wifiSetup(false);
            timeout = millis() + WIFI_RECONNECT_DELAY;
        }
    }
}

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

#if ENABLE_MQTT

    void buildTopics() {

        String tmp;

        // Replace identifier
        String base = mqtt_topic;
        base.replace("{identifier}", getIdentifier());

        // Get publish status topic
        base.toCharArray(mqtt_publish_status, base.length()+1);
        mqtt_publish_status[base.length()+1] = 0;

        // Get publish ip topic
        tmp = base + "/ip";
        tmp.toCharArray(mqtt_publish_ip, tmp.length()+1);
        mqtt_publish_ip[tmp.length()+1] = 0;

        // Get subscribe status topic
        tmp = base + "/set";
        tmp.toCharArray(mqtt_subscribe_status, tmp.length()+1);
        mqtt_subscribe_status[tmp.length()+1] = 0;

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
                    Serial.println(mqtt_subscribe_status);
                #endif

                String ipString = WiFi.localIP().toString();
                char ip[ipString.length()+1];
                ipString.toCharArray(ip, ipString.length()+1);
                mqtt.publish(mqtt_publish_ip, ip, MQTT_RETAIN);
                mqtt.subscribe(mqtt_subscribe_status);


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

#endif

// -----------------------------------------------------------------------------
// RF
// -----------------------------------------------------------------------------

#if ENABLE_RF

    void rfLoop() {
        if (rf_code == 0) return;
        Serial.print("RF code: ");
        Serial.println(rf_code);
        if (rf_code == rf_code_on ) switchRelayOn();
        if (rf_code == rf_code_off ) switchRelayOff();
        rf_code = 0;
    }

    void rfBuildCodes() {

        unsigned long code = 0;

        // channel
        unsigned int channel = rf_channel.toInt();
        for (byte i = 0; i < 5; i++) {
            code *= 3;
            if (channel & 1) code += 1;
            channel >>= 1;
        }

        // device
        unsigned int device = rf_device.toInt();
        for (byte i = 0; i < 5; i++) {
            code *= 3;
            if (device != i) code += 2;
        }

        // status
        code *= 9;
        rf_code_off = code + 2;
        rf_code_on = code + 6;

        Serial.print("RF code ON: ");
        Serial.println(rf_code_on);
        Serial.print("RF code OFF: ");
        Serial.println(rf_code_off);

    }

    void rfCallback(unsigned long code, unsigned int period) {
        rf_code = code;
    }

    void rfSetup() {
        rfBuildCodes();
        RemoteReceiver::init(RF_PIN, 3, rfCallback);
        RemoteReceiver::enable();
    }

#endif

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
        #if ENABLE_MQTT
            file.println("mqtt_server=" + mqtt_server);
            file.println("mqtt_port=" + mqtt_port);
            file.println("mqtt_topic=" + mqtt_topic);
        #endif
        #if ENABLE_RF
            file.println("rf_channel=" + rf_channel);
            file.println("rf_device=" + rf_device);
        #endif
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
            #if ENABLE_MQTT
                else if (line.startsWith("mqtt_server=")) mqtt_server = line.substring(12);
                else if (line.startsWith("mqtt_port=")) mqtt_port = line.substring(10);
                else if (line.startsWith("mqtt_topic=")) mqtt_topic = line.substring(11);
            #endif
            #if ENABLE_RF
                else if (line.startsWith("rf_channel=")) rf_channel = line.substring(11);
                else if (line.startsWith("rf_device=")) rf_device = line.substring(10);
            #endif
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

#if ENABLE_OTA

    void OTASetup() {

        // Port defaults to 8266
        ArduinoOTA.setPort(8266);

        // Hostname defaults to esp8266-[ChipID]
        ArduinoOTA.setHostname(getIdentifier());

        // No authentication by default
        ArduinoOTA.setPassword((const char *) ADMIN_PASS);

        ArduinoOTA.onStart([]() {
            #ifdef ENABLE_RF
                RemoteReceiver::disable();
            #endif
            #ifdef DEBUG
                Serial.println("OTA - Start");
            #endif
        });

        ArduinoOTA.onEnd([]() {
            #ifdef DEBUG
                Serial.println("OTA - End");
            #endif
            #ifdef ENABLE_RF
                RemoteReceiver::enable();
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

#endif

// -----------------------------------------------------------------------------
// Hardware (buttons, LEDs,...)
// -----------------------------------------------------------------------------

void hardwareSetup() {
    Serial.begin(115200);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(RF_PIN, INPUT_PULLUP);
    button1 = DebounceEvent(BUTTON_PIN);
    SPIFFS.begin();
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
    delay(5000);
    welcome();

    #if ENABLE_OTA
        OTASetup();
    #endif
    loadConfig();
    wifiSetup(false);
    #if ENABLE_WEBSERVER
        webServerSetup();
    #endif
    #if ENABLE_MQTT
        mqttSetup();
    #endif
    #if ENABLE_RF
        rfSetup();
    #endif

}

void loop() {

    #if ENABLE_OTA
        OTALoop();
    #endif
    wifiLoop();
    #if ENABLE_MQTT
        mqttLoop();
    #endif
    #if ENABLE_RF
        rfLoop();
    #endif
    #if ENABLE_WEBSERVER
        webServerLoop();
    #endif
    hardwareLoop();
    delay(10);

}
