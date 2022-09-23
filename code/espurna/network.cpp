/*

NETWORKING MODULE

Copyright (C) 2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#include <functional>
#include <utility>
#include <memory>

#include <lwip/init.h>
#include <lwip/dns.h>

#include "libs/URL.h"

// not yet CONNECTING or LISTENING
extern "C" struct tcp_pcb *tcp_bound_pcbs;
// accepting or sending data
extern "C" struct tcp_pcb *tcp_active_pcbs;
// // TIME-WAIT status
extern "C" struct tcp_pcb *tcp_tw_pcbs;

namespace espurna {
namespace network {
namespace {

namespace dns {
namespace internal {

struct Task {
    Task() = delete;
    explicit Task(String hostname, IpFoundCallback callback) :
        _hostname(std::move(hostname)),
        _callback(std::move(callback))
    {}

    IPAddress addr() const {
        return _addr;
    }

    const String& hostname() const {
        return _hostname;
    }

    void found_callback(const char* name, const ip_addr_t* addr, void*) {
        _callback(name, addr);
    }

    void found_callback() {
        _callback(_hostname, _addr);
    }

private:
    IPAddress _addr { IPADDR_NONE };
    String _hostname;

    IpFoundCallback _callback;
};

using TaskPtr = std::unique_ptr<Task>;
TaskPtr task;

void found_callback(const char* name, const ip_addr_t* addr, void* arg) {
    if (task) {
        task->found_callback(name, addr, arg);
        task.reset();
    }
}

} // namespace internal

bool started() {
    return static_cast<bool>(internal::task);
}

void start(String hostname, IpFoundCallback callback) {
    auto task = std::make_unique<internal::Task>(
            std::move(hostname), std::move(callback));

    const auto result = dns_gethostbyname(
            task->hostname().c_str(), task->addr(),
            internal::found_callback, nullptr);

    switch (result) {
    // No need to wait, return result immediately
    case ERR_OK:
        task->found_callback();
        break;
    // Task needs to linger for a bit
    case ERR_INPROGRESS:
        internal::task = std::move(task);
        break;
    }
}

} // namespace dns

#if TERMINAL_SUPPORT
namespace terminal {
namespace commands {

void host(::terminal::CommandContext&& ctx) {
    if (ctx.argv.size() != 2) {
        terminalError(ctx, F("HOST <hostname>"));
        return;
    }

    dns::start(std::move(ctx.argv[1]),
        [&](const String& name, IPAddress addr) {
            if (!addr) {
                ctx.output.printf_P(PSTR("%s not found\n"), name.c_str());
                return;
            }

            ctx.output.printf_P(PSTR("%s has address %s\n"),
                name.c_str(), addr.toString().c_str());
        });

    while (dns::started()) {
        delay(10);
    }
}

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

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
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

} // namespace commands

void setup() {
    terminalRegisterCommand(F("NETSTAT"), commands::netstat);
    terminalRegisterCommand(F("HOST"), commands::host);
#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
    terminalRegisterCommand(F("MFLN.PROBE"), commands::mfln_probe);
#endif
}

} // namespace terminal
#endif

void gethostbyname(String hostname, IpFoundCallback callback) {
    dns::start(std::move(hostname), std::move(callback));
}

IPAddress gethostbyname(String hostname) {
    IPAddress out;

    dns::start(std::move(hostname),
        [&](const String& name, IPAddress ip) {
            if (!ip.isSet()) {
                return;
            }

            out = ip;
        });

    while (dns::started()) {
        delay(10);
    }

    return out;
}

void setup() {
#if TERMINAL_SUPPORT
    terminal::setup();
#endif
}

} // namespace
} // namespace network
} // namespace espurna

void networkGetHostByName(String hostname, espurna::network::IpFoundCallback callback) {
    return espurna::network::gethostbyname(std::move(hostname), std::move(callback));
}

IPAddress networkGetHostByName(String hostname) {
    return espurna::network::gethostbyname(std::move(hostname));
}

void networkSetup() {
    espurna::network::setup();
}
