#if defined(LIBRETINY)

#include "fauxmoUdp.h"

#include <lwip/igmp.h>
#include <lwip/pbuf.h>
#include <lwip/tcpip.h>
#include <lwip/priv/tcpip_priv.h>

#include <cstring>

namespace {

// pcb calls run in the lwIP thread
struct Call {
    struct tcpip_api_call_data base;
    FauxmoUdp* self;
    struct udp_pcb** pcb;
    const ip4_addr_t* group;
    uint16_t port;
    const ip_addr_t* to;
    uint16_t to_port;
    const char* data;
    size_t length;
    udp_recv_fn recv;
};

err_t begin_api(struct tcpip_api_call_data* data) {
    auto* call = reinterpret_cast<Call*>(data);

    auto* pcb = udp_new();
    if (!pcb) {
        return ERR_MEM;
    }

    ip_set_option(pcb, SOF_REUSEADDR);
    auto err = udp_bind(pcb, IP_ADDR_ANY, call->port);
    if (err == ERR_OK) {
        err = igmp_joingroup(IP4_ADDR_ANY4, call->group);
    }

    if (err != ERR_OK) {
        udp_remove(pcb);
        return err;
    }

    udp_recv(pcb, call->recv, call->self);
    *call->pcb = pcb;

    return ERR_OK;
}

err_t stop_api(struct tcpip_api_call_data* data) {
    auto* call = reinterpret_cast<Call*>(data);
    if (*call->pcb) {
        igmp_leavegroup(IP4_ADDR_ANY4, call->group);
        udp_remove(*call->pcb);
        *call->pcb = nullptr;
    }

    return ERR_OK;
}

err_t send_api(struct tcpip_api_call_data* data) {
    auto* call = reinterpret_cast<Call*>(data);
    if (!*call->pcb) {
        return ERR_CONN;
    }

    auto* p = pbuf_alloc(PBUF_TRANSPORT, call->length, PBUF_RAM);
    if (!p) {
        return ERR_MEM;
    }

    pbuf_take(p, call->data, call->length);
    const auto err = udp_sendto(*call->pcb, p, call->to, call->to_port);
    pbuf_free(p);

    return err;
}

bool contains(struct pbuf* p, const char* text) {
    return pbuf_memfind(p, text, std::strlen(text), 0) != 0xFFFF;
}

} // namespace

bool FauxmoUdp::begin(const IPAddress& group, uint16_t port) {
    if (_pcb) {
        return true;
    }

    _head = 0;
    _tail = 0;
    IP4_ADDR(&_group, group[0], group[1], group[2], group[3]);

    Call call {};
    call.self = this;
    call.pcb = &_pcb;
    call.group = &_group;
    call.port = port;
    call.recv = _recv;

    return tcpip_api_call(begin_api, &call.base) == ERR_OK;
}

void FauxmoUdp::stop() {
    Call call {};
    call.pcb = &_pcb;
    call.group = &_group;
    tcpip_api_call(stop_api, &call.base);
}

bool FauxmoUdp::pop(Peer& out) {
    const auto tail = _tail.load();
    if (tail == _head.load()) {
        return false;
    }

    out = _peers[tail % Slots];
    _tail = tail + 1;

    return true;
}

bool FauxmoUdp::send(const Peer& to, const char* data, size_t length) {
    Call call {};
    call.pcb = &_pcb;
    call.to = &to.ip;
    call.to_port = to.port;
    call.data = data;
    call.length = length;

    return tcpip_api_call(send_api, &call.base) == ERR_OK;
}

// lwIP thread: same match as fauxmoESP::_handleUDP of the other platforms
void FauxmoUdp::_recv(void* arg, struct udp_pcb*, struct pbuf* p, const ip_addr_t* addr, u16_t port) {
    auto* self = reinterpret_cast<FauxmoUdp*>(arg);

    if (contains(p, "M-SEARCH")
        && (contains(p, "ssdp:discover") || contains(p, "upnp:rootdevice") || contains(p, "device:basic:1"))) {
        const auto head = self->_head.load();
        if ((head - self->_tail.load()) < Slots) {
            auto& peer = self->_peers[head % Slots];
            ip_addr_copy(peer.ip, *addr);
            peer.port = port;
            self->_head = head + 1;
        }
    }

    pbuf_free(p);
}

#endif
