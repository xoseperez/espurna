/*

NSPANEL MODULE

Sonoff NSPanel (gen1, ESP32) display backend for the OHpanel TFT (nspanel_ohpanel.h) and the
joBr99 "NSPanel Lovelace UI" TFT (nspanel_lovelace.h), detected automatically,
(flash layout and updates: arch/esp32/safeboot_esp32.cpp, as on every ESP32 image)

*/

#pragma once

// Early, right after the basic modules (works in the 'unstable' system mode as well):
// display power and buzzer pins (safebootSetup() follows with the crash-loop guard and `safeboot`)
void nspanelBootSetup();

// Display backend, MQTT, web endpoints, weather and temperature
void nspanelSetup();
