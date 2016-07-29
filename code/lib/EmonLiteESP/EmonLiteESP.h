/*
  EmonLiteESP

  Energy Monitor Library for ESP8266 based on EmonLib
  Currently only support current sensing
*/

#ifndef EmonLiteESP_h
#define EmonLiteESP_h

#define ADC_BITS        10
#define ADC_COUNTS      (1<<ADC_BITS)
#define COUNT_OFFSET    3
#define FILTER_SPEED    1024.0
#define WARMUP_COUNTS   3000

typedef unsigned int (*current_c)();

class EnergyMonitor {

  public:

    void initCurrent(current_c callback, double ref, double ratio);
    double getCurrent(unsigned int samples);
    byte getPrecision();
    void setPrecision(byte precision);
    void setReference(double ref);
    void setCurrentRatio(double ratio);
    void setCurrentOffset(double offset);
    void calculatePrecision();

  private:

    current_c _currentCallback;
    double _referenceVoltage;
    double _currentRatio;
    double _currentMidPoint;
    double _currentFactor;
    byte _precision;
    double _multiplier;

    void warmup();

};

#endif
