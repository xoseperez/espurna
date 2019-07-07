// -----------------------------------------------------------------------------
// printf-like debug methods
// -----------------------------------------------------------------------------

#pragma once

void debugSendImpl(const char*);

void _debugSend(const char * format, va_list args) {

    char temp[64];
    int len = ets_vsnprintf(temp, sizeof(temp), format, args);
    if (len < 64) { debugSendImpl(temp); return; }

    auto buffer = new char[len + 1];
    ets_vsnprintf(buffer, len + 1, format, args);

    debugSendImpl(buffer);

    delete[] buffer;

}

void debugSend(const char* format, ...) {

    va_list args;
    va_start(args, format);

    _debugSend(format, args);

    va_end(args);

}

void debugSend_P(PGM_P format_P, ...) {

    char format[strlen_P(format_P) + 1];
    memcpy_P(format, format_P, sizeof(format));

    va_list args;
    va_start(args, format_P);

    _debugSend(format, args);

    va_end(args);

}
