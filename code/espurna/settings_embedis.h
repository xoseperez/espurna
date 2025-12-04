/*

Part of the SETTINGS MODULE

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

Reimplementation of the Embedis storage format:
- https://github.com/thingSoC/embedis

*/

#pragma once

#include <Arduino.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "types.h"
#include "settings_helpers.h"
#include "libs/TypeChecks.h"

namespace espurna {
namespace settings {
namespace embedis {
namespace {

// Sum total is calculated from:
// - 4 bytes to store length of 2 values (stored as big-endian)
// - N bytes of values themselves
// We can't save empty keys but can save empty values as just 2 length bytes
inline size_t estimate(StringView key, StringView value) {
    if (!key.length()) {
        return 0;
    }

    return (4 + key.length() + value.length());
}

// Most of the time we are comparing user strings with storage blobs, which are also just strings
inline bool operator==(Span<const uint8_t> lhs, StringView rhs) {
    const auto view = StringView(reinterpret_cast<const char*>(lhs.data()), lhs.size());
    return view == rhs;
}

inline bool operator==(std::vector<uint8_t> lhs, StringView rhs) {
    const auto view = StringView(reinterpret_cast<const char*>(lhs.data()), lhs.size());
    return view == rhs;
}

// Note: KeyValueStore is templated to avoid having to provide RawStorageBase via virtual inheritance.

template <typename RawStorageBase>
struct KeyValueStoreTraits {
private:
    template <typename T>
    using storage_can_commit_base = decltype(std::declval<T>().commit());

    template <typename T>
    using storage_can_commit_type = is_detected<storage_can_commit_base, T>;

    template <typename T>
    using storage_can_fill_base = decltype(std::declval<T>().fill(
        std::declval<uint16_t>(), std::declval<size_t>(), std::declval<uint8_t>()));

    template <typename T>
    using storage_can_fill_type = is_detected<storage_can_fill_base, T>;

    template <typename T>
    using storage_can_data_span_base = decltype(std::declval<T>().data(
        std::declval<uint16_t>(), std::declval<size_t>()));

    template <typename T>
    using storage_can_data_span_type = is_detected<storage_can_data_span_base, T>;

    template <typename T>
    using storage_can_read_byte_base = decltype(std::declval<T>().read(
        std::declval<uint16_t>()));

    template <typename T>
    using storage_can_read_byte_type = is_detected<storage_can_read_byte_base, T>;

    template <typename T>
    using storage_can_read_vector_base = decltype(std::declval<T>().read(
        std::declval<uint16_t>(), std::declval<size_t>()));

    template <typename T>
    using storage_can_read_vector_type = is_detected<storage_can_read_vector_base, T>;

    template <typename T>
    using storage_can_write_base = decltype(std::declval<T>().write(
        std::declval<uint16_t>(), std::declval<Span<const uint8_t>>()));

    template <typename T>
    using storage_can_write_type = is_detected<storage_can_write_base, T>;

public:
    using storage_can_commit =
        std::integral_constant<bool, storage_can_commit_type<RawStorageBase>::value>;

    using storage_can_fill =
        std::integral_constant<bool, storage_can_fill_type<RawStorageBase>::value>;

    using storage_can_data_span =
        std::integral_constant<bool, storage_can_data_span_type<RawStorageBase>::value>;

    using storage_can_read_byte =
        std::integral_constant<bool, storage_can_read_byte_type<RawStorageBase>::value>;

    using storage_can_read_vector =
        std::integral_constant<bool, storage_can_read_vector_type<RawStorageBase>::value>;

    using storage_can_write =
        std::integral_constant<bool, storage_can_write_type<RawStorageBase>::value>;
};

} // namespace

template <typename RawStorageBase>
class KeyValueStore {
private:
    using Traits = KeyValueStoreTraits<RawStorageBase>;

    static_assert(
        Traits::storage_can_write::value,
        "Storage class must implement `void write(uint16_t, Span<const uint8_t>)'"
    );

    static_assert(
        Traits::storage_can_read_byte::value,
        "Storage class must implement `uint8_t read(uint16_t)'"
    );

    static_assert(
        Traits::storage_can_read_vector::value || Traits::storage_can_data_span::value,
        "Storage class must implement `Span<const uint8_t> read(uint16_t, size_t)' or `data(uint16_t, size_t)'"
    );

    static_assert(
        Traits::storage_can_commit::value,
        "Storage class must implement `void commit()'"
    );

