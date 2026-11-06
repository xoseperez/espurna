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
// pairing (commissioning) window opened / closed
void window(bool open);

// From the backend, any task: one log line (the shared part prints it from the loop)
void log_line(const char* line);

// Name of the plug of a relay when the device is a bridge (relay name `relayName<n>`, else "<hostname> <n>" counted
// from 1) and its unique id ("<chip id>-<n>"). From the loop: settings
void plug_name(size_t index, char* label, size_t label_size, char* unique, size_t unique_size);

namespace backend {

// Relays that get an endpoint (index 0 -> endpoint 1, ...)
size_t endpoints();

void setup();
bool start();
void network(bool up);

void set_onoff(size_t index, bool on);
bool codes(char* manual, size_t manual_size, char* qr, size_t qr_size);
int fabrics();

// Controllers the device was added to (fabrics): for the WebUI list, and to remove one that the controller app forgot
// (deleting a device in the hub's phone app does not tell the device; adding it to the same app again then fails)
struct Controller {
    uint8_t index;
    uint16_t vendor;
    uint64_t node;
    char label[33];
};

size_t controllers(Controller* out, size_t size);
bool remove_controller(uint8_t index);
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

bool matterStarted();
void matterRebootForUpload();

#if MATTER_TAIL_SUPPORT
// Matter code in its own flash region (Beken, scripts/beken_matter_tail.py), written from the web OTA form.
// Functions return nullptr on success or a PSTR() error message
bool matterTailIsUpload(const uint8_t* data, size_t length);
bool matterTailBlocksOta();
const char* matterTailBegin();
const char* matterTailWrite(const uint8_t* data, size_t length);
const char* matterTailEnd();
void matterTailAbort();
#endif
