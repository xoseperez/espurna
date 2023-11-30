// isn't normally used or exported, needs something external to link them in
// (for us, $OBJCOPY w/ redefine-sym on the postmortem file is the main target)

// note that functions actually return `int`, but it is a totally valid C
// to return whatever type we want instead (e.g. `void` aka nothing)

#include <c_types.h>

void __stub_printf(const char* format, ...) IRAM_ATTR;
void __stub_printf(const char* format __attribute__((unused)), ...) {
}

void __stub_printf_P(const char* format, ...) IRAM_ATTR;
void __stub_printf_P(const char* format __attribute__((unused)), ...) {
}

void __stub_putc(char ch) IRAM_ATTR;
void __stub_putc(char ch __attribute__((unused))) {
}
