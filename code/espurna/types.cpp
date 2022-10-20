/*

Part of the SYSTEM MODULE

Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "types.h"

namespace espurna {

void Callback::swap(Callback& other) noexcept {
    if (_type == other._type) {
        switch (_type) {
        case StorageType::Empty:
            break;
        case StorageType::Simple:
            std::swap(_storage.simple, other._storage.simple);
            break;
        case StorageType::Wrapper:
            std::swap(_storage.wrapper, other._storage.wrapper);
            break;
        }
        return;
    }

    auto moved = std::move(*this);
    *this = std::move(other);
    other = std::move(moved);
}

void Callback::operator()() const {
    switch (_type) {
    case StorageType::Empty:
        break;
    case StorageType::Simple:
        (*_storage.simple)();
        break;
    case StorageType::Wrapper:
        _storage.wrapper();
        break;
    }
}

void Callback::copy(const Callback& other) {
    _type = other._type;

    switch (other._type) {
    case StorageType::Empty:
        break;
    case StorageType::Simple:
        _storage.simple = other._storage.simple;
        break;
    case StorageType::Wrapper:
        new (&_storage.wrapper) WrapperType(
            other._storage.wrapper);
        break;
    }
}

void Callback::move(Callback& other) noexcept {
    _type = other._type;

    switch (other._type) {
    case StorageType::Empty:
        break;
    case StorageType::Simple:
        _storage.simple = other._storage.simple;
        break;
    case StorageType::Wrapper:
        new (&_storage.wrapper) WrapperType(
            std::move(other._storage.wrapper));
        break;
    }

    other._storage.simple = nullptr;
    other._type = StorageType::Empty;
}

void Callback::reset() {
    switch (_type) {
    case StorageType::Empty:
    case StorageType::Simple:
        break;
    case StorageType::Wrapper:
        _storage.wrapper.~WrapperType();
        break;
    }

    _storage.simple = nullptr;
    _type = StorageType::Empty;
}

Callback& Callback::operator=(Callback&& other) noexcept {
    reset();
    move(other);
    return *this;
}

bool StringView::equals(StringView other) const {
    if (other._len == _len) {
        if (inFlash(_ptr) && inFlash(other._ptr)) {
            return _ptr == other._ptr;
        } else if (inFlash(_ptr)) {
            return memcmp_P(other._ptr, _ptr, _len) == 0;
        } else if (inFlash(other._ptr)) {
            return memcmp_P(_ptr, other._ptr, _len) == 0;
        }

        return __builtin_memcmp(_ptr, other._ptr, _len) == 0;
    }

    return false;
}

bool StringView::equalsIgnoreCase(StringView other) const {
    if (other._len == _len) {
        if (inFlash(_ptr) && inFlash(other._ptr) && (_ptr == other._ptr)) {
            return true;
        } else if (inFlash(_ptr) || inFlash(other._ptr)) {
            String copy;
            const char* ptr = _ptr;
            if (inFlash(_ptr)) {
                copy = toString();
                ptr = copy.begin();
            }

            return strncasecmp_P(ptr, other._ptr, _len) == 0;
        }

        return __builtin_strncasecmp(_ptr, other._ptr, _len) == 0;
    }

    return false;
}

bool StringView::startsWith(StringView other) const {
    if (other._len <= _len) {
        return StringView(begin(), begin() + other._len).equals(other);
    }

    return false;
}

bool StringView::endsWith(StringView other) const {
    if (other._len <= _len) {
        return StringView(end() - other._len, end()).equals(other);
    }

    return false;
}

} // namespace espurna
