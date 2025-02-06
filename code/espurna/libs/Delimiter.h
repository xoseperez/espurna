#pragma once

#include "../types.h"

#include <array>

namespace espurna {

// Buffer char data and yield portion of the data when the received value has specific char
// storage works like a circular buffer; whenever buffer size exceedes capacity, we return
// to the start of the buffer and reset size.
// when buffer overflows, store internal flag until the storage is reset to the default state
struct DelimiterBuffer {
    // **only valid until the next append()**
    struct Result {
        StringView value;
        bool overflow;
    };

    DelimiterBuffer() = delete;

    explicit DelimiterBuffer(char* storage, size_t capacity);
    explicit DelimiterBuffer(char* storage, size_t capacity, StringView delimiter);

    Result next();

    void reset() {
        _overflow = false;
        _cursor = 0;
        _size = 0;
    }

    size_t capacity() const {
        return _capacity;
    }

    size_t size() const {
        return _size;
    }

    bool overflow() const {
        return _overflow;
    }

    void append(const char*, size_t);

    void append(StringView value) {
        append(value.c_str(), value.length());
    }

    void append(Stream&, size_t);

    void append(Stream& stream) {
        const auto available = stream.available();
        if (available > 0) {
            append(stream, static_cast<size_t>(available));
        }
    }

    void append(char value) {
        append(&value, 1);
    }

    StringView get() const {
        return StringView{&_storage[_cursor], &_storage[_size]};
    }

private:
    char* _storage;
    size_t _capacity { 0 };
    size_t _size { 0 };
    size_t _cursor { 0 };

    StringView _delimiter;

    bool _overflow { false };
};

namespace line_buffer_impl {

template <size_t Capacity>
struct Storage {
    char* data() {
        return _storage.data();
    }

    size_t size() const {
        return _storage.size();
    }

    static constexpr size_t capacity() {
        return Capacity;
    }

private:
    std::array<char, Capacity> _storage{};
};

struct Base : public DelimiterBuffer {
    using Result = DelimiterBuffer::Result;

    Base(char*, size_t);
    Result next();
};

} // namespace line_buffer_impl

// Helper storage class to contain a fixed-size buffer on top of a delimited buffer.
// ::next() handles both '\n' and '\r\n' as line delimiters
template <size_t Capacity>
struct LineBuffer : public line_buffer_impl::Storage<Capacity>, public line_buffer_impl::Base {
    LineBuffer() :
        line_buffer_impl::Storage<Capacity>(),
        line_buffer_impl::Base(data(), capacity())
    {}

    using line_buffer_impl::Storage<Capacity>::capacity;

    using line_buffer_impl::Base::append;
    using line_buffer_impl::Base::get;
    using line_buffer_impl::Base::next;
    using line_buffer_impl::Base::overflow;
    using line_buffer_impl::Base::size;

private:
    using line_buffer_impl::Storage<Capacity>::data;
};

// Similar to delimited buffer, but instead work on an already existing string
// and yield these stringview chunks on each call to next()
struct DelimiterView {
    DelimiterView() = delete;

    explicit DelimiterView(StringView view, StringView delimiter) :
        _view(view),
        _delimiter(delimiter)
    {}

    StringView next();

    explicit operator bool() const {
        return _cursor != _view.length();
    }

    const char* begin() const {
        return _view.begin() + _cursor;
    }

    const char* end() const {
        return _view.end();
    }

    size_t length() const {
        return std::distance(begin(), end());
    }

    StringView get() const {
        return StringView{begin(), end()};
    }

private:
    StringView _view;
    StringView _delimiter;

    size_t _cursor { 0 };
};

// Same as a buffered variant, ::next() handles both '\n' and '\r\n' as line delimiters
struct LineView : public DelimiterView {
    explicit LineView(StringView);
    StringView next();
};

struct SplitView {
    explicit SplitView(StringView);
    SplitView(StringView, StringView);

    StringView remaining() const {
        return _remaining;
    }

    StringView current() const {
        return _current;
    }

    bool next();

private:
    void remaining(const char*);
    void reset();

    StringView _remaining;
    StringView _delimiter;

    StringView _current;
};

} // namespace espurna
