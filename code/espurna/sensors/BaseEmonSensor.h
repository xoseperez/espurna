// -----------------------------------------------------------------------------
// Abstract emon sensor class (other sensor classes extend this class)
// Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>
// -----------------------------------------------------------------------------

#pragma once

#include "BaseSensor.h"

class BaseEmonSensor : public BaseSensor {
public:
    static const BaseSensor::ClassKind Kind;
    BaseSensor::ClassKind kind() const override {
        return Kind;
    }

protected:
    struct EnergyMapping {
        unsigned char index;
        espurna::sensor::Energy energy;
    };

    struct EnergyIndex {
        using Entries = std::vector<EnergyMapping>;

        template <typename T>
        void update(unsigned char index, T&& callback) {
            process(_entries, index, std::forward<T>(callback));
        }

        template <typename T>
        void find(unsigned char index, T&& callback) const {
            process(_entries, index, std::forward<T>(callback));
        }

        espurna::sensor::Energy& operator[](size_t index) {
            return _entries[index].energy;
        }

        void add(unsigned char index) {
            _entries.push_back({index, {}});
        }

        void reset() {
            for (auto& entry : _entries) {
                entry.energy.reset();
            }
        }

        espurna::sensor::Energy frontTotal() const {
            if (_entries.size()) {
                return _entries.front().energy;
            }

            return {};
        }

        size_t size() const {
            return _entries.size();
        }

        const EnergyMapping& front() const {
            return _entries.front();
        }

    private:
        template <typename T, typename Callback>
        static void process(T&& entries, unsigned char index, Callback&& callback) {
            auto it = std::find_if(entries.begin(), entries.end(), [&](const EnergyMapping& mapping) {
                return index == mapping.index;
            });

            if (it != entries.end()) {
                callback(*it);
            }
        }

        Entries _entries;
    };

    // TODO: Updated BaseEmonSensor no longer generates at least 1 slot for energy b/c Magnitudes
    // container is not accessible unless it is visible through virtual method or ctpr (ref. i2c class)
    // And that means trying to work with _energy[0] will crash accessing not-yet-initialized vector
    // - One option is to make this a class type property, where the container becomes tuple of magnitude types
    // Proxying typedefs and members though inheritance is not always obvious, though. But, it might help with
    // _id and _count members of the BaseSensor
    // - Another is to return begin and end as virtual methods... but, that is again a runtime access that should
    // be implemented by the child class

    template <size_t Size>
    BaseEmonSensor(const Magnitude (&container)[Size]) {
        findMagnitudes(container, MAGNITUDE_ENERGY, [&](unsigned char index) {
            _energy.add(index);
        });
    }

public:
    virtual void resetEnergy(unsigned char index, espurna::sensor::Energy energy) {
        _energy.update(index, [&](EnergyMapping& entry) {
            entry.energy = energy;
        });
    }

    virtual void resetEnergy(unsigned char index) {
        _energy.update(index, [](EnergyMapping& entry) {
            entry.energy.reset();
        });
    };

    virtual void resetEnergy() {
        _energy.reset();
    }

    virtual espurna::sensor::Energy totalEnergy(unsigned char index) const {
        espurna::sensor::Energy out{};
        _energy.find(index, [&](const EnergyMapping& mapping) {
            out = mapping.energy;
        });

        return out;
    }

    // when sensor implements scalable magnitudes

    // Generic ratio configuration, default is a no-op and must be implemented by the sensor class
    static constexpr double DefaultRatio { 1.0 };
    virtual double defaultRatio(unsigned char index) const {
        return DefaultRatio;
    }

    virtual double getRatio(unsigned char index) const {
        return defaultRatio(index);
    }

    template <size_t Size>
    double simpleGetRatio(const Magnitude (&magnitudes)[Size] , unsigned char index) const {
        if (index < Size) {
            switch (magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                return _current_ratio;
            case MAGNITUDE_VOLTAGE:
                return _voltage_ratio;
            case MAGNITUDE_POWER_ACTIVE:
                return _power_active_ratio;
            case MAGNITUDE_POWER_REACTIVE:
                return _power_reactive_ratio;
            case MAGNITUDE_ENERGY:
                return _energy_ratio;
            }
        }

        return defaultRatio(index);
    }

    virtual void setRatio(unsigned char index, double value) {
        // pass by default, no point updating ratios when they
        // are not supported (or cannot be supported)
    }

    template <size_t Size>
    void simpleSetRatio(const Magnitude (&magnitudes)[Size] , unsigned char index, double value) {
        if (index < Size) {
            switch (magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                _current_ratio = value;
                break;
            case MAGNITUDE_VOLTAGE:
                _voltage_ratio = value;
                break;
            case MAGNITUDE_POWER_ACTIVE:
                _power_active_ratio = value;
                break;
            case MAGNITUDE_POWER_REACTIVE:
                _power_reactive_ratio = value;
                break;
            case MAGNITUDE_ENERGY:
                _power_reactive_ratio = value;
                break;
            }
        }
    }

    virtual void resetRatios() {
        _current_ratio = DefaultRatio;
        _voltage_ratio = DefaultRatio;
        _power_active_ratio = DefaultRatio;
        _power_reactive_ratio = DefaultRatio;
        _energy_ratio = DefaultRatio;
    }

    // sometimes we allow ratio calculation from specific value
    virtual double ratioFromValue(unsigned char index, double value, double expected) const {
        if ((value > 0.0) && (expected > 0.0)) {
            return getRatio(index) * (expected / value);
        }

        return getRatio(index);
    }

protected:
    double _current_ratio { DefaultRatio };
    double _voltage_ratio { DefaultRatio };
    double _power_active_ratio { DefaultRatio };
    double _power_reactive_ratio { DefaultRatio };
    double _energy_ratio { DefaultRatio };
    EnergyIndex _energy{};
};

const BaseSensor::ClassKind BaseEmonSensor::Kind;
