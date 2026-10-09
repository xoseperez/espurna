/*

CORE DUMP FOR ESP32

On panic, ESP-IDF saves a core dump into the 'coredump' partition. Show the
crashed task, exception and backtrace from it, same purpose as the esp8266
crash recorder. Decode the addresses with the matching firmware.elf, e.g.
xtensa-esp32-elf-addr2line -pfiaC -e firmware.elf <PC>:<SP> ...

*/

#include "espurna.h"

#if defined(ARDUINO_ARCH_ESP32) && TERMINAL_SUPPORT

#include "terminal.h"

#include <esp_core_dump.h>

#include <memory>

namespace espurna {
namespace coredump {
namespace {

STRING_VIEW_INLINE(Coredump, "COREDUMP");

void command(::terminal::CommandContext&& ctx) {
    size_t address { 0 };
    size_t size { 0 };
    if (esp_core_dump_image_get(&address, &size) != ESP_OK) {
        terminalError(ctx, F("No core dump saved"));
        return;
    }

    // ~200 bytes, keep it off the loop task stack
    auto summary = std::make_unique<esp_core_dump_summary_t>();
    const auto err = esp_core_dump_get_summary(summary.get());
    if (err != ESP_OK) {
        terminalError(ctx, String(F("Could not read core dump, error 0x")) + String(err, 16));
        return;
    }

    ctx.output.printf_P(PSTR("image: 0x%08X, %u bytes\n"), address, size);
    ctx.output.printf_P(PSTR("task: %s\n"), summary->exc_task);
    ctx.output.printf_P(PSTR("pc: 0x%08X\n"), summary->exc_pc);
    ctx.output.printf_P(PSTR("exccause: %u excvaddr: 0x%08X\n"),
        summary->ex_info.exc_cause, summary->ex_info.exc_vaddr);

    ctx.output.print(F("backtrace:"));
    for (uint32_t index = 0; index < summary->exc_bt_info.depth; ++index) {
        ctx.output.printf_P(PSTR(" 0x%08X"), summary->exc_bt_info.bt[index]);
    }
    ctx.output.printf_P(PSTR("%s\n"),
        summary->exc_bt_info.corrupted ? PSTR(" (corrupted)") : PSTR(""));

    terminalOK(ctx);
}

STRING_VIEW_INLINE(CoredumpErase, "COREDUMP.ERASE");

void command_erase(::terminal::CommandContext&& ctx) {
    if (esp_core_dump_image_erase() != ESP_OK) {
        terminalError(ctx, F("Could not erase core dump"));
        return;
    }

    terminalOK(ctx);
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Coredump, command},
    {CoredumpErase, command_erase},
};

} // namespace
} // namespace coredump
} // namespace espurna

void coredumpSetup() {
    espurna::terminal::add(espurna::coredump::Commands);
}

#endif
