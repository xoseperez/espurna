// -----------------------------------------------------------------------------
// Stream Injector
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

        virtual void reset() {
            double last = _data->empty() ? 0 : _data->back();
            _data->clear();
            add(last);
        }

        virtual void add(double value) {
            _data->push_back(value);
        }

        virtual double median(bool do_reset = false) {

            double sum = 0;

            if (_data->size() == 1) {
                sum = _data->back();

            } else if (_data->size() == 2) {
                sum = _data->front();

            } else if (_data->size() > 2) {

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

            }

            if (do_reset) reset();

            return sum;

        }

        virtual unsigned char count() {
            return _data->size();
        }

    private:

        std::vector<double> *_data;

};
