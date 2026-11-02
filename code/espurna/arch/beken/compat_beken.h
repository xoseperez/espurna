#pragma once

#if defined(LIBRETINY)

#include <Arduino.h>
#include <memory>
#include <type_traits>
#include <Update.h>

#include <FreeRTOS.h>
#include <task.h>
#include "mutex_beken.h"

// LibreTiny renames Arduino delay() through a function-like macro (SDK has its own delay()), which
// also hits members and arguments named 'delay'. Arduino.h is already in, the macro is no longer needed
#ifdef delay
#undef delay
inline void delay(unsigned long ms) {
    delayMilliseconds(ms);
}
#endif

// ESP8266-specific memory functions
#ifndef memmove_P
#define memmove_P memmove
#endif
#ifndef memcpy_P
#define memcpy_P memcpy
#endif
#ifndef strncpy_P
#define strncpy_P strncpy
#endif
#ifndef strncasecmp_P
#define strncasecmp_P strncasecmp
#endif
#ifndef strcasecmp_P
#define strcasecmp_P strcasecmp
#endif
#ifndef strcmp_P
#define strcmp_P strcmp
#endif

// FreeRTOS port interrupt mask (IRQ & FIQ bits of CPSR), nesting-safe and usable from the ISR. Critical section macros
// of the port open a block scope, these do not
extern "C" int port_disable_interrupts_flag(void);
extern "C" void port_enable_interrupts_flag(int);

// Code runs from the flash anyway, nothing to place in IRAM
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

// Print has printf(), strings are not in a separate address space
#ifndef printf_P
#define printf_P printf
#endif

// Missing ADC/UART/TZ constants/functions
#define ADC_VCC 255
#define ADC_TOUT 254
#undef ADC_MODE_VALUE
#define ADC_MODE_VALUE ADC_TOUT
inline void uart_set_debug(uint8_t) {}

// LibreTiny posix/itoa.c has it, ArduinoCore-API itoa.h is not included by Arduino.h
extern "C" char* ultoa(unsigned long value, char* string, int radix);

// Preemptive scheduler, the loop task does not starve anything else
inline void optimistic_yield(uint32_t) {}
#define TZ_Etc_UTC "UTC0"

// Same as esp8266 FlashMode_t, LibreTiny does not report the mode (getFlashChipMode() is 0xFF)
enum FlashMode_t {
    FM_QIO = 0x00,
    FM_QOUT = 0x01,
    FM_DIO = 0x02,
    FM_DOUT = 0x03,
    FM_UNKNOWN = 0xff,
};

// Stack info, uxTaskGetStackHighWaterMark returns words; convert to bytes for ESP8266 parity.
#define getFreeStack() (uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t))

// LibreTiny EspClass already follows the ESP8266 API, this only adds what ESPurna uses on top of it
extern uint16_t systemVcc();
struct EspCompat {
    String getResetReason() { return ::ESP.getResetReason(); }
    String getResetInfo() { return ::ESP.getResetInfo(); }
    uint16_t getVcc() { return systemVcc(); }

    EspClass* operator->() { return &::ESP; }
    operator EspClass&() { return ::ESP; }

    uint32_t getFreeHeap() { return lt_heap_get_free(); }
    // Same as esp8266, last 3 bytes of the STA MAC
    uint32_t getChipId() {
        uint8_t mac[6] {};
        lt_get_device_mac(mac);
        return (uint32_t{mac[3]} << 16) | (uint32_t{mac[4]} << 8) | uint32_t{mac[5]};
    }

    uint32_t magicFlashChipSize(uint8_t) { return lt_flash_get_size(); }
    uint32_t getFlashChipRealSize() { return lt_flash_get_size(); }
    uint32_t getFlashChipSize() { return lt_flash_get_size(); }
    uint32_t getFlashChipSpeed() { return 0; }
    uint32_t getFlashChipMode() { return 0xFF; }
    uint32_t getFlashChipId() { return ::ESP.getFlashChipId(); }

    // OTA image goes into the 'download' partition and is compressed, report the partition size
    uint32_t getFreeSketchSpace();
    uint32_t getSketchSize();
    String getSketchMD5() { return String(); }

    uint32_t getFreeContStack() { return uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t); }

    void restart() { lt_reboot(); }
    bool eraseConfig() { return false; }

    bool flashRead(uint32_t addr, uint32_t* data, size_t size) {
        return lt_flash_read(addr, reinterpret_cast<uint8_t*>(data), size) == size;
    }

    void getHeapStats(uint32_t* free, void* max, uint8_t* frag) {
        if (free) *free = lt_heap_get_free();
        if (max) *(uint32_t*)max = lt_heap_get_max_alloc();
        if (frag) *frag = 0;
    }

    uint32_t getCycleCount() { return lt_cpu_get_cycle_count(); }
};

extern EspCompat BEKEN_ESP;
#define ESP BEKEN_ESP

uint64_t micros64();

#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE 4096
#endif

#ifndef SPI_FLASH_SEC_SIZE
#define SPI_FLASH_SEC_SIZE FLASH_SECTOR_SIZE
#endif

using SerialMode = uint32_t;
using SerialConfig = uint16_t;

// Timer shims
struct os_timer_t {
    void (*func)(void*);
    void* arg;
};

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
    #if __cplusplus < 201402L
    template<bool B, class T = void>
    using enable_if_t = typename enable_if<B, T>::type;
    #endif
    #if __cplusplus < 201703L
    template<typename F, typename... Args>
    struct invoke_result_helper {
        using type = decltype(std::declval<F>()(std::declval<Args>()...));
    };

    template<typename F, typename... Args>
    using invoke_result_t = typename invoke_result_helper<F, Args...>::type;
    #endif
}

#if __cplusplus < 201703L
#undef __cpp_lib_result_of_sfinae
#endif

#endif
