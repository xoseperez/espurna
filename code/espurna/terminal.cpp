/*

TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "terminal.h"

#if TERMINAL_SUPPORT

#include "api.h"
#include "debug.h"
#include "settings.h"
#include "system.h"
#include "telnet.h"
#include "utils.h"
#include "wifi.h"
#include "ws.h"
#include "libs/URL.h"

#include <algorithm>
#include <vector>
#include <utility>

#include <Stream.h>

#if LWIP_VERSION_MAJOR != 1

// not yet CONNECTING or LISTENING
extern struct tcp_pcb *tcp_bound_pcbs;
// accepting or sending data
extern struct tcp_pcb *tcp_active_pcbs;
// // TIME-WAIT status
extern struct tcp_pcb *tcp_tw_pcbs;

#endif

namespace {

// Based on libs/StreamInjector.h by Xose Pérez <xose dot perez at gmail dot com> (see git-log for more info)
// Instead of custom write(uint8_t) callback, we provide writer implementation in-place

struct TerminalIO : public Stream {

    TerminalIO(size_t size = 128) :
        _buffer(new char[size]),
        _size(size),
        _write(0),
        _read(0)
    {}

    ~TerminalIO() {
        delete[] _buffer;
    }

    // ---------------------------------------------------------------------
    // Injects data into the internal buffer so we can read() it
    // ---------------------------------------------------------------------

    size_t inject(char ch) {
        _buffer[_write] = ch;
        _write = (_write + 1) % _size;
        return 1;
    }

    size_t inject(char *data, size_t len) {
        for (size_t index = 0; index < len; ++index) {
            inject(data[index]);
        }
        return len;
    }

    // ---------------------------------------------------------------------
    // XXX: We are only supporting part of the Print & Stream interfaces
    //      But, we need to be have all pure virtual methods implemented
    // ---------------------------------------------------------------------

    // Return data from the internal buffer
    int available() final override {
        unsigned int bytes = 0;
        if (_read > _write) {
            bytes += (_write - _read + _size);
        } else if (_read < _write) {
            bytes += (_write - _read);
        }
        return bytes;
    }

    int peek() final override {
        int ch = -1;
        if (_read != _write) {
            ch = _buffer[_read];
        }
        return ch;
    }

    int read() final override {
        int ch = -1;
        if (_read != _write) {
            ch = _buffer[_read];
            _read = (_read + 1) % _size;
        }
        return ch;
    }

    // {Stream,Print}::flush(), see:
    // - https://github.com/esp8266/Arduino/blob/master/cores/esp8266/Print.h
    // - https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/Print.h
    // - https://github.com/arduino/ArduinoCore-API/issues/102
    // Old 2.3.0 expects flush() on Stream, latest puts in in Print
    // We may have to cheat the system and implement everything as Stream to have it available.
    void flush() final override {
        // Here, reset reader position so that we return -1 until we have new data
        // writer flushing is implemented below, we don't need it here atm
        _read = _write;
    }

    size_t write(uint8_t) final override {
        return 0;
    }

    size_t write(const uint8_t* buffer, size_t size) final override {
    // Buffer data until we encounter line break, then flush via Raw debug method
    // (which is supposed to 1-to-1 copy the data, without adding the timestamp)
#if DEBUG_SUPPORT
        if (!size) return 0;
        if (buffer[size-1] == '\0') return 0;
        if (_output.capacity() < (size + 2)) {
            _output.reserve(_output.size() + size + 2);
        }
        _output.insert(_output.end(),
            reinterpret_cast<const char*>(buffer),
            reinterpret_cast<const char*>(buffer) + size
        );
        if (_output.end() != std::find(_output.begin(), _output.end(), '\n')) {
            _output.push_back('\0');
            debugSendRaw(_output.data());
            _output.clear();
        }
#endif
        return size;
    }

    private:

#if DEBUG_SUPPORT
    std::vector<char> _output;
#endif

    char * _buffer;
    unsigned char _size;
    unsigned char _write;
    unsigned char _read;

};

auto _io = TerminalIO(TERMINAL_BUFFER_SIZE);
terminal::Terminal _terminal(_io, TERMINAL_BUFFER_SIZE);

// TODO: re-evaluate how and why this is used
#if SERIAL_RX_ENABLED
    char _serial_rx_buffer[TERMINAL_BUFFER_SIZE];
    static unsigned char _serial_rx_pointer = 0;
#endif // SERIAL_RX_ENABLED

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

void _terminalHelpCommand(const terminal::CommandContext& ctx) {

    // Get sorted list of commands
    auto commands = _terminal.commandNames();
    std::sort(commands.begin(), commands.end(), [](const String& rhs, const String& lhs) -> bool {
        return lhs.compareTo(rhs) > 0;
    });

    // Output the list asap
    ctx.output.print(F("Available commands:\n"));
    for (auto& command : commands) {
        ctx.output.printf("> %s\n", command.c_str());
    }

    terminalOK(ctx.output);

}

#if LWIP_VERSION_MAJOR != 1

String _terminalPcbStateToString(const unsigned char state) {
    switch (state) {
        case 0: return F("CLOSED");
        case 1: return F("LISTEN");
        case 2: return F("SYN_SENT");
        case 3: return F("SYN_RCVD");
        case 4: return F("ESTABLISHED");
        case 5: return F("FIN_WAIT_1");
        case 6: return F("FIN_WAIT_2");
        case 7: return F("CLOSE_WAIT");
        case 8: return F("CLOSING");
        case 9: return F("LAST_ACK");
        case 10: return F("TIME_WAIT");
        default: return String(int(state));
    };
}

void _terminalPrintTcpPcb(tcp_pcb* pcb) {

    char remote_ip[32] = {0};
    char local_ip[32] = {0};

    inet_ntoa_r((pcb->local_ip), local_ip, sizeof(local_ip));
    inet_ntoa_r((pcb->remote_ip), remote_ip, sizeof(remote_ip));

    DEBUG_MSG_P(PSTR("state=%s local=%s:%u remote=%s:%u snd_queuelen=%u lastack=%u send_wnd=%u rto=%u\n"),
            _terminalPcbStateToString(pcb->state).c_str(),
            local_ip, pcb->local_port,
            remote_ip, pcb->remote_port,
            pcb->snd_queuelen, pcb->lastack,
            pcb->snd_wnd, pcb->rto
    );

}

void _terminalPrintTcpPcbs() {

    tcp_pcb *pcb;
    //DEBUG_MSG_P(PSTR("Active PCB states:\n"));
    for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
        _terminalPrintTcpPcb(pcb);
    }
    //DEBUG_MSG_P(PSTR("TIME-WAIT PCB states:\n"));
    for (pcb = tcp_tw_pcbs; pcb != NULL; pcb = pcb->next) {
        _terminalPrintTcpPcb(pcb);
    }
    //DEBUG_MSG_P(PSTR("BOUND PCB states:\n"));
    for (pcb = tcp_bound_pcbs; pcb != NULL; pcb = pcb->next) {
        _terminalPrintTcpPcb(pcb);
    }

}

void _terminalPrintDnsResult(const char* name, const ip_addr_t* address) {
    // TODO fix asynctcp building with lwip-ipv6
    /*
    #if LWIP_IPV6
        if (IP_IS_V6(address)) {
            DEBUG_MSG_P(PSTR("[DNS] %s has IPV6 address %s\n"), name, ip6addr_ntoa(ip_2_ip6(address)));
        }
    #endif
    */
    DEBUG_MSG_P(PSTR("[DNS] %s has address %s\n"), name, ipaddr_ntoa(address));
}

