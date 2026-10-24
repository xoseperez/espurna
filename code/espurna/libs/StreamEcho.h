/*

Arduino Stream holding both input and output. Currently, only used in tests.

*/

#pragma once

#include <Arduino.h>

#include <limits>
#include <vector>

class StreamEcho : public Stream {
public:
    size_t write(uint8_t c) override {
        return _write(&c, 1);
    }

    size_t write(const uint8_t* data, size_t size) override {
        return _write(data, size);
    }

    int availableForWrite() override {
        return std::numeric_limits<int>::max();
    }

    void flush() override {
        _data.clear();
    }

    int available() override {
        return _size();
    }

    int read() override {
        if (_data.size()) {
            auto it = _data.cbegin();

            auto out = *it;
            _data.erase(it);

            return out;
        }

        return -1;
    }

    int read(uint8_t* data, size_t size) override {
        if (_data.size()) {
            return _read(data, size);
        }

        return -1;
    }

    size_t readBytes(uint8_t* data, size_t size) override {
        return _read(data, size);
    }

    size_t readBytes(char* data, size_t size) override {
        return _read(reinterpret_cast<uint8_t*>(data), size);
    }

    bool hasPeekBufferAPI() const override {
        return true;
    }

    int peek() override {
        if (_data.size()) {
            return _data.front();
        }

        return -1;
    }

    void peekConsume(size_t consume) override {
        _consume(consume);
    }

    size_t peekAvailable() override {
        return _size();
    }

    const char* peekBuffer() override {
        return reinterpret_cast<const char*>(_data.data());
    }

    bool inputCanTimeout() override {
        return false;
    }

    bool outputCanTimeout() override {
        return false;
    }

private:
    size_t _size() const {
        return _data.size();
    }

    size_t _read(uint8_t* data, size_t size) {
        const auto need = std::min(size, _data.size());

        std::memcpy(data, _data.data(), need);
        _data.erase(_data.begin(), _data.begin() + need);

        return need;
    }

    size_t _write(const uint8_t* data, size_t size) {
        _data.insert(_data.end(), data, data + size);
        return size;
    }

    void _consume(size_t size) {
        const auto need = std::min(_data.size(), size);
        if (!need) {
            return;
        }

        _data.erase(_data.begin(), _data.begin() + need);

    }

    std::vector<uint8_t> _data;
};