    static_assert(
        Traits::storage_can_fill::value,
        "Storage class must implement `void fill(uint16_t, size_t, uint8_t)'"
    );

    // data read / write methods differ and depend on available RawStorageBase functionality

    template <typename D, typename S>
    static void write_impl(std::true_type, std::true_type, D& dst, const S& src, size_t index) {
        dst.write(index, src.data());
    }

    template <typename D, typename S>
    static void write_impl(std::true_type, std::false_type, D& dst, const S& src, size_t index) {
        dst.write(index, src.data());
    }

    template <typename D, typename S>
    static void write_impl(std::false_type, std::true_type, D& dst, const S& src, size_t index) {
        const auto data = src.read(src.size());
        dst.write(index, Span<const uint8_t>(data.data(), data.size()));
    }

    template <typename S>
    static void read_impl(std::true_type, Span<const uint8_t>& out, S& src, size_t index, size_t size) {
        out = src.data(index, size);
    }

    template <typename S>
    static void read_impl(std::false_type, std::true_type, std::vector<uint8_t>& out, S& src, size_t index, size_t size) {
        out = src.read(index, size);
    }

    template <typename S>
    static void read_impl(std::true_type, std::false_type, std::vector<uint8_t>& out, S& src, size_t index, size_t size) {
        const auto span = src.data(index, size);
        out.insert(out.end(), span.data(), span.data() + span.size());
    }

    template <typename T>
    static void value_concat(std::true_type, std::false_type, String& dst, T&& src, uint16_t length) {
        const auto data = src.data(length);
        const auto* ptr = reinterpret_cast<const char*>(data.data());
        dst.concat(ptr, data.size());
    }

    template <typename T>
    static void value_concat(std::false_type, std::true_type, String& dst, T&& src, uint16_t length) {
        dst.reserve(length);
        while (src) {
            dst += reinterpret_cast<char>(src.read());
        }
    }

    template <typename T>
    static void value_concat(std::true_type, std::true_type, String& dst, T&& src, uint16_t length) {
        value_concat(std::true_type{}, std::false_type{}, dst, std::forward<T>(src), length);
    }

    template <typename T>
    static bool value_compare(std::true_type, std::false_type, StringView rhs, T&& src, uint16_t length) {
        return src.data(length) == rhs;
    }

    template <typename T>
    static bool value_compare(std::false_type, std::true_type, StringView rhs, T&& src, uint16_t length) {
        return src.read(length) == rhs;
    }

    template <typename T>
    static bool value_compare(std::true_type, std::true_type, StringView rhs, T&& src, uint16_t length) {
        return value_compare(std::true_type{}, std::false_type{}, rhs, std::forward<T>(src), length);
    }

#ifdef __cpp_lib_result_of_sfinae
    template <typename T, typename R, typename... Args>
    using enable_if_args = typename std::enable_if_t<std::is_same_v<typename std::invoke_result_t<T, Args...>, R>>;
#else
    template <typename T, typename R, typename... Args>
    using enable_if_args = typename std::enable_if<std::is_same<typename std::result_of<T(Args...)>::type, R>::value>::type;
#endif

    // -----------------------------------------------------------------------------------

    // Pointer to the region of data that we are using
    // Note the u16 sizes, underlying storage is expected to be pretty small
    struct Cursor {
        Cursor(RawStorageBase* storage, uint16_t begin, uint16_t end, uint16_t position) :
            _storage(storage),
            _begin(begin),
            _end(end),
            _position(position)
        {}

        static Cursor fromNowhere(RawStorageBase* storage) {
            return Cursor(storage, 0, 0, 0);
        }

        static Cursor fromBegin(RawStorageBase* storage, uint16_t begin, uint16_t end) {
            return Cursor(storage, begin, end, begin);
        }

        static Cursor fromEnd(RawStorageBase* storage, uint16_t begin, uint16_t end) {
            return Cursor(storage, begin, end, end);
        }

        Cursor() = default;

        Cursor(const Cursor&) = default;
        Cursor& operator=(const Cursor&) = default;

        Cursor(Cursor&&) = default;
        Cursor& operator=(Cursor&&) noexcept = default;

        Span<const uint8_t> data() const {
            return data(_end - _position);
        }

        Span<const uint8_t> data(size_t size) const {
            Span<const uint8_t> out;
            read_impl(
                typename Traits::storage_can_data_span{},
                out, *_storage, _position, size);
            return out;
        }

        uint8_t read() const {
            return _storage->read(_position);
        }

