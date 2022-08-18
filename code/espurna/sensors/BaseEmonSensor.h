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
    // Forcibly set energy value at the specified magnitude index
    virtual void resetEnergy(unsigned char index, espurna::sensor::Energy energy) {
        _energy.update(index, [&](EnergyMapping& entry) {
            entry.energy = energy;
        });
    }

    // Initial energy value, should **only** be called once
    // In case sensor does not want to implement resets
    virtual void initialEnergy(unsigned char index, espurna::sensor::Energy energy) {
        resetEnergy(index, energy);
    }

    // Forcibly set energy value to 0 at the specified magnitude index
    virtual void resetEnergy(unsigned char index) {
        _energy.update(index, [](EnergyMapping& entry) {
            entry.energy.reset();
        });
    };

    // Forciby set energy value to 0 for **all** energy magnitudes
    // (**only** if initialized by the sensor class)
    virtual void resetEnergy() {
        _energy.reset();
    }

    // Retrieve energy value at the specified magnitude index
    // Usually, this value is accumulated for the duration of the sensors lifetime
    virtual espurna::sensor::Energy totalEnergy(unsigned char index) const {
        espurna::sensor::Energy out{};
        _energy.find(index, [&](const EnergyMapping& mapping) {
            out = mapping.energy;
        });

        return out;
    }

    // ------------------------------------------------------------------------

    // Generic ratio configuration, default is a no-op and must be implemented by the sensor class
    static constexpr double DefaultRatio { 1.0 };

    // Default ratio value for the magnitude slot
    virtual double defaultRatio(unsigned char index) const {
        return DefaultRatio;
    }

    // Current ratio value for the magnitude slot
    virtual double getRatio(unsigned char index) const {
        return defaultRatio(index);
    }

    // Update ratio value for the magnitude slot
    virtual void setRatio(unsigned char index, double value) {
        // pass by default, no point updating ratios when they
        // are not supported (or cannot be supported)
    }

    // Reset *all* magnitude ratios to sensor defaults
    virtual void resetRatios() {
        _current_ratio = DefaultRatio;
        _voltage_ratio = DefaultRatio;
        _power_active_ratio = DefaultRatio;
        _power_reactive_ratio = DefaultRatio;
        _energy_ratio = DefaultRatio;
        _ratios_changed = true;
    }

    // Return ratio value for the slot, that would make the current value closer to the expected one
    virtual double ratioFromValue(unsigned char index, double value, double expected) const {
        if ((value > 0.0) && (expected > 0.0)) {
            return getRatio(index) * (expected / value);
        }

        return getRatio(index);
    }

    // helper methods to either get or set our internal ratio values
    // notice that only a single magnitude-index <-> ratio is supported

    template <size_t Size>
    double simpleGetRatio(const Magnitude (&magnitudes)[Size] , unsigned char index) const {
        const auto* ratio = magnitudeRatio(magnitudes, index);
        if (ratio) {
            return *ratio;
        }

        return defaultRatio(index);
    }

    template <size_t Size>
    void simpleSetRatio(const Magnitude (&magnitudes)[Size], unsigned char index, double value) {
       auto* ratio = magnitudeRatio(magnitudes, index);
       if (ratio) {
           if (!almostEqual(*ratio, value)) {
               _ratios_changed = true;
           }
           *ratio = value;
       }
    }

private:
    template <size_t Size>
    const double* magnitudeRatio(const Magnitude (&magnitudes)[Size], unsigned char index) const {
        return magnitudeRatioImpl<const double*>(*this, magnitudes, index);
    }

    template <size_t Size>
    double* magnitudeRatio(const Magnitude (&magnitudes)[Size], unsigned char index) {
        return magnitudeRatioImpl<double*>(*this, magnitudes, index);
    }

    template <typename Result, typename T, size_t Size>
    static Result magnitudeRatioImpl(T& instance, const Magnitude (&magnitudes)[Size], unsigned char index) {
        if (index < Size) {
            switch (magnitudes[index].type) {
            case MAGNITUDE_CURRENT:
                return &(instance._current_ratio);
            case MAGNITUDE_VOLTAGE:
                return &(instance._voltage_ratio);
            case MAGNITUDE_POWER_ACTIVE:
                return &(instance._power_active_ratio);
            case MAGNITUDE_POWER_REACTIVE:
                return &(instance._power_reactive_ratio);
            case MAGNITUDE_ENERGY:
                return &(instance._energy_ratio);
            }
        }

        return nullptr;
    }

protected:
    bool _ratios_changed = false;

    double _current_ratio { DefaultRatio };
    double _voltage_ratio { DefaultRatio };
    double _power_active_ratio { DefaultRatio };
    double _power_reactive_ratio { DefaultRatio };
    double _energy_ratio { DefaultRatio };

    EnergyIndex _energy{};
};

const BaseSensor::ClassKind BaseEmonSensor::Kind;
