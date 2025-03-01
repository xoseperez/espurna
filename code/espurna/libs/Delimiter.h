#pragma once

#include "../types.h"

#include <array>
#include <iterator>

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

// Helper class & iterator handler for generic delimiter-based parsing
struct StatefulSplitView;

// Base use-case, wrap the view and provide iterator interface
struct SplitView {
    struct Iterator {
        struct End {
        };

        using iterator_category = std::forward_iterator_tag;
        using value_type = StringView;

        using difference_type = void;
        using pointer_type = void;
        using reference_type = void;

        value_type operator*() const {
            return _value;
        }

        Iterator& operator++();
        Iterator operator++(int);

        bool operator==(const Iterator&) const;
        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        bool operator==(const End&) const;
        bool operator!=(const End& other) const {
            return !(*this == other);
        }

    private:
        friend SplitView;
        friend StatefulSplitView;

        StringView after_delimiter(const char*) const;
        StringView remaining() const;

        void reset();
        bool init();
        bool next();

        Iterator() = delete;
        explicit Iterator(const SplitView*);

        const SplitView* _base;
        StringView _value;
    };

    explicit SplitView(StringView);
    SplitView(StringView, StringView);

    Iterator begin() const;

    Iterator::End end() const {
        return SplitView::Iterator::End{};
    }

private:
    friend StatefulSplitView;

    StringView before_begin() const {
        return StringView(
            _view.begin() - _delimiter.length(),
            _view.begin() - _delimiter.length());
    }

    StringView _view;
    StringView _delimiter;
};

// Sometimes wrapped iterator is preferred over for(...)
struct StatefulSplitView {
    explicit StatefulSplitView(StringView view) :
        _base(view),
        _iterator(&_base)
    {}

    StatefulSplitView(StringView view, StringView delimiter) :
        _base(view, delimiter),
        _iterator(&_base)
    {}

    bool next() {
        return _iterator.next();
    }

    StringView current() {
        return *_iterator;
    }

    StringView remaining() {
        return _iterator.remaining();
    }

    SplitView::Iterator begin() const {
        return _base.begin();
    }

    SplitView::Iterator::End end() const {
        return _base.end();
    }

    SplitView::Iterator find(StringView) const;

private:
    SplitView _base;
    SplitView::Iterator _iterator;
};

} // namespace espurna
