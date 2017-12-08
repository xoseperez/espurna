// -----------------------------------------------------------------------------
// Median Filter
// -----------------------------------------------------------------------------

#pragma once

class MedianFilter {

    public:

        MedianFilter() {
            _data = new std::vector<double>();
        }

        ~MedianFilter() {
            if (_data) delete _data;
        }

        virtual void add(double value) {
            _data->push_back(value);
        }

        virtual unsigned char count() {
            return _data->size();
        }

        virtual void reset() {
            double last = _data->empty() ? 0 : _data->back();
            _data->clear();
            add(last);
        }

        virtual double max() {
            double max = 0;
            for (unsigned char i = 1; i < _data->size(); i++) {
                if (max < _data->at(i)) max = _data->at(i);
            }
            return max;
        }

        virtual double result(bool do_reset = false) {

            double sum = 0;

            if (_data->size() > 2) {

                for (unsigned char i = 1; i <= _data->size() - 2; i++) {

                    double previous = _data->at(i-1);
                    double current = _data->at(i);
                    double next = _data->at(i+1);

                    if (previous > current) std::swap(previous, current);
                    if (current > next) std::swap(current, next);
                    if (previous > current) std::swap(previous, current);

                    sum += current;

                }

                sum /= (_data->size() - 2);

            } else if (_data->size() > 0) {

                sum = _data->front();

            }

            if (do_reset) reset();

            return sum;

        }

    private:

        std::vector<double> *_data;

};
