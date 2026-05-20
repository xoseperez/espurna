#pragma once

#if defined(ESP32)

#include <Arduino.h>
#include <pgmspace.h>
#include <memory>
#include <type_traits>
#include <Update.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ESP8266-specific memory functions
#define memmove_P memmove
#define memcpy_P memcpy
#define strncpy_P strncpy
#define strncasecmp_P strncasecmp
#define strcasecmp_P strcasecmp
#define strcmp_P strcmp

// Missing ADC/UART/TZ constants/functions
#define ADC_VCC 255
#ifndef ADC_MODE_VALUE
#define ADC_MODE_VALUE ADC_VCC
#endif
inline void uart_set_debug(uint8_t) {}
#define TZ_Etc_UTC "UTC0"

// Stack info
#define getFreeStack() (uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t))

// ESP class shim helper
inline String getResetReasonShim() { return "ESP32 Reset"; }
inline String getResetInfoShim() { return "ESP32 Reset Info"; }

// We use a macro for ESP to intercept reset calls without changing source code
extern uint16_t systemVcc();
struct EspCompat {
    String getResetReason() { return getResetReasonShim(); }
    String getResetInfo() { return getResetInfoShim(); }
    uint16_t getVcc() { return systemVcc(); }
    
    EspClass* operator->() { return &ESP; }
    operator EspClass&() { return ESP; }
    
    uint32_t getFreeHeap() { return ESP.getFreeHeap(); }
    uint32_t getChipId() { return (uint32_t)ESP.getEfuseMac(); }
    uint32_t getEfuseMac() { return (uint32_t)ESP.getEfuseMac(); }
    
    // OTA and Flash related
    uint32_t magicFlashChipSize(uint8_t byte) { return 0; }
    uint32_t getFlashChipRealSize() { return 4 * 1024 * 1024; }
    uint32_t getFlashChipSize() { return 4 * 1024 * 1024; }
    uint32_t getFlashChipSpeed() { return 40000000; }
    uint32_t getFlashChipMode() { return 0; }
    uint32_t getFlashChipId() { return 0; }
    
    uint32_t getFreeSketchSpace() { return ESP.getFreeSketchSpace(); }
    uint32_t getSketchSize() { return 0; }
    String getSketchMD5() { return "00000000000000000000000000000000"; }
    
    uint32_t getFreeContStack() { return uxTaskGetStackHighWaterMark(NULL); }
    
    void restart() { ESP.restart(); }
    bool eraseConfig() { return false; }
    
    bool flashRead(uint32_t addr, uint32_t *data, size_t size) { return false; }
    
    void getHeapStats(uint32_t* free, void* max, uint8_t* frag) {
        if (free) *free = ESP.getFreeHeap();
        if (max) *(uint32_t*)max = ESP.getMaxAllocHeap();
        if (frag) *frag = 0;
    }

    uint32_t getCycleCount() {
        uint32_t ccount;
        __asm__ __volatile__("rsr %0, ccount" : "=a" (ccount));
        return ccount;
    }
};

extern EspCompat ESP32_ESP;
#define ESP ESP32_ESP

// Missing cycle/time functions shimmed globally
inline uint32_t esp_get_cycle_count() {
    uint32_t ccount;
    __asm__ __volatile__("rsr %0, ccount" : "=a" (ccount));
    return ccount;
}

inline uint64_t micros64() {
    return (uint64_t)esp_timer_get_time();
}

// FLASH_SECTOR_SIZE fallback
#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE 4096
#endif

// UART related shims for uart.cpp
// (SerialConfig is defined in esp32-hal-uart.h in newer cores)
// using SerialConfig = uint32_t;
using SerialMode = uint32_t;

// Timer shims
#ifdef __cplusplus
#include <Ticker.h>
struct os_timer_t : public Ticker {
    void (*func)(void*);
    void* arg;
};
#else
typedef struct {
    void* func;
    void* arg;
} os_timer_t;
#endif

#ifndef SERIAL_RX_ONLY
#define SERIAL_RX_ONLY 1
#endif
#ifndef SERIAL_TX_ONLY
#define SERIAL_TX_ONLY 2
#endif
#ifndef SERIAL_FULL
#define SERIAL_FULL 3
#endif

// SDK Sleep functions shims for system.cpp
// We use inline functions instead of macros to avoid conflict with extern "C" declarations
extern "C" {
    inline bool fpm_is_open() { return false; }
    inline bool fpm_rf_is_closed() { return true; }
    inline void wifi_fpm_do_wakeup() {}
    inline void wifi_fpm_close() {}
    inline void wifi_fpm_set_sleep_type(uint8_t) {}
    inline void wifi_fpm_open() {}
    inline int wifi_fpm_do_sleep(uint32_t) { return 0; }
    inline void wifi_fpm_auto_sleep_set_in_null_mode(uint8_t) {}
    inline void wifi_fpm_set_wakeup_cb(void (*)()) {}
    inline void wifi_enable_gpio_wakeup(uint32_t, uint8_t) {}
    inline void wifi_disable_gpio_wakeup() {}
    inline void system_deep_sleep_set_option(uint8_t) {}
    inline bool system_deep_sleep(uint32_t) { return false; }
}

#define RF_DEFAULT 0

// std library shims for C++11
namespace std {
    #if __cplusplus < 201703L
    template<bool B, class T = void>
    using enable_if_t = typename enable_if<B, T>::type;

    template<typename F, typename... Args>
    struct invoke_result_helper {
        using type = decltype(std::declval<F>()(std::declval<Args>()...));
    };

    template<typename F, typename... Args>
    using invoke_result_t = typename invoke_result_helper<F, Args...>::type;
    #endif
}

// Special hack for settings_embedis.h
#if defined(ESP32) && (__cplusplus < 201703L)
#undef __cpp_lib_result_of_sfinae
#endif

#endif
