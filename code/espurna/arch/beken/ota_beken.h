/*

BK7231N: ONE OTA FILE FOR BOTH BOOTLOADERS

*/

#pragma once

#include <cstddef>
#include <cstdint>

// espurna-beken.bin (scripts/beken_image.py): a header, a table of parts and the parts. The board writes the one its
// bootloader takes into that bootloader's 'download' partition: the RBL for the Tuya layout (the image without the
// Matter code) or for the Quectel layout (the whole image). On the Tuya layout the same file uploaded again after the
// reboot, by the image it carries, writes the Matter code (ref. arch/beken/matter_beken.cpp). Functions return
// nullptr on success or a PSTR() error message
bool bekenImageIsUpload(const uint8_t* data, size_t length);
const char* bekenImageBegin();
const char* bekenImageWrite(const uint8_t* data, size_t length);
const char* bekenImageEnd();
void bekenImageAbort();

// The upload just finished wrote the image without its Matter code (Tuya layout): the same file has to go up
// once more after the reboot. The web OTA form answers "OK, again" then
bool bekenImageAgain();

// Plain LibreTiny .uf2 uploads write the 'download' partition of the build's own (Tuya) layout. On a board with
// another bootloader that is the running firmware
const char* bekenUf2Allowed();