        std::vector<uint8_t> read(size_t size) const {
            std::vector<uint8_t> out;
            read_impl(
                typename Traits::storage_can_data_span{},
                typename Traits::storage_can_read_vector{},
                out, *_storage, _position, size);
            return out;
        }

        const Cursor& write(uint8_t value) const {
            return write(std::addressof(value), 1);
        }

        const Cursor& write(const uint8_t* data, size_t size) const {
            const auto span = Span<const uint8_t>(data, size);
            _storage->write(_position, span);
            return *this;
        }

        const Cursor& write(const char* data, size_t size) const {
            const auto* ptr = reinterpret_cast<const uint8_t*>(data);
            return write(ptr, size);
        }

        const Cursor& write(Span<const uint8_t> data) const {
            _storage->write(_position, data);
            return *this;
        }

        const Cursor& write(const Cursor& other) const {
            write_impl(
                typename Traits::storage_can_data_span{},
                typename Traits::storage_can_read_vector{},
                *_storage, other, _position);
            return *this;
        }

        const Cursor& fill(size_t size, uint8_t value) const {
            _storage->fill(_position, size, value);
            return *this;
        }

        const Cursor& fill(uint8_t value) const {
            return fill(_end - _position, value);
        }

        uint16_t size() const {
            return (_end - _begin);
        }

        explicit operator bool() const {
            return (_position >= _begin)
                && (_position < _end);
        }

        uint8_t operator[](size_t offset) const {
            return _storage->read(_begin + offset);
        }

        bool operator==(const Cursor& other) const {
            return (_begin == other._begin) && (_end == other._end);
        }

        bool operator!=(const Cursor& other) const {
            return !(*this == other);
        }

        Cursor& operator-=(uint16_t value) {
            _position -= value;
            return *this;
        }

        Cursor& operator--() {
            return (*this -= 1);
        }

        Cursor operator--(int) {
            Cursor other(*this);
            *this -= 1;
            return other;
        }

        Cursor& operator+=(uint16_t value) {
            _position += value;
            return *this;
        }

        Cursor& operator++() {
            return (*this += 1);
        }

        Cursor operator++(int) {
            Cursor other(*this);
            *this += 1;
            return other;
        }

        uint16_t offset() const {
            return _position - _begin;
        }

        uint16_t position() const {
            return _position;
        }

        void position(uint16_t position) {
            _position = position;
        }

        RawStorageBase* storage() const {
            return _storage;
        }

        uint16_t begin() const {
            return _begin;
        }

        uint16_t end() const {
            return _end;
        }

    private:
        template <typename T>
        void _write_impl(T&, const Cursor&);

        RawStorageBase* _storage{ nullptr };
        uint16_t _begin{ 0 };
        uint16_t _end{ 0 };
        uint16_t _position{ 0 };
    };

    // Tracking state of the parser inside of _raw_read()
    enum class State {
        Begin,
        End,
        Len,
        Value,
        Output
    };

    struct ReadContext {
        Cursor cursor;
        State state;
    };

    ReadContext _make_read_context() {
        return ReadContext{
            .cursor = _make_end_cursor(_begin, _end),
            .state = State::Begin,
        };
    }

public:
    // Store value location in a more reasonable forward-iterator-style manner
    // Allows us to postpone data retrieval when just searching for specific values
    // XXX: be cautious that cursor *references* underlying storage, and some methods *may* return views to data
    struct ReadResult {
        friend class KeyValueStore<RawStorageBase>;

        ReadResult() = default;

        ReadResult(const ReadResult&) = delete;
        ReadResult& operator=(const ReadResult&) noexcept = delete;

        ReadResult(ReadResult&&) = default;
        ReadResult& operator=(ReadResult&&) noexcept = default;

        ReadResult(RawStorageBase* storage, uint16_t begin, uint16_t end) :
            _cursor(Cursor::fromBegin(storage, begin, end)),
            _length(valueLengthImpl(_cursor)),
            _result(true)
        {}

        explicit ReadResult(RawStorageBase* storage) :
            _cursor(Cursor::fromNowhere(storage))
        {}

        explicit operator bool() const {
            return _result;
        }

        bool operator==(StringView other) const noexcept {
            return value_compare(
                typename Traits::storage_can_data_span{},
                typename Traits::storage_can_read_vector{},
                other, valueCursor(), _length);
        }

