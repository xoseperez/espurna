/*

ESPurna
RF MODULE

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_RF

    #include <RemoteReceiver.h>

    unsigned long rfCode = 0;
    unsigned long rfCodeON = 0;
    unsigned long rfCodeOFF = 0;

    // -----------------------------------------------------------------------------
    // RF
    // -----------------------------------------------------------------------------

    void rfEnable(bool enable) {
        if (enable) {
            RemoteReceiver::enable();
        } else {
            RemoteReceiver::disable();
        }
    }
    
    void rfLoop() {
        if (rfCode == 0) return;
        #ifdef DEBUG
            Serial.print(F("[RF] Received code: "));
            Serial.println(rfCode);
        #endif
        if (rfCode == rfCodeON) switchRelayOn();
        if (rfCode == rfCodeOFF) switchRelayOff();
        rfCode = 0;
    }

    void rfBuildCodes() {

        unsigned long code = 0;

        // channel
        unsigned int channel = config.rfChannel.toInt();
        for (byte i = 0; i < 5; i++) {
            code *= 3;
            if (channel & 1) code += 1;
            channel >>= 1;
        }

        // device
        unsigned int device = config.rfDevice.toInt();
        for (byte i = 0; i < 5; i++) {
            code *= 3;
            if (device != i) code += 2;
        }

        // status
        code *= 9;
        rfCodeOFF = code + 2;
        rfCodeON = code + 6;

        #ifdef DEBUG
            Serial.print(F("[RF] Code ON: "));
            Serial.println(rfCodeON);
            Serial.print(F("[RF] Code OFF: "));
            Serial.println(rfCodeOFF);
        #endif

    }

    void rfCallback(unsigned long code, unsigned int period) {
        rfCode = code;
    }

    void rfSetup() {
        pinMode(RF_PIN, INPUT_PULLUP);
        rfBuildCodes();
        RemoteReceiver::init(RF_PIN, 3, rfCallback);
        RemoteReceiver::enable();
    }

#endif
