/*

ESPurna
NOFUSS MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_NOFUSS

    #include "NoFUSSClient.h"

    // -----------------------------------------------------------------------------
    // NOFUSS
    // -----------------------------------------------------------------------------

    void nofussSetup() {

        NoFUSSClient.setServer(config.nofussServer);
        NoFUSSClient.setDevice(DEVICE);
        NoFUSSClient.setVersion(APP_VERSION);

        NoFUSSClient.onMessage([](nofuss_t code) {

            if (code == NOFUSS_START) {
                Serial.println(F("[NoFUSS] Start"));
            }

            if (code == NOFUSS_UPTODATE) {
                Serial.println(F("[NoFUSS] Already in the last version"));
            }

            if (code == NOFUSS_PARSE_ERROR) {
                Serial.println(F("[NoFUSS] Error parsing server response"));
            }

            if (code == NOFUSS_UPDATING) {
                Serial.println(F("[NoFUSS] Updating"));
                Serial.print(  F("         New version: "));
                Serial.println(NoFUSSClient.getNewVersion());
                Serial.print(  F("         Firmware: "));
                Serial.println(NoFUSSClient.getNewFirmware());
                Serial.print(  F("         File System: "));
                Serial.println(NoFUSSClient.getNewFileSystem());
            }

            if (code == NOFUSS_FILESYSTEM_UPDATE_ERROR) {
                Serial.print(F("[NoFUSS] File System Update Error: "));
                Serial.println(NoFUSSClient.getErrorString());
            }

            if (code == NOFUSS_FILESYSTEM_UPDATED) {
                Serial.println(F("[NoFUSS] File System Updated"));
            }

            if (code == NOFUSS_FIRMWARE_UPDATE_ERROR) {
                Serial.print(F("[NoFUSS] Firmware Update Error: "));
                Serial.println(NoFUSSClient.getErrorString());
            }

            if (code == NOFUSS_FIRMWARE_UPDATED) {
                Serial.println(F("[NoFUSS] Firmware Updated"));
            }

            if (code == NOFUSS_RESET) {
                Serial.println(F("[NoFUSS] Resetting board"));
            }

            if (code == NOFUSS_END) {
                Serial.println(F("[NoFUSS] End"));
            }

        });

    }

    void nofussLoop() {

        static unsigned long last_check = 0;
        if (!wifiConnected()) return;
        if ((last_check > 0) && ((millis() - last_check) < config.nofussInterval.toInt())) return;
        last_check = millis();
        NoFUSSClient.handle();

    }

#endif
