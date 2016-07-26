/*
  EmonLiteESP

  Energy Monitor Library for ESP8266 based on EmonLib
  Currently only support current sensing
*/

#include "Arduino.h"
#include "EmonLiteESP.h"

void EnergyMonitor::initCurrent(current_c callback, double ref, double ratio) {

    _currentCallback = callback;
    _referenceVoltage = ref;
    _currentRatio = ratio;
    _currentMidPoint = (ADC_COUNTS>>1);

    calculatePrecision();

};

void EnergyMonitor::calculatePrecision() {
    _currentFactor = _currentRatio * _referenceVoltage / ADC_COUNTS;
    _precision = 0;
    _multiplier = 1;
    while (_multiplier * _currentFactor < 1) {
        _multiplier *= 10;
        ++_precision;
    }
    --_precision;
    _multiplier /= 10;
}

void EnergyMonitor::setReference(double ref) {
    _referenceVoltage = ref;
}

void EnergyMonitor::setCurrentRatio(double ratio) {
    _currentRatio = ratio;
}

byte EnergyMonitor::getPrecision() {
    return _precision;
}

void EnergyMonitor::setPrecision(byte precision) {
    _precision = precision;
    _multiplier = 1;
    for (byte i=0; i<_precision; i++) _multiplier *= 10;
}

double EnergyMonitor::getCurrent(unsigned int samples) {

    int sample;
    double filtered;
    double sum;

    for (unsigned int n = 0; n < samples; n++) {

        // Read analog value
        sample = _currentCallback();

        // Digital low pass filter extracts the VDC offset
        _currentMidPoint = (_currentMidPoint + (sample - _currentMidPoint) / 10.0);
        filtered = sample - _currentMidPoint;

        // Root-mean-square method
        sum += (filtered * filtered);

    }

    double rms = int(sqrt(sum / samples) - 0.5);
    double current = _currentFactor * rms;
    Serial.println(sum);
    Serial.println(rms);
    Serial.println(current);
    current = round(current * _multiplier) / _multiplier;
    Serial.println(current);
    return current;

};
