// -----------------------------------------------------------------------------
// Abstract emon sensor class (other sensor classes extend this class)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "../sensor.h"
#include "BaseSensor.h"

template <size_t ParamDevices>
class BaseEmonSensorTemplate : public BaseSensor {

    public:

        constexpr static size_t MaxDevices = ParamDevices;

        virtual size_t countDevices() {
            return 1;
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
            if (index >= ParamDevices) return 0.0;
            return _energy[index].asDouble();
        }

        virtual double getEnergy() {
            return getEnergy(0);
        }

        virtual void setCurrentRatio(double) {}
        virtual void setVoltageRatio(double) {}
        virtual void setPowerRatio(double) {}
        virtual void setEnergyRatio(double) {}

        // when sensor implements a single device
        virtual double getCurrentRatio() { return 0.0; }
        virtual double getVoltageRatio() { return 0.0; }
        virtual double getPowerRatio() { return 0.0; }
        virtual double getEnergyRatio() { return 0.0; }

        // when sensor implements more than one device
        virtual double getCurrentRatio(unsigned char index) { return getCurrentRatio(); }
        virtual double getVoltageRatio(unsigned char index) { return getCurrentRatio(); }
        virtual double getPowerRatio(unsigned char index) { return getCurrentRatio(); }
        virtual double getEnergyRatio(unsigned char index) { return getCurrentRatio(); }

        virtual void expectedCurrent(double value) {}
        virtual void expectedVoltage(unsigned int value) {}
        virtual void expectedPower(unsigned int value) {}

        virtual void resetCalibration(double value) {}
        virtual void resetRatios() {}

    protected:

        sensor::Energy _energy[MaxDevices];

};

// Default to 4, as current sensors only implement up to 3 devices
// TODO: expose as build time flag?
using BaseEmonSensor = BaseEmonSensorTemplate<4>;

