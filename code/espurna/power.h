// -----------------------------------------------------------------------------
// Stream Injector
// -----------------------------------------------------------------------------

#pragma once

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

            if (_pointer > 2) {

                for (unsigned char i = 1; i<_pointer-1; i++) {

                    double previous = _data[i-1];
                    double current = _data[i];
                    double next = _data[i+1];

                    if (previous > current) std::swap(previous, current);
                    if (current > next) std::swap(current, next);
                    if (previous > current) std::swap(previous, current);

                    sum += current;

                }

                sum /= (_pointer - 1);

            }

            if (do_reset) reset();

            return sum;

        }

        virtual unsigned char count() {
            return _pointer - 1;
        }

    private:

        double *_data;
        unsigned char _size = 0;
        unsigned char _pointer = 0;

};
