// -----------------------------------------------------------------------------
// Aggregator base class
// -----------------------------------------------------------------------------

#pragma once

#include <vector>

class AggregatorBase {

    public:

        AggregatorBase() {
            _data = new std::vector<double>();
        }

        ~AggregatorBase() {
            if (_data) delete _data;
        }

        virtual void add(double value) {
            _data->push_back(value);
        }

        virtual unsigned char count() {
            return _data->size();
        }

        virtual void reset() {
            _data->clear();
        }

        virtual double max() {
            double max = 0;
            for (unsigned char i = 1; i < _data->size(); i++) {
                if (max < _data->at(i)) max = _data->at(i);
            }
            return max;
        }

        virtual double result() {
            return 0;
        }

    protected:

        std::vector<double> *_data;

};
