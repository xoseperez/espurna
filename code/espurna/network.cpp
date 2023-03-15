/*

NETWORKING MODULE

Copyright (C) 2022 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"

#include <forward_list>
#include <functional>
#include <memory>
#include <utility>

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
