/*

Radio

RFM69 Radio Manager for ESP8266
Based on sample code by Felix Rusu - http://LowPowerLab.com/contact
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

#pragma once

#include <RFM69.h>
#include <RFM69_ATC.h>
#include <SPI.h>

// -----------------------------------------------------------------------------
// Configuration
// -----------------------------------------------------------------------------

#define PING_EVERY          3
#define RETRIES             2
#define REQUESTACK          1
#define RADIO_DEBUG         0
#define SEND_PACKET_ID      1
#define PACKET_SEPARATOR    ':'

/*
typedef struct {
    unsigned long messageID;
    unsigned char packetID;
    unsigned char senderID;
    unsigned char targetID;
    char * name;
    char * value;
    int16_t rssi;
} packet_t;
*/

typedef void (*TMessageCallback)(packet_t *);

class RFM69Manager: public RFM69_ATC {

    public:

        RFM69Manager(uint8_t slaveSelectPin=RF69_SPI_CS, uint8_t interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false, uint8_t interruptNum=RF69_IRQ_NUM):
            RFM69_ATC(slaveSelectPin, interruptPin, isRFM69HW, interruptNum) {};

        bool initialize(uint8_t frequency, uint8_t nodeID, uint8_t networkID, const char* key, uint8_t gatewayID = 0, int16_t targetRSSI = -70) {

            bool ret = RFM69_ATC::initialize(frequency, nodeID, networkID);
            encrypt(key);
            _gatewayID = gatewayID;
            if (_gatewayID > 0) enableAutoPower(targetRSSI);
            if (_isRFM69HW) setHighPower();

            #if RADIO_DEBUG
                char buff[50];
                sprintf(buff, "[RFM69] Working at %d Mhz", frequency == RF69_433MHZ ? 433 : frequency == RF69_868MHZ ? 868 : 915);
                Serial.println(buff);
                Serial.print(F("[RFM69] Node: "));
                Serial.println(nodeID);
                Serial.print(F("[RFM69] Network: "));
                Serial.println(networkID);
                if (gatewayID == 0) {
                    Serial.println("[RFM69] This node is a gateway");
                } else {
                    Serial.print(F("[RFM69] Gateway: "));
                    Serial.println(gatewayID);
                }
                Serial.println(F("[RFM69] Auto Transmission Control (ATC) enabled"));

            #endif

            return ret;

        }

        void promiscuous(bool promiscuous) {
            RFM69_ATC::promiscuous(promiscuous);
            #if RADIO_DEBUG
                if (_promiscuousMode) {
                    Serial.println(F("[RFM69] Promiscuous mode ON"));
                } else {
                    Serial.println(F("[RFM69] Promiscuous mode OFF"));
                }
            #endif
        }

        void onMessage(TMessageCallback fn) {
            _callback = fn;
        }

        void separator(char sep) {
            _separator = sep;
        }

        bool loop() {

            boolean ret = false;

            if (receiveDone()) {

                uint8_t senderID = SENDERID;
                uint8_t targetID = _promiscuousMode ? TARGETID : _address;
                int16_t rssi = RSSI;
                uint8_t length = DATALEN;
                char buffer[length + 1];
                strncpy(buffer, (const char *) DATA, length);
                buffer[length] = 0;

                // Do not send ACKs in promiscuous mode,
                // we want to listen without being heard
                if (!_promiscuousMode) {
                    if (ACKRequested()) sendACK();
                }

                uint8_t parts = 1;
                for (uint8_t i=0; i<length; i++) {
                    if (buffer[i] == _separator) ++parts;
                }

                if (parts > 1) {

                    char sep[2] = {_separator, 0};

                    uint8_t packetID = 0;
                    char * name = strtok(buffer, sep);
                    char * value = strtok(NULL, sep);
                    if (parts > 2) {
                        char * packet = strtok(NULL, sep);
                        packetID = atoi(packet);
                    }

                    _message.messageID = ++_receiveCount;
                    _message.packetID = packetID;
                    _message.senderID = senderID;
                    _message.targetID = targetID;
                    _message.name = name;
                    _message.value = value;
                    _message.rssi = rssi;
                    ret = true;

                    if (_callback != NULL) {
                        _callback(&_message);
                    }

                }

            }

            return ret;

        }

        bool send(uint8_t destinationID, char * name, char * value, uint8_t retries = RETRIES, bool requestACK = REQUESTACK) {

            char message[RF69_MAX_DATA_LEN];

            #if SEND_PACKET_ID
                if (++_sendCount == 0) _sendCount = 1;
                snprintf(message, RF69_MAX_DATA_LEN-1, "%s%c%s%c%d", name, _separator, value, _separator, _sendCount);
            #else
                snprintf(message, RF69_MAX_DATA_LEN-1, "%s%c%s", name, _separator, value);
            #endif

            #if RADIO_DEBUG
                Serial.print(F("[RFM69] Sending: "));
                Serial.print(message);
            #endif

            bool ret = true;
            if (retries > 0) {
                ret = sendWithRetry(destinationID, message, strlen(message), retries);
            } else {
                RFM69_ATC::send(destinationID, message, strlen(message), requestACK);
            }

            #if RADIO_DEBUG
                if (ret) {
                    Serial.println(" OK");
                } else {
                    Serial.println(" KO");
                }
            #endif

            return ret;

        }

        bool send(char * name, char * value, uint8_t retries = RETRIES) {
            return send(_gatewayID, name, value, retries, false);
        }

        bool send(char * name, char * value, bool requestACK = REQUESTACK) {
            return send(_gatewayID, name, value, 0, requestACK);
        }

        packet_t * getMessage() {
            return &_message;
        }

    protected:

        packet_t _message;
        TMessageCallback _callback = NULL;
        uint8_t _gatewayID = 0;
        unsigned long _receiveCount = 0;
        #if SEND_PACKET_ID
            unsigned char _sendCount = 0;
        #endif
        unsigned int _ackCount = 0;
        char _separator = PACKET_SEPARATOR;

        // ---------------------------------------------------------------------

        // overriding select the RFM69 transceiver (save SPI settings, set CS low)
        void select() {
            noInterrupts();
            #if defined (SPCR) && defined (SPSR)
                // save current SPI settings
                _SPCR = SPCR;
                _SPSR = SPSR;
            #endif
            // set RFM69 SPI settings
            SPI.setDataMode(SPI_MODE0);
            SPI.setBitOrder(MSBFIRST);
            #if defined(ARDUINO_ARCH_ESP8266)
                SPI.setClockDivider(SPI_CLOCK_DIV2); // speeding it up for the ESP8266
            #else
                SPI.setClockDivider(SPI_CLOCK_DIV4);
            #endif
            digitalWrite(_slaveSelectPin, LOW);
        }

};
