/*

NETWORKING MODULE

Copyright (C) 2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>
#include <IPAddress.h>

#include <memory>
#include <vector>
#include <utility>
#include <numeric>
#include <functional>
#include <forward_list>
#include <system_error>

#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/err.h>

#include "system.h"
#include "types.h"

namespace espurna {
namespace network {

using TimeSource = time::SystemClock;

struct Address {
    ip_addr_t ip;
    uint16_t port;
};

using Data = Span<uint8_t>;
using ConstData = Span<const uint8_t>;

struct ConstDataSequence {
    using value_type = ConstData;
    using container_type = std::vector<value_type>;

    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

    size_t size() const;

    iterator begin() {
        return data.begin();
    }

    const_iterator begin() const {
        return data.begin();
    }

    iterator end() {
        return data.end();
    }

    const_iterator end() const {
        return data.end();
    }

    container_type data;
};

struct Packet {
    static constexpr auto RecvMax = size_t{ std::numeric_limits<uint16_t>::max() };

    Packet() = default;
    ~Packet();

    Packet(const Packet&) = delete;
    Packet& operator=(const Packet&) = delete;

    Packet(Packet&&) noexcept;
    Packet& operator=(Packet&&) noexcept;

    void append(pbuf*);
    void consume(size_t);

    size_t size_chunk() const;
    ConstData peek_chunk(size_t);

    size_t size() const;

    size_t peek(Data);
    std::vector<uint8_t> peek(size_t);

    size_t read(Data);
    std::vector<uint8_t> read(size_t);

private:
    pbuf* _pbuf { nullptr };
};

namespace tcp {

class Client;

struct Completion {
    virtual ~Completion() = default;

    virtual bool is_done() const = 0;
    virtual void set_done() = 0;
    virtual void set_error(std::errc) = 0;
};

using CompletionPtr = std::shared_ptr<Completion>;

struct IoCompletion : public Completion {
    using Result = std::function<void(size_t, std::errc)>;

    IoCompletion() = delete;

    template <typename T>
    IoCompletion(size_t size, T&& result) :
        _result(std::forward<T>(result)),
        _total(size)
    {}

    bool is_done() const override {
        return _done;
    }

    void set_done() override {
        set_done(std::errc{});
    }

    void set_error(std::errc err) override {
        set_done(err);
    }

    void notify_progress(size_t size) {
        if (_done) {
            return;
        }

        _size += size;
        if (_size == _total) {
            set_done(std::errc{});
        }
    }

    void notify_total(size_t size) {
        if (_done) {
            return;
        }

        _size = size;
        if (_size == _total) {
            set_done(std::errc{});
        }
    }

    void notify_partial(size_t size) {
        if (_done) {
            return;
        }

        _size = size;
        set_done(std::errc{});
        retry(size);
    }

    size_t size() const {
        return _size;
    }

    size_t total() const {
        return _total;
    }

    void retry(size_t total) {
        _done = false;
        _size = 0;
        _total = total;
    }

private:
    void set_done(std::errc err) {
        if (!_done) {
            _result(_size, err);
            _done = true;
        }
    }

    Result _result;

    size_t _size { 0 };
    size_t _total { 0 };

    bool _done { false };
};

using IoCompletionPtr = std::shared_ptr<IoCompletion>;

struct BasicCompletion : public Completion {
    using Result = std::function<void(std::errc)>;

    template <typename T>
    BasicCompletion(T&& result) :
        _result(std::forward<T>(result))
    {}

    bool is_done() const override {
        return _done;
    }

    void set_done() override {
        set_done(std::errc{});
    }

    void set_error(std::errc err) override {
        set_done(err);
    }

    void reset() {
        _done = false;
    }

private:
    void set_done(std::errc err) {
        if (!_done) {
            _result(err);
            _done = true;
        }
    }

    Result _result;
    bool _done { false };
};

struct ClientCancelation {
    enum class Type {
        Connection,
        Reading,
        Writing,
    };

    ClientCancelation() = default;
    ClientCancelation(Type type, CompletionPtr ptr) :
        _type(type),
        _ptr(ptr)
    {}

    explicit operator bool() const {
        return _ptr.expired();
    }

    bool set_timeout(Client&, TimeSource::duration);

    void wait_for(Client&, TimeSource::duration);
    void wait_for(Client&, TimeSource::duration, TimeSource::duration);

    void wait(Client&);

    void cancel(Client&);

private:
    bool try_completion_once(Client&);
    void try_completion(Client&, TimeSource::duration);

    Type _type;
    std::weak_ptr<Completion> _ptr;
};

[[nodiscard]]
ClientCancelation connect_async(Client&, Address, BasicCompletion::Result&&);
std::errc connect(Client&, Address);

struct ReadResult {
    std::vector<uint8_t> data;
    std::errc err { std::errc::invalid_argument };
};

[[nodiscard]]
ClientCancelation read_async(Client&, size_t, bool, IoCompletion::Result&&); 
ReadResult read(Client&, size_t);

struct WriteResult {
    size_t size { 0 };
    std::errc err { std::errc::invalid_argument };
};

[[nodiscard]]
ClientCancelation write_async(Client&, ConstDataSequence, IoCompletion::Result&&); 
WriteResult write(Client&, ConstDataSequence);

class Control {
public:
    struct ClientHandler {
        void* arg;
        tcp_err_fn on_error;
        tcp_poll_fn on_poll;
        tcp_recv_fn on_recv;
        tcp_sent_fn on_sent;
    };

    struct ServerHandler {
        void* arg;
        tcp_accept_fn on_accept;
    };

    Control() = default;
    explicit Control(tcp_pcb* pcb) :
        _pcb(pcb)
    {}

    explicit operator bool() const {
        return _pcb != nullptr;
    }

    Control(const Control&) = delete;
    Control& operator=(const Control&) = delete;

    Control(Control&&) noexcept;
    Control& operator=(Control&&) noexcept;

    void set_nodelay();
    void set_nagle();

    err_t connect(Address, tcp_connected_fn);

    err_t attach(Address, ServerHandler);

    err_t attach(tcp_pcb*, ClientHandler);
    err_t attach(ClientHandler);

    tcp_state state() const {
        if (_pcb) {
            return _pcb->state;
        }

        return CLOSED;
    }

    err_t error() const {
        return _last_err;
    }

    tcp_pcb* get() const {
        return _pcb;
    }

    void recv_consume(size_t);

    size_t write_buffer() const;
    err_t write(const uint8_t*, size_t, uint8_t);
    err_t try_send();

    err_t close() noexcept;
    void detach() noexcept;

    err_t abort(err_t);
    err_t abort() {
        return abort(ERR_ABRT);
    }

private:
    err_t _last_err { ERR_OK };
    tcp_pcb* _pcb { nullptr };
};

class Client {
public:
    friend Completion;
    friend IoCompletion;

    friend ClientCancelation;

    friend ClientCancelation connect_async(Client&, Address, BasicCompletion::Result&&);
    friend std::errc connect(Client&, Address);

    friend ClientCancelation write_async(Client&, ConstDataSequence, IoCompletion::Result&&);
    friend WriteResult write(Client&, ConstDataSequence);

    friend ClientCancelation read_async(Client&, size_t, bool, IoCompletion::Result&&);
    friend ReadResult read(Client&, size_t); 

    static constexpr auto RecvBufferSize = size_t{ 536 };

    Client() = delete;
    explicit Client(tcp_pcb*) noexcept;
    explicit Client(Control&&) noexcept;

    // tcp_pcb can be held by only one client
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    // but it is allowed to transfer ownership
    Client(Client&&) noexcept;
    Client& operator=(Client&&) noexcept;

    ~Client() {
        close();
    }

    std::errc connect(Address);

    err_t close();

    err_t error() const {
        return _control.error();
    }

    Address local() const;
    Address remote() const {
        return _remote;
    }

    size_t available() const;
    size_t available_chunk() const;

    bool connected() const;
    bool closed() const;
    void consume(size_t);

    ConstData peek_chunk(size_t);
    ConstData read_chunk(size_t);

    size_t peek(Data);
    std::vector<uint8_t> peek(size_t);

    size_t read(Data);
    std::vector<uint8_t> read(size_t);

    size_t write(ConstData);
    size_t write(ConstDataSequence);

private:
    err_t abort(err_t);
    err_t abort() {
        return abort(ERR_ABRT);
    }

    void detach();
    void complete(err_t);

    err_t on_connect(err_t);
    static err_t s_on_tcp_connect(void* arg, tcp_pcb*, err_t err) {
        return reinterpret_cast<Client*>(arg)->on_connect(err);
    }

    static void s_on_tcp_error(void* arg, err_t err) {
        reinterpret_cast<Client*>(arg)->abort(err);
    }

    err_t on_poll();
    static err_t s_on_tcp_poll(void* arg, tcp_pcb* pcb) {
        return reinterpret_cast<Client*>(arg)->on_poll();
    }

    err_t on_sent(uint16_t);
    static err_t s_on_tcp_sent(void* arg, tcp_pcb*, uint16_t len) {
        return reinterpret_cast<Client*>(arg)->on_sent(len);
    }

    err_t on_recv(pbuf*, err_t);
    static err_t s_on_tcp_recv(void* arg, tcp_pcb*, pbuf* pb, err_t err) {
        return reinterpret_cast<Client*>(arg)->on_recv(pb, err);
    }

    Address _remote;
    Control _control;
    Packet _packet;

    struct Timeout {
        TimeSource::duration timeout;
        TimeSource::time_point start;
        ClientCancelation::Type type;
        std::weak_ptr<Completion> completion;
    };

    struct ReadRequest {
        IoCompletionPtr ptr;
        size_t size;
        bool at_most;
    };

    struct WriteRequest {
        IoCompletionPtr ptr;
        ConstDataSequence data;
        ConstDataSequence::iterator current;
        std::vector<uint8_t> copy_buffer;
        TimeSource::time_point start;
    };

    struct Execution {
        std::forward_list<Timeout> timeouts;
        CompletionPtr connection;
        std::vector<ReadRequest> readers;
        std::vector<WriteRequest> writers;
    };

    Execution _execution;

    void try_timeouts();
    void stop_completion(ClientCancelation::Type, CompletionPtr);

    void stop_connection(CompletionPtr);

    template <typename Requests, typename Handler>
    void try_requests(Requests&, Handler&&);

    template <typename Requests, typename Request = typename Requests::value_type>
    void stop_requests(Requests&, CompletionPtr);

    void start_reader(IoCompletionPtr, size_t, bool);
    void stop_reader(CompletionPtr);

    std::errc read_some(ReadRequest&);
    void try_readers();

    void start_writer(IoCompletionPtr, ConstDataSequence data);
    void stop_writer(CompletionPtr);

    std::errc write_some(WriteRequest&);
    void try_writers();
};

struct AcceptCompletion : public Completion {
    using ClientPtr = std::unique_ptr<Client>;
    using Result = std::function<void(ClientPtr, std::errc)>;

    template <typename T>
    explicit AcceptCompletion(T&& result) noexcept :
        _result(std::move(result))
    {}

    void set_done() override {
        _done = true;
    }

    void set_error(std::errc) override;

    void set_done(ClientPtr, std::errc);
    void retry() {
        _done = false;
    }

private:
    bool _done { false };
    Result _result;
};

class Server;

struct ServerCancelation {
    ServerCancelation() = default;

    explicit ServerCancelation(CompletionPtr ptr) :
        _ptr(ptr)
    {}

    explicit operator bool() const {
        return _ptr.expired();
    }

    // server cancelations are always polling in current context,
    // `set_timeout` not yet implemented to allow async wait

    void wait_for(Server&, TimeSource::duration);
    void wait_for(Server&, TimeSource::duration, TimeSource::duration);

    void wait(Server&);

    void cancel(Server&);

private:
    std::weak_ptr<Completion> _ptr;
};

ServerCancelation accept(Server&, bool, AcceptCompletion::Result&&);

ServerCancelation accept_once(Server&, AcceptCompletion::Result&&);
ServerCancelation accept(Server&, AcceptCompletion::Result&&);

class Server {
public:
    friend ServerCancelation;
    friend ServerCancelation accept(Server&, bool, AcceptCompletion::Result&&);

    Server() = default;
    ~Server();

    Address local() const;

    err_t close();
    err_t listen(Address);

private:
    using AcceptCompletionPtr = std::shared_ptr<AcceptCompletion>;

    err_t on_accept(tcp_pcb*, err_t);
    static err_t s_on_tcp_accept(void* arg, tcp_pcb* pcb, err_t err) {
        return reinterpret_cast<Server*>(arg)->on_accept(pcb, err);
    }

    void detach();

    bool accept(AcceptCompletionPtr, bool retry);

    void complete(err_t);
    void stop_completion(CompletionPtr);

    void try_timeouts();

    struct Accept {
        AcceptCompletionPtr completion;
        bool retry;
    };

    std::vector<Accept> _accept;

    struct Timeout {
        TimeSource::duration timeout;
        TimeSource::time_point start;
        std::weak_ptr<Completion> completion;
    };

    std::forward_list<Timeout> _timeouts;

    Control _control;
};

} // namespace tcp

namespace dns {

struct Host {
    String name;
    IPAddress addr;
    err_t err;
};

using HostPtr = std::shared_ptr<Host>;
using HostCallback = std::function<void(HostPtr)>;

// DNS request is lauched in the background, HostPtr should be waited upon
HostPtr resolve(String);

// ...or, user callback is executed when DNS client is ready to return something
void resolve(String, HostCallback);

// Block until the HostPtr becomes available for reading, or when timeout occurs
bool wait_for(HostPtr, duration::Milliseconds);

// Arduino style result
IPAddress gethostbyname(String, duration::Milliseconds);
IPAddress gethostbyname(String);

} // namespace dns
} // namespace network
} // namespace espurna

void networkSetup();
