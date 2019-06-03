/*

Show extended heap stats when EspClass::getHeapStats() is available

*/

#pragma once

#include <type_traits>

namespace has_getHeapStats {
    struct _detector {
        template<typename T, typename = decltype(
                std::declval<T>().getHeapStats(0,0,0))>
          static std::true_type detect(int);

        template<typename>
          static std::false_type detect(...);
    };

    template <typename T>
    struct detector : public _detector {
        using result = decltype(
                std::declval<detector>().detect<T>(0));
    };

    template <typename T>
    struct typed_check : public detector<T>::result {
    };

    typed_check<EspClass> check{};
};

template <typename T>
void _getHeapStats(std::true_type&, T& instance, uint32_t* free, uint16_t* max, uint8_t* frag) {
    instance.getHeapStats(free, max, frag);
}

template <typename T>
void _getHeapStats(std::false_type&, T& instance, uint32_t* free, uint16_t* max, uint8_t* frag) {
    *free = instance.getFreeHeap();
    *max = 0;
    *frag = 0;
}

inline void getHeapStats(uint32_t* free, uint16_t* max, uint8_t* frag) {
    _getHeapStats(has_getHeapStats::check, ESP, free, max, frag);
}

void infoHeapStats() {
    uint32_t free;
    uint16_t max;
    uint8_t frag;
    getHeapStats(&free, &max, &frag);
    infoMemory("Heap", getInitialFreeHeap(), free);
    if (has_getHeapStats::check) {
        DEBUG_MSG_P(PSTR("[MAIN] %-6s: %5u bytes usable, %2u%% fragmentation\n"), "Heap", max, frag);
    }
}
