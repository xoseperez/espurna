/*

SAFEBOOT (ESP32)

Asymmetric flash layout (esp32_4mb_safeboot.csv, Tasmota's offsets): a factory app with the Tasmota safeboot image
and a single big OTA app for ESPurna. The same bits as the NSpanel module has for the Tasmota layout, for any ESP32:
- `safeboot` terminal command: next boot from the factory partition, where the new image is uploaded
- crash-loop guard: a few crashes in a row switch to safeboot, so a broken image can still be replaced over Wi-Fi
- web /upgrade answers with an error when there is no second OTA slot (Update would overwrite the running image)
- web /safeboot: migration from the two-slot layouts over Wi-Fi, update from an URL through safeboot

*/

#pragma once

// Early, right after the basic modules (works in the 'unstable' system mode as well)
void safebootSetup();

// The /safeboot page (flash layout migration, update from an URL) and the /upgrade guard, after webSetup()
void safebootWebSetup();