void _terminalDnsFound(const char* name, const ip_addr_t* result, void*) {
    if (!result) {
        DEBUG_MSG_P(PSTR("[DNS] %s not found\n"), name);
        return;
    }

    _terminalPrintDnsResult(name, result);
}

#endif // LWIP_VERSION_MAJOR != 1

void _terminalInitCommands() {

    terminalRegisterCommand(F("COMMANDS"), _terminalHelpCommand);
    terminalRegisterCommand(F("HELP"), _terminalHelpCommand);

    terminalRegisterCommand(F("ERASE.CONFIG"), [](const terminal::CommandContext&) {
        terminalOK();
        customResetReason(CUSTOM_RESET_TERMINAL);
        eraseSDKConfig();
        *((int*) 0) = 0; // see https://github.com/esp8266/Arduino/issues/1494
    });

    terminalRegisterCommand(F("GPIO"), [](const terminal::CommandContext& ctx) {
        int pin = -1;

        if (ctx.argc < 2) {
            DEBUG_MSG("Printing all GPIO pins:\n");
        } else {
            pin = ctx.argv[1].toInt();
            if (!gpioValid(pin)) {
                terminalError(F("Invalid GPIO pin"));
                return;
            }

            if (ctx.argc > 2) {
                bool state = String(ctx.argv[2]).toInt() == 1;
                digitalWrite(pin, state);
            }
        }

        for (int i = 0; i <= 15; i++) {
            if (gpioValid(i) && (pin == -1 || pin == i)) {
                DEBUG_MSG_P(PSTR("GPIO %s pin %d is %s\n"), GPEP(i) ? "output" : "input", i, digitalRead(i) == HIGH ? "HIGH" : "LOW");
            }
        }

        terminalOK();
    });

    terminalRegisterCommand(F("HEAP"), [](const terminal::CommandContext&) {
        infoHeapStats();
        terminalOK();
    });

    terminalRegisterCommand(F("STACK"), [](const terminal::CommandContext&) {
        infoMemory("Stack", CONT_STACKSIZE, getFreeStack());
        terminalOK();
    });

    terminalRegisterCommand(F("INFO"), [](const terminal::CommandContext&) {
        info();
        terminalOK();
    });

    terminalRegisterCommand(F("RESET"), [](const terminal::CommandContext&) {
        terminalOK();
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    terminalRegisterCommand(F("RESET.SAFE"), [](const terminal::CommandContext&) {
        systemStabilityCounter(SYSTEM_CHECK_MAX);
        terminalOK();
        deferredReset(100, CUSTOM_RESET_TERMINAL);
    });

    terminalRegisterCommand(F("UPTIME"), [](const terminal::CommandContext&) {
        infoUptime();
        terminalOK();
    });

    #if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
        terminalRegisterCommand(F("MFLN.PROBE"), [](const terminal::CommandContext& ctx) {
            if (ctx.argc != 3) {
                terminalError(F("[url] [value]"));
                return;
            }

            URL _url(ctx.argv[1]);
            uint16_t requested_mfln = atol(ctx.argv[2].c_str());

            auto client = std::make_unique<BearSSL::WiFiClientSecure>();
            client->setInsecure();

            if (client->probeMaxFragmentLength(_url.host.c_str(), _url.port, requested_mfln)) {
                terminalOK();
            } else {
                terminalError(F("Buffer size not supported"));
            }
        });
    #endif

    #if LWIP_VERSION_MAJOR != 1
        terminalRegisterCommand(F("HOST"), [](const terminal::CommandContext& ctx) {
            if (ctx.argc != 2) {
                terminalError(F("HOST [hostname]"));
                return;
            }

            ip_addr_t result;
            auto error = dns_gethostbyname(ctx.argv[1].c_str(), &result, _terminalDnsFound, nullptr);
            if (error == ERR_OK) {
                _terminalPrintDnsResult(ctx.argv[1].c_str(), &result);
                terminalOK();
                return;
            } else if (error != ERR_INPROGRESS) {
                DEBUG_MSG_P(PSTR("[DNS] dns_gethostbyname error: %s\n"), lwip_strerr(error));
                return;
            }

        });

        terminalRegisterCommand(F("NETSTAT"), [](const terminal::CommandContext&) {
            _terminalPrintTcpPcbs();
        });

    #endif // LWIP_VERSION_MAJOR != 1
    
}

void _terminalLoop() {

    #if DEBUG_SERIAL_SUPPORT
        while (DEBUG_PORT.available()) {
            _io.inject(DEBUG_PORT.read());
        }
    #endif

    _terminal.process([](terminal::Terminal::Result result) {
        bool out = false;
        switch (result) {
            case terminal::Terminal::Result::CommandNotFound:
                DEBUG_MSG_P(PSTR("[TERMINAL] Command not found\n"));
                out = true;
                break;
            case terminal::Terminal::Result::BufferOverflow:
                DEBUG_MSG_P(PSTR("[TERMINAL] Command line buffer overflow\n"));
                out = true;
                break;
            case terminal::Terminal::Result::Command:
                out = true;
                break;
            case terminal::Terminal::Result::Pending:
                out = false;
                break;
            case terminal::Terminal::Result::Error:
                DEBUG_MSG_P(PSTR("[TERMINAL] Unexpected error when parsing command line\n"));
                out = false;
                break;
            case terminal::Terminal::Result::NoInput:
                out = false;
                break;
        }
        return out;
    });

    #if SERIAL_RX_ENABLED

        while (SERIAL_RX_PORT.available() > 0) {
            char rc = SERIAL_RX_PORT.read();
            _serial_rx_buffer[_serial_rx_pointer++] = rc;
            if ((_serial_rx_pointer == TERMINAL_BUFFER_SIZE) || (rc == 10)) {
                terminalInject(_serial_rx_buffer, (size_t) _serial_rx_pointer);
                _serial_rx_pointer = 0;
            }
        }

    #endif // SERIAL_RX_ENABLED

}

}

// -----------------------------------------------------------------------------
// Pubic API
// -----------------------------------------------------------------------------

Stream & terminalDefaultStream() {
    return (Stream &) _io;
}

void terminalInject(void *data, size_t len) {
    _io.inject((char *) data, len);
}

void terminalInject(char ch) {
    _io.inject(ch);
}

void terminalRegisterCommand(const String& name, terminal::Terminal::CommandFunc func) {
    terminal::Terminal::addCommand(name, func);
};

void terminalOK(Print& print) {
    print.print(F("+OK\n"));
}

void terminalError(Print& print, const String& error) {
    print.printf("-ERROR: %s\n", error.c_str());
}

void terminalOK(const terminal::CommandContext& ctx) {
    terminalOK(ctx.output);
}

void terminalError(const terminal::CommandContext& ctx, const String& error) {
    terminalError(ctx.output, error);
}

void terminalOK() {
    terminalOK(_io);
}

void terminalError(const String& error) {
    terminalError(_io, error);
}

template <typename T>
struct StreamAdapter : public Stream {
    StreamAdapter(Print& writer, T&& begin, T&& end) :
        _writer(writer),
        _current(std::forward<T>(begin)),
        _begin(std::forward<T>(begin)),
        _end(std::forward<T>(end))
    {}

    int available() final override {
        return (_end - _current);
    }

    int peek() final override {
        if (available() && (_end != (1 + _current))) {
            return *(1 + _current);
        }
        return -1;
    }

    int read() final override {
        if (_end != _current) {
            return *(_current++);
        }
        return -1;
    }

    void flush() final override {
// 2.3.0  - Stream::flush()
// latest - Print::flush()
#if not defined(ARDUINO_ESP8266_RELEASE_2_3_0)
        _writer.flush();
#endif
    }

    size_t write(const uint8_t* buffer, size_t size) final override {
        return _writer.write(buffer, size);
    }

    size_t write(uint8_t ch) final override {
        return _writer.write(ch);
    }

    protected:

    Print& _writer;

    T _current;
    T const _begin;
    T const _end;
};

void terminalSetup() {

    #if WEB_SUPPORT
        wsRegister()
            .onVisible([](JsonObject& root) { root["cmdVisible"] = 1; });

        // TODO: each command needs to be re-wired to output into ctx.output
        // TODO: 2.3.0 Print::printf_P is missing (there is Print::println(F(...)) though)
        webRequestRegister([](AsyncWebServerRequest* request) {
            const auto url = request->url();
            if (!url.equals("/cmd")) return false;

            webLog(request);
            if (!apiAuthenticate(request)) return true;

            // TODO: batch requests?
            auto* cmd_param = request->getParam("line");
            if (!cmd_param) {
                request->send(500);
                return true;
            }

            auto cmd = cmd_param->value();
            if (!cmd.length()) {
                request->send(500);
                return true;
            }

            if (!cmd.endsWith("\r\n") && !cmd.endsWith("\n")) {
                cmd += '\n';
            }

            AsyncWebPrint::scheduleFromRequest(request, [cmd](Print& print) {
                StreamAdapter<const char*> stream(print, cmd.c_str(), cmd.c_str() + cmd.length() + 1);
                terminal::Terminal handler(stream);
                handler.processLine();
            });

            return true;
        });
    #endif

    _terminalInitCommands();

    #if SERIAL_RX_ENABLED
        SERIAL_RX_PORT.begin(SERIAL_RX_BAUDRATE);
    #endif // SERIAL_RX_ENABLED

    // Register loop
    espurnaRegisterLoop(_terminalLoop);

}

#endif // TERMINAL_SUPPORT 

