/*

Part of the WEBSOCKET MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <IPAddress.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "system.h"
#include "ws.h"

// -----------------------------------------------------------------------------
// WS authentication
// -----------------------------------------------------------------------------

struct WsTicket {
    using TimeSource = espurna::time::CoreClock;
    IPAddress ip;
    TimeSource::time_point timestamp{};
};

// -----------------------------------------------------------------------------
// WS callbacks
// -----------------------------------------------------------------------------

// The idea here is to bind either:
// - constant 'callbacks' list as reference, which was registered via wsRegister()
// - in-place callback / callbacks that will be moved inside this container

namespace espurna {
namespace web {
namespace ws {

class PostponedCallback {
public:
    using Callback = OnSend;
    using Container = Callbacks::OnSendContainer;

    struct Storage {
        enum class Type {
            Empty,
            Callback,
            Pointer,
            Instance,
        };

        struct Pointer {
            Pointer(const Pointer&) = default;

            Pointer(Pointer&& other) noexcept :
                ptr(other.ptr),
                offset(other.offset)
            {
                other.ptr = nullptr;
                other.offset = 0;
            }

            explicit Pointer(const Container* ptr) noexcept :
                ptr(ptr)
            {}

            const Container* ptr;
            size_t offset{};
        };

        struct Instance {
            explicit Instance(Container&& obj) noexcept :
                obj(std::move(obj))
            {}

            Instance(Instance&& other) noexcept :
                obj(std::move(other.obj)),
                offset(other.offset)
            {
                other.offset = 0;
            }

            Container obj;
            size_t offset{};
        };

        explicit Storage(const Callback& callback) :
            _impl(callback),
            _type(Type::Callback)
        {}

        explicit Storage(Callback&& callback) noexcept :
            _impl(std::move(callback)),
            _type(Type::Callback)
        {}

        explicit Storage(const Container& ref) noexcept :
            _impl(ref),
            _type(Type::Pointer)
        {}

        explicit Storage(Container&& obj) noexcept :
            _impl(std::move(obj)),
            _type(Type::Instance)
        {}

        ~Storage();

        Storage(const Storage&) = delete;
        Storage& operator=(const Storage&) = delete;

        Storage(Storage&&) noexcept;
        Storage& operator=(Storage&&) noexcept;

        Type type() const {
            return _type;
        }

        template <typename T>
        void visit(const T& visitor) {
            switch (_type) {
            case Type::Empty:
                break;

            case Type::Callback:
                visitor(_impl.callback);
                break;

            case Type::Pointer:
                visitor(_impl.pointer);
                break;

            case Type::Instance:
                visitor(_impl.instance);
                break;
            }
        }

        template <typename R, typename T>
        R visit(T&& visitor, R defaultValue) {
            switch (_type) {
            case Type::Empty:
                break;

            case Type::Callback:
                return visitor(_impl.callback);

            case Type::Pointer:
                return visitor(_impl.pointer);

            case Type::Instance:
                return visitor(_impl.instance);
            }

            return defaultValue;
        }

    private:
        struct Destructor {
            void operator()(Callback&) const noexcept;
            void operator()(Storage::Pointer&) const noexcept;
            void operator()(Storage::Instance&) const noexcept;
        };

        struct Move {
            explicit Move(Storage&);

            void operator()(Callback&) const noexcept;
            void operator()(Storage::Pointer&) const noexcept;
            void operator()(Storage::Instance&) const noexcept;

        private:
            Storage& _storage;
        };

        union Impl {
            Impl();
            ~Impl();

            explicit Impl(const Callback& callback) :
                callback(callback)
            {}

            explicit Impl(Callback&& callback) noexcept :
                callback(std::move(callback))
            {}

            explicit Impl(Pointer&& ptr) noexcept :
                pointer(ptr)
            {}

            explicit Impl(const Container& ref) noexcept :
                pointer(&ref)
            {}

            explicit Impl(Container&& obj) noexcept :
                instance(std::move(obj))
            {}

            char empty;
            Callback callback;
            Pointer pointer;
            Instance instance;
        };

        Impl _impl;
        Type _type;
    };

    using TimeSource = espurna::time::CpuClock;

    enum class Mode {
        Manual,
        Sequence,
        All,
    };

    struct Done {
        bool operator()(const Callback&) const {
            return true;
        }

        bool operator()(Storage::Pointer& ptr) const {
            return !(ptr.offset < ptr.ptr->size());
        }

        bool operator()(Storage::Instance& obj) const {
            return !(obj.offset < obj.obj.size());
        }
    };

    struct Sequence {
        Sequence(JsonObject& root) :
            _root(root)
        {}

        void operator()(const Callback& func) const {
            func(_root);
        }

        void operator()(Storage::Pointer& ptr) const {
            if (ptr.offset < ptr.ptr->size()) {
                (*ptr.ptr)[ptr.offset](_root);
                ++ptr.offset; 
            }
        }

        void operator()(Storage::Instance& obj) const {
            if (obj.offset < obj.obj.size()) {
                obj.obj[obj.offset](_root);
                ++obj.offset; 
            }
        }

    private:
        JsonObject& _root;
    };

    struct All {
        All(JsonObject& root) :
            _root(root)
        {}

        void operator()(const Callback& func) const {
            func(_root);
        }

        void operator()(Storage::Pointer& ptr) const {
            for (const auto& func : *ptr.ptr) {
                func(_root);
            }

            ptr.offset = ptr.ptr->size();
        }

        void operator()(Storage::Instance& obj) const {
            for (const auto& func : obj.obj) {
                func(_root);
            }

            obj.offset = obj.obj.size();
        }

    private:
        JsonObject& _root;
    };

    PostponedCallback() = delete;

    PostponedCallback(uint32_t client_id, Storage storage, Mode mode) noexcept :
        _client_id(client_id),
        _storage(std::move(storage)),
        _mode(mode)
    {}

    PostponedCallback(uint32_t client_id, Storage storage) noexcept :
        _client_id(client_id),
        _storage(std::move(storage)),
        _mode(mode_from_storage_type(_storage))
    {}

    explicit PostponedCallback(Storage storage) noexcept :
        PostponedCallback(0, std::move(storage))
    {}

    bool done() {
        return _storage.visit(Done{}, true);
    }

    void sendAll(JsonObject& root) {
        _storage.visit(All(root));
    }

    void sendCurrent(JsonObject& root) {
        _storage.visit(Sequence(root));
    }

    void send(JsonObject& root) {
        switch (_mode) {
        case Mode::Sequence:
        case Mode::Manual:
            sendCurrent(root);
            break;

        case Mode::All:
            sendAll(root);
            break;

        }
    }

    uint32_t id() const {
        return _client_id;
    }

    TimeSource::time_point start() const {
        return _start;
    }

    Mode mode() const {
        return _mode;
    }

    void buffer_hint(size_t hint) {
        _hint = hint;
    }

    size_t buffer_hint() {
        return _hint;
    }

private:
    static constexpr auto DefaultBufferHint = size_t{ 3192 };

    static Mode mode_from_storage_type(const Storage& storage) noexcept {
        switch (storage.type()) {
        case Storage::Type::Empty:
        case Storage::Type::Callback:
            break;

        case Storage::Type::Pointer:
            return Mode::Sequence;

        case Storage::Type::Instance:
            return Mode::All;
        }

        return Mode::All;
    }

    uint32_t _client_id;
    TimeSource::time_point _start { TimeSource::now() };

    Storage _storage;
    Mode _mode;

    size_t _hint { DefaultBufferHint };
};

} // namespace ws
} // namespace web
} // namespace espurna
