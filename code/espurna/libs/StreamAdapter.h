/*

Arduino Stream from a generic generic byte range
Implementation of the Print is taken by reference and will be proxied

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <memory>

template <typename T>
struct StreamAdapter final : public Stream {
    StreamAdapter(Print& writer, T&& begin, T&& end) :
        _writer(writer),
        _current(std::forward<T>(begin)),
        _begin(std::forward<T>(begin)),
        _end(std::forward<T>(end))
    {}

    int available() override {
        return (_end - _current);
    }

    int peek() override {
        if (available() && (_end != (1 + _current))) {
            return *(1 + _current);
        }
        return -1;
    }

    int read() override {
        if (_end != _current) {
            return *(_current++);
        }
        return -1;
    }

    void flush() override {
        _writer.flush();
    }

    size_t write(const uint8_t* buffer, size_t size) override {
        return _writer.write(buffer, size);
    }

    size_t write(uint8_t ch) override {
        return _writer.write(ch);
    }

    private:

    Print& _writer;

    T _current;
    T const _begin;
    T const _end;
};


