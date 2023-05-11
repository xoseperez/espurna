/*

NETWORKING MODULE

Copyright (C) 2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#include <forward_list>
#include <functional>
#include <memory>
#include <utility>
#include <numeric>

#include <lwip/init.h>
#include <lwip/tcp.h>
#include <lwip/dns.h>

#include "network.h"
#include "libs/URL.h"

// not yet CONNECTING or LISTENING
extern "C" struct tcp_pcb *tcp_bound_pcbs;
// accepting or sending data
extern "C" struct tcp_pcb *tcp_active_pcbs;
// // TIME-WAIT status
extern "C" struct tcp_pcb *tcp_tw_pcbs;

namespace espurna {
namespace network {

Packet::~Packet() {
    pbuf_free(_pbuf);
}

Packet::Packet(Packet&& other) noexcept :
    _pbuf(other._pbuf)
{
    other._pbuf = nullptr;
}

Packet& Packet::operator=(Packet&& other) noexcept {
    _pbuf = other._pbuf;
    other._pbuf = nullptr;
    return *this;
}

void Packet::append(pbuf* pb) {
    if (!_pbuf) {
        _pbuf = pb;
    } else {
        pbuf_cat(_pbuf, pb);
    }
}

void Packet::consume(size_t size) {
    if (_pbuf) {
        _pbuf = pbuf_free_header(_pbuf, size);
    }
}

size_t Packet::size() const {
    if (_pbuf) {
        return _pbuf->tot_len;
    }

    return 0;
}

size_t Packet::size_chunk() const {
    if (_pbuf) {
        return _pbuf->len;
    }

    return 0;
}

ConstData Packet::peek_chunk(size_t size) {
    if (_pbuf) {
        const auto available = size_t{ _pbuf->len };
        const auto* payload = reinterpret_cast<const uint8_t*>(_pbuf->payload);
        return {payload, std::min({size, available, RecvMax})};
    }

    return {};
}

size_t Packet::peek(Data out) {
    if (_pbuf) {
        return pbuf_copy_partial(_pbuf, out.data(), out.size(), 0);
    }

    return 0;
}

std::vector<uint8_t> Packet::peek(size_t size) {
    std::vector<uint8_t> out;
    if (size) {
        size = std::min(size, RecvMax);
        out.resize(size);
        peek(Data{out.data(), size});
    }

    return out;
}

size_t Packet::read(Data out) {
    const auto received = peek(out);
    if (received) {
        consume(received);
    }

    return 0;
}

std::vector<uint8_t> Packet::read(size_t size) {
    auto received = peek(size);
    consume(received.size());
    return received;
}

namespace tcp {

Address remote_address(tcp_pcb* pcb) {
    Address out;
    if (pcb) {
        ip_addr_copy(out.ip, pcb->remote_ip);
        out.port = pcb->remote_port;
    }

    return out;
}

Address remote_address(const Control& control) {
    return remote_address(control.get());
}

Address local_address(tcp_pcb* pcb) {
    Address out;
    if (pcb) {
        ip_addr_copy(out.ip, pcb->local_ip);
        out.port = pcb->local_port;
    }

    return out;
}

Address local_address(const Control& control) {
    return local_address(control.get());
}

template <typename Result>
std::shared_ptr<IoCompletion> make_io_completion(size_t size, Result&& result) {
    return std::make_shared<IoCompletion>(size, std::forward<Result>(result));
}

template <typename Timeout, typename Interval, typename T>
static bool poll(Timeout timeout, Interval interval, T&& blocked) {
    return time::blockingDelay(
        std::chrono::duration_cast<duration::Milliseconds>(timeout),
        std::chrono::duration_cast<duration::Milliseconds>(interval),
        std::forward<T>(blocked));
}

template <typename Interval, typename T>
static void poll(Interval interval, T&& blocked) {
    for (;;) {
        const auto result = blocked();
        if (!result) {
            break;
        }

        time::delay(interval);
    }
}

std::errc from_lwip_error(err_t err) {
    switch (err) {
    case ERR_OK:
        break;

    case ERR_MEM:
        return std::errc::not_enough_memory;

    case ERR_BUF:
        return std::errc::no_buffer_space;

    case ERR_TIMEOUT:
        return std::errc::timed_out;

    case ERR_RTE:
        return std::errc::network_unreachable;

    case ERR_INPROGRESS:
        return std::errc::operation_in_progress;

    case ERR_VAL:
        return std::errc::invalid_argument;

    case ERR_WOULDBLOCK:
        return std::errc::operation_would_block;

    case ERR_USE:
        return std::errc::address_in_use;

    case ERR_ALREADY:
        return std::errc::already_connected;

    case ERR_ISCONN:
        return std::errc::already_connected;

    case ERR_CONN:
        return std::errc::not_connected;

    case ERR_IF:
        return std::errc::network_down;

    case ERR_ABRT:
        return std::errc::operation_canceled;

    case ERR_RST:
        return std::errc::connection_reset;

    case ERR_CLSD:
        return std::errc::connection_aborted;

    case ERR_ARG:
        return std::errc::invalid_argument;

    }

    return std::errc{};
}

void Control::set_nagle() {
    if (_pcb) {
        tcp_nagle_enable(_pcb);
    }
}

void Control::set_nodelay() {
    if (_pcb) {
        tcp_nagle_disable(_pcb);
    }
}

err_t Control::connect(Address address, tcp_connected_fn handler) {
    if (_pcb) {
        return ERR_VAL;
    }

    if (_pcb->state == ESTABLISHED) {
        return ERR_ALREADY;
    }

    return tcp_connect(_pcb, &address.ip, address.port, handler);
}

err_t Control::attach(Address address, ServerHandler handler) {
    if (_pcb) {
        return ERR_VAL;
    }

    auto* pcb = tcp_new();
    if (!pcb) {
        return ERR_MEM;
    }

    pcb->so_options |= SOF_REUSEADDR;
    pcb->prio = TCP_PRIO_MIN;

    const auto err = tcp_bind(pcb, &address.ip, address.port);
    if (err != ERR_OK) {
        return err;
    }

    auto* listen = tcp_listen(pcb);
    if (!listen) {
        tcp_abort(pcb);
        return ERR_VAL;
    }

    tcp_arg(pcb, handler.arg);
    tcp_accept(pcb, handler.on_accept);

    _pcb = pcb;

    return ERR_OK;
}


err_t Control::attach(ClientHandler handler) {
    tcp_arg(_pcb, handler.arg);
    tcp_err(_pcb, handler.on_error);
    tcp_poll(_pcb, handler.on_poll, 1);
    tcp_recv(_pcb, handler.on_recv);
    tcp_sent(_pcb, handler.on_sent);

    return ERR_OK;
}

err_t Control::attach(tcp_pcb* other, ClientHandler handler) {
    if (!_pcb) {
        _pcb = other;
        attach(handler);
        return ERR_OK;
    }

    return ERR_VAL;
}

size_t Control::write_buffer() const {
    if (_pcb) {
        return tcp_sndbuf(_pcb);
    }

    return 0;
}

void Control::recv_consume(size_t size) {
    if (_pcb) {
        tcp_recved(_pcb, size);
    }
}

err_t Control::write(const uint8_t* data, size_t size, uint8_t flags) {
    return tcp_write(_pcb, data, size, flags);
}

err_t Control::try_send() {
    return tcp_output(_pcb);
}

void Control::detach() noexcept {
    if (_pcb) {
        tcp_arg(_pcb, nullptr);
        tcp_recv(_pcb, nullptr);
        tcp_err(_pcb, nullptr);
        tcp_sent(_pcb, nullptr);
        tcp_poll(_pcb, nullptr, 0);
    }
}

err_t Control::close() noexcept {
    err_t err = ERR_OK;
    if (_pcb) {
        detach();
        err = tcp_close(_pcb);
        if (err != ERR_OK) {
            err = abort();
        }

        _pcb = nullptr;
    }

    return err;
}

err_t Control::abort(err_t err) {
    tcp_abort(_pcb);

    _pcb = nullptr;
    _last_err = err;

    return ERR_ABRT;
}

Client::Client(Control&& control) noexcept :
    _remote(remote_address(control.get())),
    _control(std::move(control))
{
    _control.attach(
        Control::ClientHandler{
            .arg = this,
            .on_error = s_on_tcp_error,
            .on_poll = s_on_tcp_poll,
            .on_recv = s_on_tcp_recv,
            .on_sent = s_on_tcp_sent,
        });
    _control.set_nagle();
}

Client::Client(tcp_pcb* pcb) noexcept :
    Client(Control(pcb))
{}

Client::Client(Client&& other) noexcept :
    _remote(other._remote),
    _control(std::move(other._control)),
    _packet(std::move(other._packet)),
    _execution(std::move(other._execution))
{}

Client& Client::operator=(Client&& other) noexcept {
    abort();

    _control = std::move(other._control);
    _execution = std::move(other._execution);

    _remote = other._remote;

    return *this;
}

Address Client::local() const {
    return local_address(_control);
}

std::errc Client::connect(Address address) {
    if (_control && !_execution.connection) {
        return from_lwip_error(
            _control.connect(address, s_on_tcp_connect));
    }

    if (_execution.connection) {
        return std::errc::connection_already_in_progress;
    }

    return std::errc::invalid_argument;
}

err_t Client::on_connect(err_t err) {
    if (!_execution.connection) {
        return ERR_OK;
    }

    decltype(_execution.connection) connection;
    std::swap(_execution.connection, connection);

    if (err != ERR_OK) {
        connection->set_error(from_lwip_error(err));
    } else {
        connection->set_done();
    }

    return _control.error();
}

err_t Client::on_poll() {
    try_readers();
    try_writers();
    return _control.error();
}

err_t Client::on_sent(uint16_t) {
    try_writers();
    return _control.error();
}

void Client::stop_connection(CompletionPtr ptr) {
    if (_execution.connection == ptr) {
        decltype(_execution.connection) connection;
        std::swap(_execution.connection, connection);
        connection->set_error(std::errc::operation_canceled);
        close();
    }
}

bool ClientCancelation::set_timeout(Client& client, TimeSource::duration timeout) {
    auto ptr = _ptr.lock();
    if (!ptr) {
        return false;
    }

    client._execution.timeouts.push_front(
        Client::Timeout{
            .timeout = timeout,
            .start = TimeSource::now(),
            .type = _type,
            .completion = ptr,
        });

    return true;
}

bool ClientCancelation::try_completion_once(Client& client) {
    switch (_type) {
    case Type::Connection:
        break;
    case Type::Writing:
        client.try_writers();
        break;
    case Type::Reading:
        client.try_readers();
        break;
    }

    return _ptr.expired();
}

void ClientCancelation::try_completion(Client& client, TimeSource::duration interval) {
    for (;;) {
        if (try_completion_once(client)) {
            return;
        }

        time::delay(std::chrono::duration_cast<duration::Milliseconds>(interval));
    }
}

void ClientCancelation::wait_for(Client& client, TimeSource::duration timeout, TimeSource::duration interval) {
    const auto result = set_timeout(client, timeout);
    if (!result) {
        return;
    }
    
    try_completion(client, interval);
}

void ClientCancelation::wait_for(Client& client, TimeSource::duration timeout) {
    wait_for(client, timeout, duration::Milliseconds{ 10 });
}

void ClientCancelation::wait(Client& client) {
    try_completion(client, duration::Milliseconds{ 10 });
}

void ClientCancelation::cancel(Client& client) {
    auto ptr = _ptr.lock();
    if (!ptr) {
        return;
    }

    client.stop_completion(_type, ptr);
}

size_t Client::available() const {
    return _packet.size();
}

size_t Client::available_chunk() const {
    return _packet.size_chunk();
}

bool Client::connected() const {
    return _control.state() == ESTABLISHED;
}

bool Client::closed() const {
    switch (_control.state()) {
    case tcp_state::CLOSED:
    case tcp_state::CLOSE_WAIT:
        return true;

    default:
        break;
    }

    return false;
}

void Client::consume(size_t size) {
    _packet.consume(size);
    _control.recv_consume(size);
}


size_t Client::peek(Data out) {
    return _packet.peek(out);
}

std::vector<uint8_t> Client::peek(size_t size) {
    return _packet.peek(size);
}

size_t Client::read(Data out) {
    const auto received = peek(out);
    if (received) {
        consume(received);
    }

    return 0;
}

std::vector<uint8_t> Client::read(size_t size) {
    auto received = peek(size);
    consume(received.size());
    return received;
}

void Client::stop_completion(ClientCancelation::Type type, CompletionPtr ptr) {
    switch (type) {
    case ClientCancelation::Type::Connection:
        stop_connection(ptr);
        break;

    case ClientCancelation::Type::Reading:
        stop_reader(ptr);
        break;

    case ClientCancelation::Type::Writing:
        stop_writer(ptr);
        break;
    }
}

template <typename Timeouts, typename T>
void try_timeouts_impl(Timeouts& timeouts, T&& handler) {
    timeouts.remove_if(
        [](const typename Timeouts::value_type& timeout) {
            return timeout.completion.expired();
        });

    if (timeouts.empty()) {
        return;
    }

    const auto now = TimeSource::now();
    for (auto& timeout : timeouts) {
        if (now - timeout.start < timeout.timeout) {
            continue;
        }

        auto ptr = timeout.completion.lock();
        if (!ptr) {
            continue;
        }

        ptr->set_error(std::errc::timed_out);
        handler(timeout, ptr);
    }
}

void Client::try_timeouts() {
    try_timeouts_impl(
        _execution.timeouts,
        [&](Timeout& timeout, CompletionPtr ptr) {
            stop_completion(timeout.type, ptr);
        });
}

template <typename Requests, typename Handler>
void Client::try_requests(Requests& requests, Handler&& handler) {
    try_timeouts();
    for (;;) {
        const auto it = requests.begin();
        if (it == requests.end()) {
            return;
        }

        switch (handler(*it)) {
        // still a pending request, but cannot continue just this moment
        case std::errc::operation_in_progress:
            return;

        // (possibly) interrupted by external means, cannot use `it`
        case std::errc::interrupted:
            return;

        // everything else should discard the request
        default:
            break;
        }

        requests.erase(it);
    }
}

template <typename Requests, typename Request>
void Client::stop_requests(Requests& requests, CompletionPtr ptr) {
    auto it = std::find_if(
        requests.begin(),
        requests.end(),
        [&](const Request& request) {
            return request.ptr == ptr;
        });

    if (it != requests.end()) {
        IoCompletionPtr ptr;
        std::swap((*it).ptr, ptr);
        requests.erase(it);
        ptr->set_error(std::errc::operation_canceled);
    }
}

std::errc Client::read_some(ReadRequest& request) {
    const size_t pending = _packet.size();
    if (!pending) {
        return std::errc::operation_in_progress;
    }

    auto completion = request.ptr;

    const auto chunk = std::min(request.size, pending);
    if (request.at_most && chunk != 0) {
        completion->notify_partial(pending);
        return std::errc::operation_in_progress;
    }

    auto err = std::errc::operation_in_progress;
    if (chunk == request.size) {
        err = std::errc{};
    }

    if (chunk > 0) {
        completion->notify_total(chunk);
    }

    if (completion->is_done()) {
        err = std::errc::interrupted;
    }

    return err;
}

void Client::try_readers() {
    try_requests(_execution.readers,
        [&](ReadRequest& request) -> std::errc {
            return read_some(request);
        });
}

void Client::stop_reader(CompletionPtr ptr) {
    stop_requests(_execution.readers, ptr);
}

void Client::try_writers() {
    try_requests(_execution.writers,
        [&](WriteRequest& request) -> std::errc {
            return write_some(request);
        });
}

void Client::stop_writer(CompletionPtr ptr) {
    stop_requests(_execution.writers, ptr);
}

void Client::start_reader(IoCompletionPtr ptr, size_t size, bool at_most) {
    _execution.readers.push_back(ReadRequest{
        .ptr = std::move(ptr),
        .size = size,
        .at_most = at_most,
    });
}

std::errc Client::write_some(WriteRequest& request) {
    for (;;) {
        if (request.current == request.data.end()) {
            return std::errc{};
        }

        if (!_control) {
            return std::errc::not_connected;
        }

        const size_t available = _control.write_buffer();
        auto& current = *request.current;

        auto size = std::min(current.size(), available);
        if (!size) {
            return std::errc::operation_in_progress;
        }

        auto flags = TCP_WRITE_FLAG_COPY;
        if ((request.data.end() != (request.current + 1))
            || (size != current.size()))
        {
            flags |= TCP_WRITE_FLAG_MORE;
        }

        const auto* ptr = current.data();

        if (pointerInFlash(ptr)) {
            size = std::min(size, RecvBufferSize);
            request.copy_buffer.resize(size);
            memcpy_P(
                request.copy_buffer.data(), ptr,
                request.copy_buffer.size());
            ptr = request.copy_buffer.data();
        }

        auto completion = request.ptr;

        auto err = _control.write(ptr, size, flags);
        if (err != ERR_OK) {
            completion->set_error(from_lwip_error(err));
            return std::errc::interrupted;
        }

        if (current.size() - size != 0) {
            current = current.subspan(size);
        } else {
            std::advance(request.current, 1);
        }

        err = _control.try_send();
        if (err != ERR_OK) {
            completion->set_error(from_lwip_error(err));
            return std::errc::interrupted;
        }

        completion->notify_progress(size);
        if (completion->is_done()) {
            return std::errc::interrupted;
        }
    }

    return std::errc{};
}

void Client::start_writer(IoCompletionPtr ptr, ConstDataSequence data) {
    _execution.writers.push_back(WriteRequest{
        .ptr = std::move(ptr),
        .data = std::move(data),
        .current = data.begin(),
        .start = TimeSource::now(),
    });
}

std::shared_ptr<BasicCompletion> make_basic_completion(BasicCompletion::Result&& result) {
    return std::make_shared<BasicCompletion>(std::move(result));
}

std::errc connect(Client& client, Address address) {
    auto out = std::errc::operation_in_progress;

    auto cancelation = connect_async(
        client,
        address,
        [&](std::errc err) {
            out = err;
        });
    cancelation.wait(client);

    return out;
}

ClientCancelation connect_async(Client& client, Address address, BasicCompletion::Result&& result) {
    ClientCancelation out;
    if (client._execution.connection) {
        result(std::errc::operation_in_progress);
        return out;
    }

    const auto err = client.connect(address);
    if (err != std::errc{}) {
        result(err);
        return out;
    }

    auto completion = make_basic_completion(std::move(result));
    client._execution.connection = completion;

    return ClientCancelation(ClientCancelation::Type::Connection, completion);
}

err_t Client::close() {
    complete(ERR_CLSD);
    return _control.close();
}

void Client::complete(err_t err) {
    const auto errc = from_lwip_error(err);

    decltype(_execution) execution;
    std::swap(_execution, execution);
    if (execution.connection) {
        execution.connection->set_error(errc);
    }

    for (auto& reader : execution.readers) {
        reader.ptr->set_error(errc);
    }

    for (auto& writer : execution.writers) {
        writer.ptr->set_error(errc);
    }
}

err_t Client::abort(err_t err) {
    detach();
    complete(err);
    return _control.abort(err);
}

void Client::detach() {
    _control.detach();
}

err_t Client::on_recv(pbuf* pb, err_t err) {
    // safe to assume on err, just abort
    if (!pb || (err != ERR_OK)) {
        return close();
    }

    _packet.append(pb);
    try_readers();

    return _control.error();
}

ClientCancelation write_async(Client& client, ConstDataSequence data, IoCompletion::Result&& result) {
    ClientCancelation out;
    if (!client.connected()) {
        result(0, std::errc::not_connected);
        return out;
    }

    auto completion = make_io_completion(
        data.size(),
        std::move(result));
    client.start_writer(completion, std::move(data));

    return ClientCancelation(ClientCancelation::Type::Writing, completion);
}

WriteResult write(Client& client, ConstDataSequence data) {
    WriteResult out;
    out.size = 0;
    out.err = std::errc::operation_in_progress;

    auto completion = write_async(
        client,
        std::move(data),
        [&](size_t size, std::errc err) {
            out.size = size;
            out.err = err;
        });

    for (;;) {
        if (out.err != std::errc::operation_in_progress) {
            break;
        }

        client.try_writers();
        time::delay(duration::Milliseconds{ 10 });
    }

    return out;
}

ClientCancelation read_async(Client& client, size_t size, bool at_most, IoCompletion::Result&& result) {
    ClientCancelation out;
    if (!client.connected()) {
        result(0, std::errc::not_connected);
        return out;
    }

    auto completion = make_io_completion(size, std::move(result));
    client.start_reader(completion, size, at_most);

    return ClientCancelation(ClientCancelation::Type::Reading, completion);
}

ReadResult read(Client& client, size_t size) {
    ReadResult out;
    out.err = std::errc::operation_in_progress;

    size_t received = 0;
    auto cancelation = read_async(
        client,
        size,
        false,
        [&](size_t size, std::errc err) {
            out.err = err;
            received = size;
        });
    cancelation.wait(client);

    out.data = client.read(received);

    return out;
}

Server::~Server() {
    close();
}

Address Server::local() const {
    return local_address(_control);
}

bool Server::accept(AcceptCompletionPtr ptr, bool retry) {
    if (_control) {
        _accept.push_back(
            Server::Accept{
                .completion = std::move(ptr),
                .retry = retry,
            });
        return true;
    }

    return false;
}

ServerCancelation accept(Server& server, bool retry, AcceptCompletion::Result&& result) {
    auto completion = std::make_shared<AcceptCompletion>(std::move(result));
    if (!server.accept(completion, retry)) {
        return ServerCancelation();
    }

    return ServerCancelation(completion);
}

ServerCancelation accept_once(Server& server, AcceptCompletion::Result&& result) {
    return accept(server, false, std::move(result));
}

ServerCancelation accept(Server& server, AcceptCompletion::Result&& result) {
    return accept(server, true, std::move(result));
}

void AcceptCompletion::set_error(std::errc err) {
    set_done(nullptr, err);
}

void AcceptCompletion::set_done(ClientPtr ptr, std::errc err) {
    if (!_done) {
        _result(std::move(ptr), err);
        _done = true;
    }
}

err_t Server::on_accept(tcp_pcb* pcb, err_t err) {
    if (!_accept.size()) {
        return ERR_VAL;
    }

    const auto errc = from_lwip_error(err);
    auto it = _accept.begin();

    auto accept = (*it);
    if (!pcb) {
        accept.completion->set_error(errc);
    } else if (err != ERR_OK) {
        accept.completion->set_error(errc);
    } else {
        accept.completion->set_done(
            std::make_unique<Client>(pcb), std::errc{});
    }

    const auto last = _control.error();
    if (last != ERR_OK) {
        return err;
    }

    if (!accept.retry) {
        _accept.erase(it);
        return ERR_OK;
    }

    accept.completion->retry();

    return ERR_OK;
}

void Server::complete(err_t err) {
    const auto errc = from_lwip_error(err);

    decltype(_accept) accept;
    std::swap(_accept, accept);
    for (auto& handler : accept) {
        handler.completion->set_error(errc);
    }
}

err_t Server::close() {
    complete(ERR_CLSD);
    return _control.close();
}

err_t Server::listen(Address address) {
    return _control.attach(
        address, 
        Control::ServerHandler{
            .arg = this,
            .on_accept = s_on_tcp_accept,
        });
}

void Server::detach() {
    _control.detach();
}

void Server::try_timeouts() {
    try_timeouts_impl(
        _timeouts,
        [&](const Timeout&, CompletionPtr ptr) {
            stop_completion(ptr);
        });
}

void Server::stop_completion(CompletionPtr ptr) {
    _accept.erase(std::remove_if(
        _accept.begin(),
        _accept.end(),
        [ptr](const Accept& accept) {
            return accept.completion == ptr;
        }));
}

void ServerCancelation::cancel(Server& server) {
    auto ptr = _ptr.lock();
    if (!ptr) {
        return;
    }

    server.stop_completion(ptr); 
}

void ServerCancelation::wait_for(Server& server, TimeSource::duration timeout, TimeSource::duration interval) {
    const auto result = poll(
        timeout, interval,
        [&]() {
            return !_ptr.expired();
        });

    if (result) {
        return;
    }

    const auto ptr = _ptr.lock();
    if (!ptr) {
        return;
    }

    ptr->set_error(std::errc::timed_out);
    server.stop_completion(ptr);
}

void ServerCancelation::wait_for(Server& server, TimeSource::duration timeout) {
    wait_for(server, timeout, duration::Milliseconds{ 10 });
}

void ServerCancelation::wait(Server& server) {
    for (;;) {
        if (_ptr.expired()) {
            return;
        }

        time::delay(duration::Milliseconds{ 10 });
    }
}

} // namespace tcp

namespace dns {
namespace {

struct PendingHost {
    HostPtr ptr;
    HostCallback callback;
};

namespace internal {

using Pending = std::forward_list<PendingHost>;
Pending pending;

} // namespace internal

void dns_found_callback_impl(const char* name, const ip_addr_t* addr, void* arg) {
    auto* pending = reinterpret_cast<Host*>(arg);

    if (addr) {
        pending->addr = addr;
        pending->err = ERR_OK;
    } else {
        pending->err = ERR_ABRT;
    }

    internal::pending.remove_if(
        [&](const PendingHost& lhs) {
            if (lhs.ptr.get() == pending) {
                if (lhs.callback) {
                    lhs.callback(lhs.ptr);
                }

                return true;
            }

            return false;
        });
}

HostPtr resolve_impl(String hostname, HostCallback callback) {
    auto host = std::make_shared<Host>(
        Host{
            .name = std::move(hostname),
            .addr = IPAddress{},
            .err = ERR_INPROGRESS,
        });

    const auto err = dns_gethostbyname(
        host->name.c_str(),
        host->addr,
        dns_found_callback_impl,
        host.get());

    host->err = err;

    switch (err) {
    case ERR_OK:
    case ERR_MEM:
        if (callback) {
            callback(host);
        }
        break;

    case ERR_INPROGRESS:
        internal::pending.push_front(
            PendingHost{
                .ptr = host,
                .callback = std::move(callback)
            });
        break;
    }

    return host;
}

} // namespace

HostPtr resolve(String hostname) {
    return resolve_impl(hostname, nullptr);
}

void resolve(String hostname, HostCallback callback) {
    if (!callback) {
        return;
    }

    resolve_impl(std::move(hostname), std::move(callback));
}

bool wait_for(HostPtr ptr, duration::Milliseconds timeout) {
    if (ptr->err == ERR_OK) {
        return true;
    }

    if (ptr->err != ERR_INPROGRESS) {
        return false;
    }

    time::blockingDelay(
        timeout,
        duration::Milliseconds{ 10 },
        [&]() {
            return ptr->err == ERR_INPROGRESS;
        });

    return ptr->err == ERR_OK;
}

IPAddress gethostbyname(String hostname, duration::Milliseconds timeout) {
    IPAddress out;

    auto result = resolve(hostname);
    if (wait_for(result, timeout)) {
        out = result->addr;
    }

    return out;
}

IPAddress gethostbyname(String hostname) {
    return gethostbyname(hostname, duration::Seconds{ 3 });
}

} // namespace dns

namespace {

#if TERMINAL_SUPPORT
namespace terminal {
namespace commands {

PROGMEM_STRING(Host, "HOST");

void host(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 2) {
        terminalError(ctx, F("HOST <hostname>"));
        return;
    }

    const auto result = dns::gethostbyname(ctx.argv[1]);
    if (result.isSet()) {
        ctx.output.printf_P(PSTR("%s has address %s\n"),
            ctx.argv[1].c_str(), result.toString().c_str());
        terminalOK(ctx);
        return;
    }

    ctx.output.printf_P(PSTR("%s not found\n"), ctx.argv[1].c_str());
}

PROGMEM_STRING(Netstat, "NETSTAT");

void netstat(::terminal::CommandContext&& ctx) {
    const struct tcp_pcb* pcbs[] {
        tcp_active_pcbs,
        tcp_tw_pcbs,
        tcp_bound_pcbs,
    };

    for (const auto* list : pcbs) {
        for (const tcp_pcb* pcb = list; pcb != nullptr; pcb = pcb->next) {
            ctx.output.printf_P(PSTR("state %s local %s:%hu remote %s:%hu\n"),
                    tcp_debug_state_str(pcb->state),
                    IPAddress(pcb->local_ip).toString().c_str(),
                    pcb->local_port,
                    IPAddress(pcb->remote_ip).toString().c_str(),
                    pcb->remote_port);
        }
    }
}

PROGMEM_STRING(Ncat, "NCAT");

void ncat(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 3) {
        terminalError(ctx, F("NCAT <HOST> <PORT>"));
        return;
    }

    const auto host = dns::gethostbyname(ctx.argv[1]);
    if (!host.isSet()) {
        terminalError(ctx, F("cannot resolve"));
        return;
    }

    const auto port = settings::internal::convert<uint16_t>(ctx.argv[2]);

    auto* pcb = tcp_new();
    if (!pcb) {
        terminalError(ctx, F("no pcb"));
        return;
    }

    const auto serialize_error = [&](std::errc err) {
        String error;
        error += "error ";
        error += (int)err;
        return error;
    };

    tcp::Client x(pcb);
    const auto result = tcp::connect(x, Address{host, port});
    if (result != std::errc{}) {
        terminalError(ctx, serialize_error(result));
        return;
    }

    auto cancelation = tcp::read_async(x, 1024, true,
        [&](size_t size, std::errc err) {
            if (err != std::errc{}) {
                terminalError(ctx, serialize_error(err));
                return;
            }

            const auto data = x.read(size);
            ctx.output.printf("notified %zu read %zu\n",
                size, data.size());
            ctx.output.println(
                hexEncode(data.data(), data.data() + data.size()));
        });
    cancelation.wait_for(x, duration::Seconds{ 5 });

    terminalOK(ctx);
}

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
PROGMEM_STRING(MflnProbe, "MFLN.PROBE");

void mfln_probe(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 3) {
        terminalError(ctx, F("MFLN.PROBE <URL> <SIZE>"));
        return;
    }

    URL parsed(std::move(ctx.argv[1]));

    const auto parse_mfln = espurna::settings::internal::convert<uint16_t>;
    uint16_t mfln = parse_mfln(ctx.argv[2]);

    auto client = std::make_unique<BearSSL::WiFiClientSecure>();
    client->setInsecure();

    if (client->probeMaxFragmentLength(parsed.host.c_str(), parsed.port, mfln)) {
        terminalOK(ctx);
        return;
    }

    terminalError(ctx, F("Buffer size not supported"));
}
#endif

static constexpr ::terminal::Command List[] PROGMEM {
    {Host, host},
    {Netstat, netstat},
    {Ncat, ncat},
#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
    {MflnProbe, mfln_probe},
#endif
};

} // namespace commands

void setup() {
    espurna::terminal::add(commands::List);
}

} // namespace terminal
#endif

void setup() {
#if TERMINAL_SUPPORT
    terminal::setup();
#endif
}

} // namespace
} // namespace network
} // namespace espurna

void networkSetup() {
    espurna::network::setup();
}
