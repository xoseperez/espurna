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
    warmup();

};

void EnergyMonitor::warmup() {
    int sample;
    for (unsigned int n = 0; n < WARMUP_COUNTS; n++) {
        sample = _currentCallback();
        _currentMidPoint = (_currentMidPoint + (sample - _currentMidPoint) / ADC_COUNTS);
    }
}

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

    //Serial.print("_currentMidPoint: "); Serial.println(_currentMidPoint);

    for (unsigned int n = 0; n < samples; n++) {

        // Read analog value
        sample = _currentCallback();

        // Digital low pass filter extracts the VDC offset
        _currentMidPoint = (_currentMidPoint + (sample - _currentMidPoint) / ADC_COUNTS);
        filtered = sample - _currentMidPoint;

        // Root-mean-square method
        sum += (filtered * filtered);

    }

    double rms = sqrt(sum / samples) - COUNT_OFFSET;
    if (rms < 0) rms = 0;
    double current = _currentFactor * rms;

    /*
    Serial.print("_currentMidPoint: "); Serial.println(_currentMidPoint);
    Serial.print("sample          : "); Serial.println(sample);
    Serial.print("sum             : "); Serial.println(sum);
    Serial.print("samples         : "); Serial.println(samples);
    Serial.print("rms1            : "); Serial.println(sqrt(sum / samples));
    Serial.print("rms2            : "); Serial.println(rms);
    Serial.print("current         : "); Serial.println(current);
    */

    current = round(current * _multiplier) / _multiplier;
    return current;

};
