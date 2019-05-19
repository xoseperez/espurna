/*

Show extended heap stats when EspClass::getHeapStats() is available

*/

#pragma once

#include <type_traits>

template<typename T> struct has_getHeapStats {
private:
    static int detect(...);
    template<typename U> static decltype(std::declval<U>().getHeapStats()) detect(const U&);
public:
    static constexpr bool value = std::is_same<void, decltype(detect(std::declval<T>()))>::value;
};

template <typename T, T& instance>
typename std::enable_if<has_getHeapStats<T>::value, void>::type
getHeapStats(uint32_t* free, uint16_t* max, uint8_t* frag) {
    instance.getHeapStats(free, max, frag);
}

template <typename T, T& instance>
typename std::enable_if<not has_getHeapStats<EspClass>::value, void>::type
getHeapStats(uint32_t* free, uint16_t* max, uint8_t* frag) {
    *free = getFreeHeap();
    *max = 0;
    *frag = 0;
}

void infoHeapStats() {
    uint32_t free;
    uint16_t max;
    uint8_t frag;
    getHeapStats<EspClass, ESP>(&free, &max, &frag);
    infoMemory("Heap", getInitialFreeHeap(), free);
    if ((max > 0) || (frag > 0)) {
        DEBUG_MSG_P(PSTR("[MAIN] %-6s: %5u bytes usable, %2u%% fragmentation\n"), "Heap", max, frag);
    }
}