        ReadResult& operator=(Cursor&& other) noexcept {
            _cursor = std::move(other);
            _length = valueLengthImpl(_cursor);
            _result = true;
            return *this;
        }

        uint16_t valueLength() const {
            return _length;
        }

        std::vector<uint8_t> toVector() const {
            const auto out = valueCursor();
            return out.read(out.size());
        }

        Span<const uint8_t> toSpan() const {
            const auto out = valueCursor();
            return out.data();
        }

        StringView toView() const {
            StringView out;

            const auto span = toSpan();
            if (span.size()) {
                const auto* ptr = reinterpret_cast<const char*>(span.data());
                out = StringView(ptr, span.size());
            }

            return out;
        }

        String toString() const {
            String out;
            value_concat(
                typename Traits::storage_can_data_span{},
                typename Traits::storage_can_read_vector{},
                out, valueCursor(), _length);
            return out;
        }

    private:
        uint16_t begin() const {
            return _cursor.begin();
        }

        uint16_t end() const {
            return _cursor.end();
        }

        uint16_t size() const {
            return _cursor.size();
        }

        RawStorageBase* storage() const {
            return _cursor.storage();
        }

        Cursor valueCursor() const {
            return Cursor::fromBegin(
                _cursor.storage(),
                _cursor.begin(),
                _cursor.begin() + _length);
        }

        static size_t valueLengthImpl(const Cursor& cursor) {
            const auto out = cursor.size();
            if (out <= 2) {
                return 0;
            }

            return out - 2;
        }

        Cursor _cursor;

        uint16_t _length{ 0 };
        bool _result{ false };
    };

    // Internal storage consists of pairs of <byte-range><length>
    struct KeyValueResult {
        KeyValueResult() = default;

        KeyValueResult(const KeyValueResult&) = delete;
        KeyValueResult& operator=(const KeyValueResult&) = delete;

        KeyValueResult(KeyValueResult&&) noexcept = default;
        KeyValueResult& operator=(KeyValueResult&&) noexcept = default;

        template <typename T = ReadResult>
        KeyValueResult(T&& key_, T&& value_) :
            key(std::forward<T>(key_)),
            value(std::forward<T>(value_))
        {}

        explicit KeyValueResult(RawStorageBase* storage) :
            key(storage),
            value(storage)
        {}

        explicit operator bool() const {
            return key.storage() != nullptr
                && value.storage() != nullptr
                && static_cast<bool>(key)
                && (key.valueLength() > 0)
                && static_cast<bool>(value);
        }

        bool operator!() const {
            return !(static_cast<bool>(*this));
        }

        uint16_t begin() const {
            return value.begin();
        }

        uint16_t end() const {
            return key.end();
        }

        size_t size() const {
            return end() - begin();
        }

        ReadResult key;
        ReadResult value;
    };

    struct StopToken {
        explicit StopToken(bool* done) :
            _done(done)
        {}

        explicit StopToken(bool& done) :
            _done(std::addressof(done))
        {}

        bool is_done() const {
            return *_done;
        }

        void set_done() {
            *_done = true;
        }

        explicit operator bool() const {
            return is_done();
        }

    private:
        bool* _done;
    };

    // one and only possible constructor, simply move the class object into the
    // member variable to avoid forcing the user of the API to keep 2 objects alive.
    KeyValueStore(RawStorageBase&& storage, uint16_t begin, uint16_t end) :
        _storage(std::move(storage)),
        _begin(begin),
        _end(end)
    {}

    // Try to find the matching key. Datastructure that we use does not specify
    // any value 'type' inside of it. We expect 'key' to be the first non-empty string,
    // 'value' can be empty.
    ValueResult get(const String& key) {
        return _get(key, true);
    }

    bool has(const String& key) {
        return static_cast<bool>(_get(key, false));
    }

    // Always using _read_kv() from now on, there **must** be a valid pair

    template <typename T>
    enable_if_args<T, void, KeyValueResult&&>
    foreach(T&& callback) {
        auto ctx = _make_read_context();
        foreach(ctx, std::forward<T>(callback));
    }

    template <typename T>
    enable_if_args<T, void, KeyValueResult&&, StopToken>
    foreach(T&& callback) {
        auto ctx = _make_read_context();
        foreach(ctx, std::forward<T>(callback));
    }

