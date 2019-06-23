/*

Show extended heap stats when EspClass::getHeapStats() is available

*/

#pragma once

#include <type_traits>

struct heap_stats_t {
    uint32_t available;
    uint16_t usable;
    uint8_t frag_pct;
};

namespace EspClass_has_getHeapStats {
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
void _getHeapStats(std::true_type&, T& instance, heap_stats_t& stats) {
    instance.getHeapStats(&stats.available, &stats.usable, &stats.frag_pct);
}

template <typename T>
void _getHeapStats(std::false_type&, T& instance, heap_stats_t& stats) {
    stats.available = instance.getFreeHeap();
    stats.usable = 0;
    stats.frag_pct = 0;
}

void getHeapStats(heap_stats_t& stats) {
    _getHeapStats(EspClass_has_getHeapStats::check, ESP, stats);
}

// WTF
// Calling ESP.getFreeHeap() is making the system crash on a specific
// AiLight bulb, but anywhere else it should work as expected
static bool _heap_value_wtf = false;

heap_stats_t getHeapStats() {
    heap_stats_t stats;
    if (_heap_value_wtf) {
        stats.available = 9999;
        stats.usable = 9999;
        stats.frag_pct = 0;
        return stats;
    }
    getHeapStats(stats);
    return stats;
}

void wtfHeap(bool value) {
    _heap_value_wtf = value;
}

unsigned int getFreeHeap() {
    return getHeapStats().available;
}

static unsigned int _initial_heap_value = 0;
void setInitialFreeHeap() {
    _initial_heap_value = getFreeHeap();
}

unsigned int getInitialFreeHeap() {
    if (0 == _initial_heap_value) {
        setInitialFreeHeap();
    }
    return _initial_heap_value;
}

void infoMemory(const char* name, const heap_stats_t& stats) {
    infoMemory(name, getInitialFreeHeap(), stats.available);
}

void infoHeapStats(const char* name, const heap_stats_t& stats) {
    DEBUG_MSG_P(
        PSTR("[MAIN] %-6s: %5u bytes available | %5u bytes lost (%2u%%) | %5u bytes free (%2u%%)\n"),
        name,
        stats.available,
        (stats.available - stats.usable),
        stats.frag_pct,
        stats.usable,
        (100 - stats.frag_pct)
    );
}

void infoHeapStats(bool show_frag_stats = true) {
    infoMemory("Heap", getHeapStats());
    if (show_frag_stats && EspClass_has_getHeapStats::check) {
        infoHeapStats("Heap", getHeapStats());
    }
}
