/*

MATTER MODULE

(espurna_matter.h, not matter.h: on a case-insensitive file system <Matter.h> of Arduino-ESP32 found this one)

Matter On/Off plug endpoints for the relays. The shared part (matter.cpp) keeps settings, the terminal commands, the
WebUI panel and the relay link; a backend per architecture runs the Matter stack:
- arch/beken/matter_beken.cpp: libMatterApp.a, connectedhomeip built for the BK7231N (matter-bk7231n)
- arch/esp32/matter_esp32.cpp: the Matter library of Arduino-ESP32 core 3.x (esp_matter)

*/

#pragma once

#include <Arduino.h>

#include <cstddef>

namespace espurna {
namespace matter {

// From the backend, any task: applied in the loop
void onoff_changed(size_t index, bool on);
void commissioning(bool active);
void pong();

// From the backend, any task: one log line (the shared part prints it from the loop)
void log_line(const char* line);

namespace backend {

// Relays that get an endpoint (index 0 -> endpoint 1, ...)
size_t endpoints();

void setup();
bool start();
void network(bool up);

void set_onoff(size_t index, bool on);
bool codes(char* manual, size_t manual_size, char* qr, size_t qr_size);
int fabrics();
void open_window();
void ping();

// "vendor (0xVID) / product (0xPID)" for the panel and the status command
String product();

size_t heap();
size_t heap_min();

} // namespace backend
} // namespace matter
} // namespace espurna

void matterSetup();
