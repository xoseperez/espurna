// -----------------------------------------------------------------------------
// Abstract emon sensor class (other sensor classes extend this class)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "../sensor.h"
#include "BaseSensor.h"

class BaseEmonSensor : public BaseSensor {

    public:

        BaseEmonSensor(size_t devices) :
            _energy(devices),
            _devices(devices)
        {}

        BaseEmonSensor() :
            BaseEmonSensor(1)
        {}

        unsigned char type() {
            return sensor::type::Emon;
        }

        // --- energy monitoring --

        virtual void resizeDevices(size_t devices) {
            _energy.resize(devices);
            _devices = devices;
        }

        virtual size_t countDevices() {
            return _devices;
        }

        virtual void resetEnergy(unsigned char index, sensor::Energy energy) {
            _energy[index] = energy;
        }

        virtual void resetEnergy(unsigned char index) {
            _energy[index].reset();
        };

        virtual void resetEnergy() {
            for (auto& energy : _energy) {
                energy.reset();
            }
        }

        virtual sensor::Energy totalEnergy(unsigned char index) {
            return _energy[index];
        }

        virtual sensor::Energy totalEnergy() {
            return totalEnergy(0);
        }

        virtual double getEnergy(unsigned char index) {
            return _energy[index].asDouble();
        }

        virtual double getEnergy() {
            return getEnergy(0);
        }

        // --- configuration ---

        virtual double defaultVoltage() {
            return 0.0;
        }
        
        // when sensor needs explicit mains voltage value
        virtual void setVoltage(double) {}
        virtual void setVoltage(unsigned char index, double value) {
            setVoltage(value);
        }
        virtual double getVoltage() {
            return defaultVoltage();
        }
        virtual double getVoltage(unsigned char index) {
            return getVoltage();
        }

        // when sensor implements ratios / multipliers
        virtual void setCurrentRatio(double) {}
        virtual void setVoltageRatio(double) {}
        virtual void setPowerRatio(double) {}
        virtual void setEnergyRatio(double) {}

        // when sensor implements ratios / multipliers
        virtual void setCurrentRatio(unsigned char index, double value) {
            setCurrentRatio(value);
        }
        virtual void setVoltageRatio(unsigned char index, double value) {
            setVoltageRatio(value);
        }
        virtual void setPowerRatio(unsigned char index, double value) {
            setPowerRatio(value);
        }
        virtual void setEnergyRatio(unsigned char index, double value) {
            setEnergyRatio(value);
        }

        // Generic ratio configuration, default is a
        // no-op and must be implemented by the sensor class
        virtual double defaultCurrentRatio() const {
            return 0.0;
        }
        virtual double defaultVoltageRatio() const {
            return 0.0;
        }
        virtual double defaultPowerRatio() const {
            return 0.0;
        }
        virtual double defaultEnergyRatio() const {
            return 0.0;
        }

        // when sensor implements a single device
        virtual double getCurrentRatio() {
            return defaultCurrentRatio();
        }
        virtual double getVoltageRatio() {
            return defaultVoltageRatio();
        }
        virtual double getPowerRatio() {
            return defaultPowerRatio();
        }
        virtual double getEnergyRatio() {
            return defaultEnergyRatio();
        }

        // when sensor implements more than one device
        virtual double getCurrentRatio(unsigned char index) {
            return getCurrentRatio();
        }
        virtual double getVoltageRatio(unsigned char index) {
            return getVoltageRatio();
        }
        virtual double getPowerRatio(unsigned char index) {
            return getPowerRatio();
        }
        virtual double getEnergyRatio(unsigned char index) {
            return getEnergyRatio();
        }

        virtual void expectedCurrent(double value) {}
        virtual void expectedVoltage(unsigned int value) {}
        virtual void expectedPower(unsigned int value) {}

        virtual void expectedCurrent(unsigned char index, double value) {
            expectedCurrent(value);
        }
        virtual void expectedVoltage(unsigned char index, unsigned int value) {
            expectedVoltage(value);
        }
        virtual void expectedPower(unsigned char index, unsigned int value) {
            expectedPower(value);
        }

        virtual void resetRatios() {}

    protected:

        std::vector<sensor::Energy> _energy;
        size_t _devices;

};

