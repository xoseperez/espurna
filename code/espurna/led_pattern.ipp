/*

Part of the LED MODULE

*/

#pragma once

#include "led_internal.h"
#include "settings_convert.h"

namespace espurna {
namespace led {
namespace {

// Multi-purpose container for either empty, single or multiple delays
// Mainly useful to get rid of vector alloc when using predefined patterns
struct Delays {
    using ConstPointer = Delay*;
    using Pointer = Delay*;

    using Container = std::vector<Delay>;

    struct Storage {
        struct Empty {
        };

        struct Single {
            explicit Single(Delay delay) noexcept :
                delay(delay)
            {}

            Single() = default;
            ~Single() = default;

            Delay delay;
        };

        struct Multiple {
            explicit Multiple(const Container& container) :
                container(container)
            {}

            explicit Multiple(Container&& container) noexcept :
                container(std::move(container))
            {}

            Multiple(const Multiple&) = default;
            Multiple& operator=(const Multiple&) = default;

            Multiple(Multiple&&) noexcept = default;
            Multiple& operator=(Multiple&&) = default;

            Multiple() = default;
            ~Multiple() = default;

            Container container;
        };

        enum class Type {
            Empty,
            Single,
            Multiple,
        };

        explicit Storage(const Single& single) :
            _impl(single),
            _type(Type::Single)
        {}

        explicit Storage(Single&& single) :
            _impl(std::move(single)),
            _type(Type::Single)
        {}

        explicit Storage(const Multiple& multiple) :
            _impl(multiple),
            _type(Type::Multiple)
        {}

        explicit Storage(Multiple&& multiple) :
            _impl(std::move(multiple)),
            _type(Type::Multiple)
        {}

        Storage() = default;
        ~Storage();

        Storage(const Storage&);
        Storage& operator=(const Storage&);

        Storage(Storage&&) noexcept;
        Storage& operator=(Storage&&) noexcept;

        bool operator==(const Storage&) const noexcept;
        bool operator!=(const Storage& other) const noexcept {
            return !(*this == other);
        }

        Type type() const {
            return _type;
        }

        template <typename T>
        void visit(T&& visitor) {
            switch (_type) {
            case Type::Empty:
                visitor();
                break;

            case Type::Single:
                visitor(_impl.single);
                break;

            case Type::Multiple:
                visitor(_impl.multiple);
                break;
            }
        }

        template <typename T>
        void visit(T&& visitor) const {
            return const_cast<Storage*>(this)->visit(std::forward<T>(visitor));
        }

        template <typename R, typename T>
        R visit(T&& visitor, R) {
            switch (_type) {
            case Type::Empty:
                return visitor();

            case Type::Single:
                return visitor(_impl.single);

            case Type::Multiple:
                return visitor(_impl.multiple);
            }

            return R{};
        }

        template <typename R, typename T>
        R visit(T&& visitor, R _) const {
            return const_cast<Storage*>(this)->visit(std::forward<T>(visitor), _);
        }

        struct Destructor {
            void operator()() const noexcept;
            void operator()(Storage::Single&) const noexcept;
            void operator()(Storage::Multiple&) const noexcept;
        };

        struct Copy {
            explicit Copy(Storage&);
            void operator()() const noexcept;
            void operator()(Storage::Single&) const noexcept;
            void operator()(Storage::Multiple&) const noexcept;

        private:
            Storage& _storage;
        };

        struct Move {
            explicit Move(Storage&);
            void operator()() const noexcept;
            void operator()(Storage::Single&) const noexcept;
            void operator()(Storage::Multiple&) const noexcept;

        private:
            Storage& _storage;
        };

        struct Equal {
            explicit Equal(const Storage&);
            bool operator()() const noexcept;
            bool operator()(Storage::Single&) const noexcept;
            bool operator()(Storage::Multiple&) const noexcept;

        private:
            const Storage& _other;
        };

        union Impl {
            Impl();
            ~Impl();

            explicit Impl(Single single) noexcept :
                single(single)
            {}

            explicit Impl(const Multiple& multiple) :
                multiple(multiple)
            {}

            explicit Impl(Multiple&& multiple) noexcept :
                multiple(std::move(multiple))
            {}

            char empty;
            Single single;
            Multiple multiple;
        };

    private:
        Impl _impl;
        Type _type{};
    };

    Delays() = default;
    ~Delays() = default;

    Delays(const Delays&) = default;
    Delays& operator=(const Delays&) = default;

    Delays(Delays&&) = default;
    Delays& operator=(Delays&&) = default;

    Delays(Delay delay) :
        _storage(Storage::Single(delay))
    {}

    Delays(Container&& multiple) noexcept :
        _storage(Storage::Multiple(std::move(multiple)))
    {}

    size_t size() const noexcept;
    void add(Delay);

    ConstPointer begin() const noexcept;
    ConstPointer end() const noexcept;

    Pointer begin() noexcept;
    Pointer end() noexcept;

