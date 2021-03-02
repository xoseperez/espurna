/*

Part of the API MODULE

Copyright (C) 2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <vector>

// -----------------------------------------------------------------------------

struct PathPart {
    enum class Type {
        Unknown,
        Value,
        SingleWildcard,
        MultiWildcard
    };

    Type type;
    size_t offset;
    size_t length;
};

struct PathParts {
    using Parts = std::vector<PathPart>;

    PathParts() = delete;

    PathParts(const PathParts&) = default;
    PathParts(PathParts&&) noexcept = default;

    explicit PathParts(const String& path);

    explicit operator bool() const {
        return _ok;
    }

    void clear() {
        _parts.clear();
    }

    void reserve(size_t size) {
        _parts.reserve(size);
    }

    String operator[](size_t index) const {
        auto& part = _parts[index];
        return _path.substring(part.offset, part.offset + part.length);
    }

    const String& path() const {
        return _path;
    }

    const Parts& parts() const {
        return _parts;
    }

    size_t size() const {
        return _parts.size();
    }

    Parts::const_iterator begin() const {
        return _parts.begin();
    }

    Parts::const_iterator end() const {
        return _parts.end();
    }

    bool match(const PathParts& path) const;
    bool match(const String& path) const {
        return match(PathParts(path));
    }

private:
    PathPart& emplace_back(PathPart::Type type, size_t offset, size_t length) {
        PathPart part { type, offset, length };
        _parts.push_back(std::move(part));
        return _parts.back();
    }

    const String& _path;
    Parts _parts;
    bool _ok { false };
};
