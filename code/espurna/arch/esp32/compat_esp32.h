#pragma once

#if defined(ESP32)

#include <Arduino.h>
#include <pgmspace.h>
#include <memory>
#include <type_traits>
#include <Update.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_system.h>

// ESP8266-specific memory functions
#define memmove_P memmove
#define memcpy_P memcpy
#define strncpy_P strncpy
#define strncasecmp_P strncasecmp
#define strcasecmp_P strcasecmp
#define strcmp_P strcmp

// Missing ADC/UART/TZ constants/functions
#define ADC_VCC 255
#define ADC_TOUT 254
#undef ADC_MODE_VALUE
#define ADC_MODE_VALUE ADC_TOUT
inline void uart_set_debug(uint8_t) {}
#define TZ_Etc_UTC "UTC0"

// Stack info — uxTaskGetStackHighWaterMark returns words; convert to bytes for ESP8266 parity.
#define getFreeStack() (uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t))

// ESP class shim helper, same role as ESP.getResetReason() / getResetInfo() on esp8266
inline String getResetReasonShim() {
    switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return F("Power On");
    case ESP_RST_EXT: return F("External System");
    case ESP_RST_SW: return F("Software/System restart");
    case ESP_RST_PANIC: return F("Exception");
    case ESP_RST_INT_WDT: return F("Interrupt Watchdog");
    case ESP_RST_TASK_WDT: return F("Task Watchdog");
    case ESP_RST_WDT: return F("Watchdog");
    case ESP_RST_DEEPSLEEP: return F("Deep-Sleep Wake");
    case ESP_RST_BROWNOUT: return F("Brownout");
    case ESP_RST_SDIO: return F("SDIO");
    case ESP_RST_UNKNOWN:
        break;
    }
    return F("Unknown");
}

inline String getResetInfoShim() {
    return String(F("Reset reason: ")) + String(static_cast<int>(esp_reset_reason()), 10);
}

// Xtensa cycle counter — exposed both as a free function (matches the
// arduino-esp32 SDK name) and via EspCompat::getCycleCount() so existing
// ESP.getCycleCount() callers keep working unchanged.
inline uint32_t esp_get_cycle_count() {
    uint32_t ccount;
    __asm__ __volatile__("rsr %0, ccount" : "=a" (ccount));
    return ccount;
}

// We use a macro for ESP to intercept reset calls without changing source code
extern uint16_t systemVcc();
struct EspCompat {
    String getResetReason() { return getResetReasonShim(); }
    String getResetInfo() { return getResetInfoShim(); }
    uint16_t getVcc() { return systemVcc(); }
    
    EspClass* operator->() { return &ESP; }
    operator EspClass&() { return ESP; }
    
    uint32_t getFreeHeap() { return ESP.getFreeHeap(); }
    // Same as esp8266, last 3 bytes of the STA MAC (efuse value starts with the vendor OUI)
    uint32_t getChipId() {
        uint8_t mac[6] {};
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        return (uint32_t{mac[3]} << 16) | (uint32_t{mac[4]} << 8) | uint32_t{mac[5]};
    }
    uint32_t getEfuseMac() { return (uint32_t)ESP.getEfuseMac(); }

    // OTA and Flash related — delegate to Arduino-ESP32 core (which reads efuse / SPI flash).
    uint32_t magicFlashChipSize(uint8_t byte) { return ESP.getFlashChipSize(); }
    uint32_t getFlashChipRealSize() { return ESP.getFlashChipSize(); }
    uint32_t getFlashChipSize() { return ESP.getFlashChipSize(); }
    uint32_t getFlashChipSpeed() { return ESP.getFlashChipSpeed(); }
    uint32_t getFlashChipMode() { return (uint32_t)ESP.getFlashChipMode(); }
    uint32_t getFlashChipId() { return 0; }

    uint32_t getFreeSketchSpace() { return ESP.getFreeSketchSpace(); }
    uint32_t getSketchSize() { return ESP.getSketchSize(); }
    String getSketchMD5() { return ESP.getSketchMD5(); }

    // High-water mark is returned in stack words by FreeRTOS; convert to bytes.
    uint32_t getFreeContStack() { return uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t); }
    
    void restart() { ESP.restart(); }
    bool eraseConfig() { return false; }
    
    bool flashRead(uint32_t addr, uint32_t *data, size_t size) { return false; }
    
    void getHeapStats(uint32_t* free, void* max, uint8_t* frag) {
        if (free) *free = ESP.getFreeHeap();
        if (max) *(uint32_t*)max = ESP.getMaxAllocHeap();
        if (frag) *frag = 0;
    }

    uint32_t getCycleCount() { return esp_get_cycle_count(); }
};

extern EspCompat ESP32_ESP;
#define ESP ESP32_ESP

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