    bool operator==(const Delays&) const noexcept;
    bool operator!=(const Delays& other) const noexcept {
        return !(*this == other);
    }

private:
    Storage _storage;
};

void Delays::Storage::Destructor::operator()() const noexcept {
}

void Delays::Storage::Destructor::operator()(Storage::Single&) const noexcept {
}

void Delays::Storage::Destructor::operator()(Storage::Multiple& multiple) const noexcept {
    multiple.container.~Container();
}

Delays::Storage::~Storage() {
    visit(Destructor());
}

Delays::Storage::Impl::Impl() :
    empty{}
{}

Delays::Storage::Impl::~Impl() {
}

Delays::Storage::Copy::Copy(Storage& storage) :
    _storage(storage)
{}

void Delays::Storage::Copy::operator()() const noexcept {
}

void Delays::Storage::Copy::operator()(Single& single) const noexcept {
    _storage._impl.single = single;
}

void Delays::Storage::Copy::operator()(Multiple& multiple) const noexcept {
    ::new (&_storage._impl.multiple) Storage::Multiple(std::cref(multiple));
}

Delays::Storage::Storage(const Storage& other) :
    _type(other._type)
{
    other.visit(Copy(*this));
}

Delays::Storage& Delays::Storage::operator=(const Storage& other) {
    visit(Destructor());
    _type = other._type;
    other.visit(Copy(*this));
    return *this;
}

Delays::Storage::Move::Move(Storage& storage) :
    _storage(storage)
{}

void Delays::Storage::Move::operator()() const noexcept {
}

void Delays::Storage::Move::operator()(Single& single) const noexcept {
    _storage._impl.single = single;
}

void Delays::Storage::Move::operator()(Multiple& multiple) const noexcept {
    ::new (&_storage._impl.multiple) Storage::Multiple(std::move(multiple));
}

Delays::Storage::Storage(Storage&& other) noexcept :
    _type(other._type)
{
    other.visit(Move(*this));
    other.visit(Destructor());
    other._type = Type::Empty;
}

Delays::Storage::Equal::Equal(const Storage& other) :
    _other(other)
{}

bool Delays::Storage::Equal::operator()() const noexcept {
    return _other._type == Type::Empty;
}

bool Delays::Storage::Equal::operator()(Single& single) const noexcept {
    return _other._type == Type::Single
        && single.delay == _other._impl.single.delay;
}

bool Delays::Storage::Equal::operator()(Multiple& multiple) const noexcept {
    return _other._type == Type::Multiple
        && multiple.container == _other._impl.multiple.container;
}

bool Delays::Storage::operator==(const Storage& other) const noexcept {
    return visit(Equal{other}, false);
}

bool Delays::operator==(const Delays& other) const noexcept {
    return _storage == other._storage;
}

Delays::Storage& Delays::Storage::operator=(Storage&& other) noexcept {
    visit(Destructor());
    _type = other._type;
    other.visit(Move(*this));
    other.visit(Destructor());
    other._type = Type::Empty;
    return *this;
}

struct Size {
    size_t operator()() const noexcept {
        return 0;
    }

    size_t operator()(Delays::Storage::Single&) const noexcept {
        return 1;
    }

    size_t operator()(Delays::Storage::Multiple& multiple) const noexcept {
        return multiple.container.size();
    }
};

size_t Delays::size() const noexcept {
    return _storage.visit(Size{}, 1);
}

struct Add {
    Add(Delays::Storage& storage, const Delay& delay):
        _storage(storage),
        _delay(delay)
    {}

    void operator()() const noexcept {
        _storage = Delays::Storage(Delays::Storage::Single(_delay));
    }

    void operator()(Delays::Storage::Single& single) const noexcept {
        Delays::Storage::Multiple multiple;
        multiple.container.reserve(2);
        multiple.container.push_back(single.delay);
        multiple.container.push_back(_delay);
        _storage = Delays::Storage(std::move(multiple));
    }

    void operator()(Delays::Storage::Multiple& multiple) const noexcept {
        multiple.container.push_back(_delay);
    }

private:
    Delays::Storage& _storage;
    const Delay& _delay;
};

void Delays::add(Delay delay) {
    _storage.visit(Add{_storage, delay});
}

struct BeginPointer {
    Delays::Pointer operator()() const noexcept {
        return nullptr;
    }

    Delays::Pointer operator()(Delays::Storage::Single& single) const noexcept {
        return std::addressof(single.delay);
    }

    Delays::Pointer operator()(Delays::Storage::Multiple& multiple) const noexcept {
        return std::addressof(*multiple.container.begin());
    }
};

Delays::Pointer Delays::begin() noexcept {
    return _storage.visit(BeginPointer{}, Delays::Pointer{});
}

Delays::ConstPointer Delays::begin() const noexcept {
    return const_cast<Delays*>(this)->begin();
}

struct EndPointer {
    Delays::Pointer operator()() const noexcept {
        return nullptr;
    }

    Delays::Pointer operator()(Delays::Storage::Single& single) const noexcept {
        return std::addressof(single.delay) + 1;
    }