    // set or update key with value contents. ensure 'key' isn't empty, 'value' can be empty
    bool set(const String& key, const String& value) {
        // ref. 'estimate()' implementation in regards to the storage calculation
        auto need = estimate(key, value);
        if (!need) {
            return false;
        }

        auto key_len = key.length();
        auto value_len = value.length();

        Cursor to_erase;

        bool need_erase = false;
        bool is_same = false;

        // we need the position at the 'end' of the free space by default
        auto start_pos = _end;

        auto ctx = _make_read_context();

        foreach(ctx, [&](KeyValueResult&& kv, StopToken stop) {
            start_pos = kv.begin();

            // in the very special case we can match the existing key, we either
            if ((kv.key.valueLength() == key_len) && (kv.key == key)) {
                if (kv.value.valueLength() == value.length()) {
                    // - do nothing, as the value is already set
                    if (kv.value == value) {
                        is_same = true;
                        stop.set_done();
                        return;
                    }
                    // - overwrite the space again, with the new kv of the same length
                    start_pos = kv.end();
                    stop.set_done();
                    return;
                }
                // - or, erase the existing kv and place new kv at the end
                to_erase = _make_cursor(kv.begin(), kv.end());
                need_erase = true;
            }
        });

        if (is_same) {
            return true;
        }

        if (need_erase) {
            if ((start_pos + to_erase.size()) < need) {
                return false;
            }
            _raw_erase(start_pos, to_erase);
            start_pos += to_erase.size();
        }
    
        // we should only insert when possition is still within possible size
        if (start_pos && (start_pos >= need)) {
            auto writer = _make_end_cursor(start_pos - need, start_pos);

            // len is stored as big endian
            // everything else is copied as-is
            uint8_t tmp[2];
            tmp[0] = (key_len >> 8) & 0xff;
            tmp[1] = key_len & 0xff;

            writer -= 2;
            writer.write(&tmp[0], sizeof(tmp));

            writer -= key_len;
            writer.write(key.begin(), key_len);

            tmp[0] = (value_len >> 8) & 0xff;
            tmp[1] = value_len & 0xff;

            writer -= 2;
            writer.write(&tmp[0], sizeof(tmp));

            if (value_len) {
                writer -= value_len;
                writer.write(value.begin(), value_len);
            }

            // we also need to add an empty key *after* the value
            // but, only when we still have some space left
            if (writer.position() >= 2) {
                ctx.cursor.position(writer.position());
                auto next_kv = _read_kv(ctx);
                if (!next_kv) {
                    auto next = _make_cursor(writer.begin() - 2, writer.begin());
                    next.fill(0xff);
                }
            }

            _storage.commit();
            return true;
        }

        return false;
    }

    // remove key from the storage. will check that 'key' argument isn't empty
    bool del(const String& key) {
        size_t key_len = key.length();
        if (!key_len) {
            return false;
        }

        // we should only compare strings of equal length.
        // when matching, record { value ... key } range + 4 bytes for length
        // continue searching for available keys and set start_pos and the 'end' of the free space
        size_t start_pos = _end;
        auto to_erase = _make_end_cursor(_begin, _end);
        bool found = false;

        foreach([&](KeyValueResult&& kv) {
            start_pos = kv.begin();
            if (found) {
                return;
            }

            if ((kv.key.valueLength() == key_len) && (kv.key == key)) {
                found = true;
                to_erase = _make_cursor(kv.begin(), kv.end());
            }
        });

        bool out = false;
        if (to_erase) {
            _raw_erase(start_pos, to_erase);
            out = true;
        }

        return out;
    }

    // Simply count key-value pairs that we could parse
    size_t count() {
        size_t result = 0;
        foreach([&result](KeyValueResult&&) {
            ++result;
        });

        return result;
    }

    // Do exactly the same thing as 'keys' does, but return the amount
    // of bytes to the left of the last kv
    size_t available() {
        size_t result = _end - _begin;
        foreach([&result](KeyValueResult&& kv) {
            result -= kv.size();
        });

        return result;
    }

    // How much bytes can be used in total
    size_t size() {
        return _end - _begin;
    }

private:
    template <typename T, typename C = ReadContext>
    enable_if_args<T, void, KeyValueResult&&>
    foreach(C& ctx, T&& callback) {
        for (auto kv = _read_kv(ctx); static_cast<bool>(kv); kv = std::move(_read_kv(ctx))) {
            callback(std::move(kv));
        }
    }

    template <typename T, typename C = ReadContext>
    enable_if_args<T, void, KeyValueResult&&, StopToken>
    foreach(C& ctx, T&& callback) {
        bool flag = false;
        auto stop = StopToken(flag);

        for (auto kv = _read_kv(ctx); static_cast<bool>(kv); kv = std::move(_read_kv(ctx))) {
            callback(std::move(kv), stop);
            if (stop) {
                break;
            }
        }
    }

