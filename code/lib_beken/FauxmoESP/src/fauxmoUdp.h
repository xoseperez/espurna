// SSDP (M-SEARCH) listener of fauxmoESP for LibreTiny, straight on a lwIP udp_pcb.
// WiFiUDP of LibreTiny keeps a BSD socket and a 1460 byte send buffer for as long as it is open, and reads every
// packet through two more 1460 byte buffers: about 3.5 KiB of a Beken heap that Matter needs. This one keeps a pcb and
// the IGMP membership, the receive callback (lwIP thread) only notes who asked, the reply is sent from the loop

#pragma once

#if defined(LIBRETINY)

#include <Arduino.h>

#include <lwip/ip_addr.h>
#include <lwip/udp.h>

#include <atomic>

class FauxmoUdp {
public:
    struct Peer {
        ip_addr_t ip;
        uint16_t port;
    };

    bool begin(const IPAddress& group, uint16_t port);
    void stop();

    // Next sender of a matching M-SEARCH (loop task)
    bool pop(Peer& out);

    bool send(const Peer& to, const char* data, size_t length);

private:
    static void _recv(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, u16_t port);

    // a reply or two per search burst is enough, the rest is dropped
    static constexpr size_t Slots { 4 };
    Peer _peers[Slots];
    std::atomic<size_t> _head { 0 };
    std::atomic<size_t> _tail { 0 };

    struct udp_pcb* _pcb { nullptr };
    ip4_addr_t _group;
};

#endif
