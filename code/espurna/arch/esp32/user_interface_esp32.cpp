#if defined(ESP32)

#include "user_interface_esp32.h"
#include <esp_system.h>

extern "C" {
    struct rst_info resetInfo = {0};
}

// Helper to populate resetInfo on ESP32
// Using constructor attribute to ensure it runs before setup()
void system_init_reset_info() __attribute__((constructor));
void system_init_reset_info() {
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_POWERON:  resetInfo.reason = REASON_DEFAULT_RST; break;
        case ESP_RST_EXT:      resetInfo.reason = REASON_EXT_SYS_RST; break;
        case ESP_RST_SW:       resetInfo.reason = REASON_SOFT_RESTART; break;
        case ESP_RST_PANIC:    resetInfo.reason = REASON_EXCEPTION_RST; break;
        case ESP_RST_INT_WDT:  resetInfo.reason = REASON_WDT_RST; break;
        case ESP_RST_TASK_WDT: resetInfo.reason = REASON_WDT_RST; break;
        case ESP_RST_WDT:      resetInfo.reason = REASON_WDT_RST; break;
        case ESP_RST_DEEPSLEEP: resetInfo.reason = REASON_DEEP_SLEEP_AWAKE; break;
        case ESP_RST_BROWNOUT: resetInfo.reason = REASON_DEFAULT_RST; break;
        case ESP_RST_SDIO:     resetInfo.reason = REASON_DEFAULT_RST; break;
        default:               resetInfo.reason = REASON_DEFAULT_RST; break;
    }
}

#endif