    Delays::Pointer operator()(Delays::Storage::Multiple& multiple) const noexcept {
        return std::addressof(*multiple.container.end());
    }
};

Delays::Pointer Delays::end() noexcept {
    return _storage.visit(EndPointer{}, Pointer{});
}

Delays::ConstPointer Delays::end() const noexcept {
    return const_cast<Delays*>(this)->end();
}

// Sequence of pending 'delays', by default it's in the order they are specified in the underlying vector.
// Notice that there are no checks that '_current' is dereferencable, it's up to the consumer to check via 'operator bool()' first
// Also notice that consumer must correctly implement copy & move, since '_current' and '_end' pointers do not own the target 'delays'
struct Sequence {
    Sequence() = default;

    Sequence(const Sequence&) = default;
    Sequence(Sequence&&) = default;

    explicit Sequence(const Delays& delays) {
        start_with(delays);
    }

    Sequence& operator=(const Sequence&) = default;
    Sequence& operator=(Sequence&&) = default;

    template <typename Status, typename Cycle>
    bool run(Status&& status, Cycle&& cycle) {
        if (!ok()) {
            return false;
        }

        const auto current_status = status();
        const auto current_duration =
            current_status
                ? on() : off();

        if (!cycle(current_status, current_duration)) {
            return false;
        }

        if (repeat()) {
            return true;
        }

        if (next()) {
            // '0,0' at the end restarts from the beginning
            if (on() == Duration::zero()
             && off() == Duration::zero())
            {
                start();
            }
        } else {
            // ...which is completely stopped otherwise
            stop();
            return false;
        }

        return true;
    }

    void start() {
        _current = _begin;
        if (_current != _end) {
            _repeats = (*_current).repeats;
        }
    }

    void stop() {
        _current = _end;
        _repeats = 0;
    }

    void start_with(const Delays& delays) {
        _current = delays.begin();
        _begin = delays.begin();
        _end = delays.end();
        _repeats = (_current != _end)
            ? (*_current).repeats
            : 0;
    }

    void stop_with(const Delays& delays) {
        _current = delays.end();
        _begin = delays.begin();
        _end = delays.end();
        _repeats = 0;
    }

    bool ok() const {
        return _current != _end;
    }

    explicit operator bool() const {
        return ok();
    }

    size_t repeats() const {
        return _repeats;
    }

    Duration on() const {
        return (*_current).on;
    }

    Duration off() const {
        return (*_current).off;
    }

    bool repeat() {
        bool out;

        if (_repeats) {
            --_repeats;
            out = _repeats != 0;
        } else {
            out = _repeats == 0;
        }

        return out;
    }

    bool next() {
        if (_current != _end) {
            ++_current;
            if (_current != _end) {
                _repeats = (*_current).repeats;
                return true;
            }
        }

        return false;
    }

private:
    const Delay* _current{};
    const Delay* _begin{};
    const Delay* _end{};
    size_t _repeats{};
};

struct Pattern {
    Pattern() = default;

    Pattern(const Pattern&) = default;
    Pattern& operator=(const Pattern&) = default;

    Pattern(Pattern&&) noexcept = default;
    Pattern& operator=(Pattern&&) noexcept = default;

    bool operator==(const Pattern& other) const noexcept {
        return _delays == other._delays;
    }

    bool operator!=(const Pattern& other) const noexcept {
        return !(*this == other);
    }

    explicit Pattern(Delays&& delays) :
        _delays(std::move(delays))
    {}

    explicit Pattern(Delay delay) :
        Pattern(Delays(delay))
    {}

    bool ok() const {
        return _delays.size() > 0;
    }

    explicit operator bool() const {
        return ok();
    }

    String toString() const {
        String out;

        bool first = true;
        for (auto it = _delays.begin(); it != _delays.end(); ++it) {
            if (!first) {
                out += ' ';
            }

            first = false;

            const auto repeat_delay = (*it).repeats > 0;
            const auto repeat_pattern =
                ((*it).on == Duration::zero())
             && ((*it).off == Duration::zero());

            if (!repeat_delay && repeat_pattern) {
                out += 'R';
                break;
            }

            out += as_millis((*it).on);
            out += ',';
            out += as_millis((*it).off);

            if (!repeat_delay) {
                break;
            }

            out += ',';
            out += String((*it).repeats);
        }

        return out;
    }

    void add(Delay delay) {
        _delays.add(delay);
    }

    void add(Duration on, Duration off, size_t repeats) {
        _delays.add(
            Delay{
                .on = on,
                .off = off,
                .repeats = repeats,
            });
    }

    size_t size() const {
        return _delays.size();
    }

    Sequence make_sequence() const {
        return Sequence(_delays);
    }

    const Delays& delays() const {
        return _delays;
    }

private:
    static String as_millis(Duration value) {
        const auto ms = std::chrono::duration_cast<duration::Milliseconds>(value);
        return ::espurna::settings::internal::serialize(ms);
    }

    Delays _delays;
};

} // namespace
} // namespace led
} // namespace espurna

