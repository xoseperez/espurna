// -----------------------------------------------------------------------------
// Stream Injector
// -----------------------------------------------------------------------------

#pragma once

/*
class SpikesFilter {

    public:

        SpikesFilter() {
            reset();
        }

        virtual void reset() {
            _sum = 0;
            _spike = false;
        }

        virtual void add(double value) {

            // add previous value
            if (_last > 0) {
                _sum += _last;
            }

            // flag new possible spike
            if (value > 0) {
                _spike = (_last == 0);

            // delete previous spike
            } else if (_spike) {
                _sum -= _last;
                _spike = false;
            }

            _last = value;

        }

        virtual double sum() {
            return _sum;
        }

    private:

        double _last = 0;
        double _sum = 0;
        bool _spike = false;

};
*/

class MedianFilter {

    public:

        MedianFilter(unsigned char size) {
            _size = size;
            _data = new double[_size+1];
            reset();
        }

        virtual void reset() {
            _data[0] = _data[_size];
            _pointer = 1;
            for (unsigned char i=_pointer; i<=_size; i++) _data[i] = 0;
        }

        virtual void add(double value) {
            if (_pointer <= _size) {
                _data[_pointer] = value;
                ++_pointer;
            }
        }

        virtual double average(bool do_reset = false) {

            double sum = 0;
            for (unsigned char i = 1; i<_size; i++) {

                double previous = _data[i-1];
                double current = _data[i];
                double next = _data[i+1];

                if (previous > current) std::swap(previous, current);
                if (current > next) std::swap(current, next);
                if (previous > current) std::swap(previous, current);

                sum += current;

            }

            if (do_reset) reset();

            return sum / (_size-1);

        }

        virtual unsigned char count() {
            return _pointer - 1;
        }

    private:

        double *_data;
        unsigned char _size = 0;
        unsigned char _pointer = 0;

};
