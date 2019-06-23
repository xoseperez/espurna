#pragma once

void debugSendImpl(const char*);

template <typename ...Args>
void debugSend(const char * format, Args... args) {

    char temp[64];
    int len = snprintf(temp, sizeof(temp), format, args...);
    if (len < 64) { debugSendImpl(temp); return; }

    auto buffer = new char[len + 1];
    if (!buffer) return;

    snprintf(buffer, len + 1, format, args...);
    debugSendImpl(buffer);

    delete[] buffer;

}

template <typename ...Args>
void debugSend_P(PGM_P format_P, Args... args) {

    char format[strlen_P(format_P) + 1];
    memcpy_P(format, format_P, sizeof(format));

    debugSend(format, args...);

}
