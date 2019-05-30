/*

Show extended heap stats when EspClass::getHeapStats() is available

*/

#pragma once

#include <type_traits>

namespace has_getHeapStats {
    struct detector {
        template<typename T, typename = decltype(declval<T&>().getHeapStats())>
          static std::true_type detect(int);

        template<typename>
          static std::false_type detect(...);
    };

    template <typename T>
    struct trait : public detector {
        using type = decltype(detect<T>(0));
    };

    template <typename T>
    struct typed_check : public trait<T>::type {
    };

    using type = decltype(typed_check<EspClass>());
    constexpr bool value = type::value;
};

template <typename T>
void _getHeapStats(std::true_type, T& instance, uint32_t* free, uint16_t* max, uint8_t* frag) {
    instance.getHeapStats(free, max, frag);
}

template <typename T>
void _getHeapStats(std::false_type, T& instance, uint32_t* free, uint16_t* max, uint8_t* frag) {
    *free = instance.getFreeHeap();
    *max = 0;
    *frag = 0;
}

inline void getHeapStats(uint32_t* free, uint16_t* max, uint8_t* frag) {
    _getHeapStats(has_getHeapStats::type{}, ESP, free, max, frag);
}

void infoHeapStats() {
    uint32_t free;
    uint16_t max;
    uint8_t frag;
    getHeapStats(&free, &max, &frag);
    infoMemory("Heap", getInitialFreeHeap(), free);
    if (has_getHeapStats::value) {
        DEBUG_MSG_P(PSTR("[MAIN] %-6s: %5u bytes usable, %2u%% fragmentation\n"), "Heap", max, frag);
    }
}
