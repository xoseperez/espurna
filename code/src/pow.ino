/*

ESPurna
POW MODULE
Support for Sonoff POW HLW8012-based power monitor

Copyright (C) 2016 by Xose Pérez <xose dot perez at gmail dot com>

*/

#if ENABLE_POW

#include <HLW8012.h>

HLW8012 hlw8012;

// -----------------------------------------------------------------------------
// POW
// -----------------------------------------------------------------------------

// When using interrupts we have to call the library entry point
// whenever an interrupt is triggered
void hlw8012_cf1_interrupt() {
    hlw8012.cf1_interrupt();
}

void hlw8012_cf_interrupt() {
    hlw8012.cf_interrupt();
}

void powAttachInterrupts() {
    //attachInterrupt(POW_CF1_PIN, hlw8012_cf1_interrupt, CHANGE);
    attachInterrupt(POW_CF_PIN, hlw8012_cf_interrupt, CHANGE);
    DEBUG_MSG("[POW] Enabled\n");
}

void powDettachInterrupts() {
    //detachInterrupt(POW_CF1_PIN);
    detachInterrupt(POW_CF_PIN);
    DEBUG_MSG("[POW] Disabled\n");
}

void powSaveCalibration() {
    setSetting("powPowerMult", String() + hlw8012.getPowerMultiplier());
    setSetting("powCurrentMult", String() + hlw8012.getCurrentMultiplier());
    setSetting("powVoltageMult", String() + hlw8012.getVoltageMultiplier());
}

void powRetrieveCalibration() {
    double value;
    value = getSetting("powPowerMult", "0").toFloat();
    if (value > 0) hlw8012.setPowerMultiplier((int) value);
    value = getSetting("powCurrentMult", "0").toFloat();
    if (value > 0) hlw8012.setCurrentMultiplier((int) value);
    value = getSetting("powVoltageMult", "0").toFloat();
    if (value > 0) hlw8012.setVoltageMultiplier((int) value);
}

void powSetExpectedActivePower(unsigned int power) {
    if (power > 0) {
        hlw8012.expectedActivePower(power);
        powSaveCalibration();
    }
}

void powSetExpectedCurrent(double current) {
    if (current > 0) {
        hlw8012.expectedCurrent(current);
        powSaveCalibration();
    }
}

void powSetExpectedVoltage(unsigned int voltage) {
    if (voltage > 0) {
        hlw8012.expectedVoltage(voltage);
        powSaveCalibration();
    }
}

unsigned int getActivePower() {
    return hlw8012.getActivePower();
}

unsigned int getApparentPower() {
    return hlw8012.getApparentPower();
}

double getCurrent() {
    return hlw8012.getCurrent();
}

unsigned int getVoltage() {
    return hlw8012.getVoltage();
}

unsigned int getPowerFactor() {
    return (int) (100 * hlw8012.getPowerFactor());
}

void powSetup() {

    // Initialize HLW8012
    // void begin(unsigned char cf_pin, unsigned char cf1_pin, unsigned char sel_pin, unsigned char currentWhen = HIGH, bool use_interrupts = false, unsigned long pulse_timeout = PULSE_TIMEOUT);
    // * cf_pin, cf1_pin and sel_pin are GPIOs to the HLW8012 IC
    // * currentWhen is the value in sel_pin to select current sampling
    // * set use_interrupts to true to use interrupts to monitor pulse widths
    // * leave pulse_timeout to the default value, recommended when using interrupts
    hlw8012.begin(POW_CF_PIN, POW_CF1_PIN, POW_SEL_PIN, POW_SEL_CURRENT, true);

    // These values are used to calculate current, voltage and power factors as per datasheet formula
    // These are the nominal values for the Sonoff POW resistors:
    // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese resistor in series with the main line
    // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
    // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012
    hlw8012.setResistors(POW_CURRENT_R, POW_VOLTAGE_R_UP, POW_VOLTAGE_R_DOWN);

    powRetrieveCalibration();

    static WiFiEventHandler e1 = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event) {
        powDettachInterrupts();
    });

    static WiFiEventHandler e2 = WiFi.onSoftAPModeStationDisconnected([](const WiFiEventSoftAPModeStationDisconnected& event) {
        powDettachInterrupts();
    });

    static WiFiEventHandler e3 = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected& event) {
        powAttachInterrupts();
    });

    static WiFiEventHandler e4 = WiFi.onSoftAPModeStationConnected([](const WiFiEventSoftAPModeStationConnected& event) {
        powAttachInterrupts();
    });

}

void powLoop() {

    static unsigned long last_update = 0;
    static unsigned char report_count = POW_REPORT_EVERY;

    if ((millis() - last_update > POW_UPDATE_INTERVAL) || (last_update == 0 )){
        last_update = millis();

        unsigned int power = getActivePower();

        char buffer[100];
        sprintf_P(buffer, PSTR("{\"powVisible\": 1, \"powActivePower\": %d}"), power);
        wsSend(buffer);

        if (--report_count == 0) {
            mqttSend((char *) getSetting("powPowerTopic", POW_POWER_TOPIC).c_str(), (char *) String(power).c_str());
            report_count = POW_REPORT_EVERY;
        }

    }

}

#endif