    Cursor _make_cursor(size_t begin, size_t end) {
        return Cursor::fromBegin(
            std::addressof(_storage), begin, end);
    }

    Cursor _make_end_cursor(size_t begin, size_t end) {
        return Cursor::fromEnd(
            std::addressof(_storage), begin, end);
    }

    Cursor _make_nowhere_cursor() {
        return Cursor::fromNowhere(std::addressof(_storage));
    }

protected:
    // Try to find the matching key. Datastructure that we use does not specify
    // any value 'type' inside of it. We expect 'key' to be the first non-empty string,
    // 'value' can be empty.
    // To implement has(), allow to skip reading the value
    ValueResult _get(const String& key, bool read_value) {
        ValueResult out;
        const auto key_len = key.length();

        foreach([&](KeyValueResult&& kv, StopToken stop) {
            // no point in comparing keys when length does not match
            // (and we also don't want to allocate the string)
            if (kv.key.valueLength() != key_len) {
                return;
            }

            if (kv.key == key) {
                if (read_value) {
                    out = kv.value.toString();
                } else {
                    out = String();
                }
                stop.set_done();
                return;
            }
        });

        return out;
    }

    KeyValueResult _read_kv(ReadContext& ctx) {
        KeyValueResult out;

        auto key = _raw_read(ctx);
        if (key && key.valueLength()) {
            out = KeyValueResult(std::move(key), _raw_read(ctx));
        }

        return out;
    }

    void _raw_erase(size_t start_pos, Cursor& to_erase) {
        // we either end up to the left or to the right of the boundary

        size_t new_pos = (start_pos < to_erase.begin())
            ? (start_pos + to_erase.size())
            : (to_erase.end());

        auto empty = to_erase;

        if (start_pos < to_erase.begin()) {
            // shift storage to the right, overwriting over the now empty space
            auto src = _make_cursor(start_pos, to_erase.begin());
            auto dst = _make_cursor(start_pos + to_erase.size(), to_erase.end());
            dst.write(src);
            empty = _make_cursor(start_pos, start_pos + to_erase.size());
        }

        empty.fill(0xff);

        // same as set(), add empty key as padding
        empty = _make_cursor(new_pos - 2, new_pos);
        empty.fill(0xff);

        _storage.commit();
    }

    // Returns wrapped cursor to the region that holds the data
    //
    // Cursor object is always expected to point to something, e.g. minimum:
    //     0x00 0x00
    //     len2 len1
    // Position will be 0, end will be 2. Total length is 2, data length is 0
    //
    // Or, non-empty value:
    //     0x01 0x00 0x01
    //     data len2 len1
    // Position will be 0, end will be 3. Total length is 3, data length is 1
    ReadResult _raw_read(ReadContext& ctx) {
        uint16_t len = 0;

        ReadResult out;

        // storage is written right-to-left, cursor is always decreasing
        do {
            switch (ctx.state) {

            case State::Begin:
                if (ctx.cursor.offset() >= 2) {
                    ctx.state = State::Len;
                } else {
                    ctx.state = State::End;
                }
                break;

            // len is 16 bit uint (bigendian)
            // special case is 0, which is valid and should be returned when encountered
            // another special case is 0xffff, meaning we just hit an empty space
            case State::Len: {
                len = (--ctx.cursor).read();
                len |= (--ctx.cursor).read() << 8;
                if (len != std::numeric_limits<decltype(len)>::max()) {
                    ctx.state = State::Value;
                } else {
                    ctx.state = State::End;
                }
                break;
            }

            case State::Value: {
                // ensure we don't go out-of-bounds
                if (len && ctx.cursor.offset() < len) {
                    ctx.state = State::End;
                    break;
                }

                // and point at the beginning of the value
                ctx.cursor -= len;
                auto value_start = ctx.cursor.position();

                out = _make_cursor(value_start, value_start + len + 2);
                ctx.state = State::Output;

                break;
            }

            case State::End:
                break;

            case State::Output:
                ctx.state = State::Begin;
                goto return_result;
        }

        } while (ctx.state != State::End);

return_result:
        return out;
    }

    RawStorageBase _storage;
    uint16_t _begin;
    uint16_t _end;
};

} // namespace embedis
} // namespace settings
} // namespace espurna
